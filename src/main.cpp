#define STB_IMAGE_IMPLEMENTATION
#include "Global.hpp"
#include "Window.hpp"
#include "Chunks.hpp"
#include "StateGame.hpp"
#include <chrono>
#include <iostream>

Global global;

constexpr auto WIDTH = 1280;
constexpr auto HEIGHT = 720;

int main(int argc, char *argv[]) {
    global.time = std::make_unique<Time>([]() -> uint64_t {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    });

    global.window = std::make_unique<Window>(glm::ivec2(WIDTH, HEIGHT), "Voxel Engine"); 
    auto *wnd = global.window.get();

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

            std::printf("FPS: %lld | TPS: %lld\n", wnd->fps, wnd->tps);
        }

        constexpr uint64_t NANOS_PER_TICK = (Time::NANOS_PER_SECOND / 60);
        uint64_t tick_time = wnd->frame_delta + wnd->tick_remainder;
        while (tick_time > NANOS_PER_TICK) {
            wnd->ticks++;
            game->tick();
            tick_time -= NANOS_PER_TICK;
        }
        wnd->tick_remainder = std::max<uint64_t>(tick_time, 0);

        game->update();
        game->render();

        wnd->swap_buffers();
        wnd->get_mouse()->clear_delta();
        wnd->poll_events();

        wnd->frames++;
        wnd->get_keyboard()->update();
        wnd->get_mouse()->update();
    }

    game->destroy();
    return 0;
}