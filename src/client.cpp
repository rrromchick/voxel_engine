#define STB_IMAGE_IMPLEMENTATION
#include "Global.hpp"
#include "Window.hpp"
#include "Chunks.hpp"
#include "Network.hpp"
#include "StateGame.hpp"
#include <thread>
#include <chrono>
#include <iostream>

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
    return sock;
}

int main() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    socket_t raw_sock = connect_to_host("127.0.0.1", 8080);
    if (raw_sock == INVALID_SOCK) {
        std::cout << "Could not connect to server.\n";
        return 1;
    }

    auto conn = std::make_shared<TcpConnection>(raw_sock);
    auto network = std::make_unique<Network>();
    network->add_connection(conn);

    std::cout << "Connected! Type a message and press Enter:\n>";

    std::thread input_thread([conn]() {
        std::string line;
        while (conn->is_open() && std::getline(std::cin, line)) {
            if (!line.empty()) {
                conn->send(line.c_str(), line.size());
                std::cout << "> ";
            }
        }
    });
    input_thread.detach();

    char buffer[512];
    while (conn->is_open()) {
        network->update();

        int bytes = conn->recv(buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::cout << "\n[Message]: " << buffer << "\n> " << std::flush;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "\nConnection closed by server.\n";

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}