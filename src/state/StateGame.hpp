#pragma once

#include "State.hpp"
#include "VoxelRenderer.hpp"
#include "Camera.hpp"
#include "Hitbox.hpp"
#include "PhysicsSolver.hpp"
#include "Player.hpp"
#include "VoxelModel.hpp"

struct StateGame : public State {
    explicit StateGame(std::shared_ptr<Connection> conn)
        : State(STATE_GAME), server_conn(std::move(conn)) {}

    void init() override;
    void destroy() override;
    void update() override;
    void tick() override;
    void render() override;

    void render_ui();

private:
    void process_network_packets();
    void send_player_state();
    void request_entity_spawn(const glm::vec3 &spawn_pos);

    std::shared_ptr<Connection> server_conn;

    std::unique_ptr<VoxelRenderer> renderer;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<PhysicsSolver> physics_solver;
    
    Player player;

    std::shared_ptr<VoxelModel> player_model;

    uint32_t local_player_id = 0;
    bool has_spawned = false;
    float cam_x = 0.0f, cam_y = 0.0f;
    float player_speed = 4.0f;
    int choosen_block = 1;

    glm::vec3 target_position { 32.0f, 120.0f, 32.0f };
};