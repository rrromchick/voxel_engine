#pragma once

#include "State.hpp"
#include "VoxelRenderer.hpp"
#include "Camera.hpp"
#include "Hitbox.hpp"
#include "PhysicsSolver.hpp"

struct StateGame : public State {
    StateGame() : State(STATE_MAIN_MENU) {}

    void init() override;
    void destroy() override;
    void update() override;
    void tick() override;
    void render() override;

    void render_ui();

private:
    std::unique_ptr<VoxelRenderer> renderer;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Hitbox> hitbox;
    std::unique_ptr<PhysicsSolver> physics_solver;

    float cam_x = 0.0f, cam_y = 0.0f;
    float player_speed = 4.0f;
    int choosen_block = 1;
};