#define STB_IMAGE_IMPLEMENTATION
#include "Global.hpp"
#include "Window.hpp"
#include "Level.hpp"
#include "Network.hpp"
#include "StateGame.hpp"
#include "ComponentManager.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <filesystem>

#ifndef _WIN32
#include <fcntl.h>
#endif

Global global;

constexpr auto WIDTH = 1280;
constexpr auto HEIGHT = 720;

socket_t connect_to_host(const char *host, int port) {
    addrinfo hints {}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, nullptr, &hints, &res) != 0) {
        return INVALID_SOCK;
    }

    sockaddr_in addr {};
    std::memcpy(&addr, res->ai_addr, sizeof(sockaddr_in));
    addr.sin_port = htons(port);
    freeaddrinfo(res);

    socket_t sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCK) return INVALID_SOCK;

    if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        CLOSE_SOCK(sock);
        return INVALID_SOCK;
    }

    set_non_blocking(sock);
    return sock;
}

int main() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    global.time = std::make_unique<Time>([]() -> uint64_t {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
                .count();
    });

    global.window = std::make_unique<Window>(glm::ivec2(WIDTH, HEIGHT), "Voxel Engine");
    auto *wnd = global.window.get();

    socket_t raw_sock = connect_to_host("127.0.0.1", 8080);
    if (raw_sock == INVALID_SOCK) {
        std::cout << "Could not connect to server.\n";
        return 1;
    }

    auto conn = std::make_shared<TcpConnection>(raw_sock);
    auto network = std::make_unique<Network>();
    network->add_connection(conn);

    std::cout << "Connected! Type a message and press Enter:\n";

    std::thread input_thread([conn]() {
        std::string line;
        while (conn->is_open() && std::getline(std::cin, line)) {
            if (!line.empty()) {
                conn->send(line.c_str(), line.size());
                std::cout << "> " << std::flush;
            }
        }
    });
    input_thread.detach();

    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSwapInterval(1);

    auto game = std::make_unique<StateGame>();
    game->init();
    
    wnd->last_frame = global.time->now();
    wnd->frame_delta = 0;
    wnd->last_second = wnd->last_frame;

    char chat_buffer[512];

    while (!wnd->is_should_close()) {
        auto current_time = global.time->now();
        uint64_t raw_delta = current_time - wnd->last_frame;
        wnd->frame_delta = raw_delta;
        wnd->last_frame = current_time;

        if (current_time - wnd->last_second > Time::NANOS_PER_SECOND) {
            wnd->fps = wnd->frames;
            wnd->tps = wnd->ticks;
            wnd->frames = 0;
            wnd->ticks = 0;
            wnd->last_second = current_time;
        }

        constexpr uint64_t NANOS_PER_TICK = (Time::NANOS_PER_SECOND / 60);
        uint64_t tick_time = wnd->frame_delta + wnd->tick_remainder;
        while (tick_time > NANOS_PER_TICK) {
            wnd->ticks++;
            game->tick();
            tick_time -= NANOS_PER_TICK;
        } 
        wnd->tick_remainder = std::max<uint64_t>(tick_time, static_cast<uint64_t>(0));

        if (conn->is_open()) {
            network->update();

            int bytes = conn->recv(chat_buffer, sizeof(chat_buffer) - 1);
            if (bytes > 0) {
                chat_buffer[bytes] = '\0';
                std::cout << "\n[Message]: " << chat_buffer << "\n> " << std::flush;
            }
        }

        game->update();
        game->render();
        game->render_ui();

        wnd->swap_buffers();
        wnd->get_mouse()->clear_delta();
        wnd->poll_events();

        wnd->frames++;
        wnd->get_keyboard()->update();
        wnd->get_mouse()->update();
    }

    game->destroy();
    game.reset();

    if (conn->is_open()) {
        conn->close();
    }
    network.reset();

#ifdef _WIN32
    WSACleanup();
#endif

    std::cout << "\nExited.\n";
    return 0;
}