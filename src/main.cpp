#include "Global.hpp"
#include "Network.hpp"
#include "Components.hpp"
#include "PhysicsSolver.hpp"
#include "WorldGenerator.hpp"
#include "WorldFiles.hpp"
#include "Lighting.hpp"
#include "Level.hpp"
#include "Window.hpp"

#include <chrono>
#include <thread>
#include <iostream>
#include <vector>

Global global;

static void setup_server_definitions() {
    global.blocks[BlockId::AIR] = std::make_unique<Block>(BlockId::AIR, 0);
    global.blocks[BlockId::AIR]->draw_group = 1;
    global.blocks[BlockId::AIR]->light_passing = true;
    global.blocks[BlockId::AIR]->obstacle = false;

    global.blocks[BlockId::GRASS] = std::make_unique<Block>(BlockId::GRASS, 16);
    global.blocks[BlockId::GRASS]->texture_faces[3] = 1;
    global.blocks[BlockId::GRASS]->texture_faces[2] = 2;

    global.blocks[BlockId::DIRT] = std::make_unique<Block>(BlockId::DIRT, 2);

    global.blocks[BlockId::LAMP] = std::make_unique<Block>(BlockId::LAMP, 3);
    global.blocks[BlockId::LAMP]->emission[0] = 15;
    global.blocks[BlockId::LAMP]->emission[1] = 14;
    global.blocks[BlockId::LAMP]->emission[2] = 13;

    global.blocks[BlockId::STONE] = std::make_unique<Block>(BlockId::STONE, 6);
    global.blocks[BlockId::SAND] = std::make_unique<Block>(BlockId::SAND, 5);
    global.blocks[BlockId::GRAVEL] = std::make_unique<Block>(BlockId::GRAVEL, 8);

    global.blocks[BlockId::WATER] = std::make_unique<Block>(BlockId::WATER, 7);
    global.blocks[BlockId::WATER]->draw_group = 2;
    global.blocks[BlockId::WATER]->light_passing = true;
    global.blocks[BlockId::WATER]->obstacle = false;

    global.blocks[BlockId::CLAY] = std::make_unique<Block>(BlockId::CLAY, 8);

    global.blocks[BlockId::LOG] = std::make_unique<Block>(BlockId::LOG, 9);
    global.blocks[BlockId::LOG]->texture_faces[2] = 17;
    global.blocks[BlockId::LOG]->texture_faces[3] = 17;

    global.blocks[BlockId::LEAVES] = std::make_unique<Block>(BlockId::LEAVES, 10);
    global.blocks[BlockId::LEAVES]->draw_group = 2;
    global.blocks[BlockId::LEAVES]->light_passing = true;

    global.blocks[BlockId::ROSE] = std::make_unique<Block>(BlockId::ROSE, 11);
    global.blocks[BlockId::ROSE]->draw_group = 2;
    global.blocks[BlockId::ROSE]->light_passing = true;
    global.blocks[BlockId::ROSE]->obstacle = false;
    global.blocks[BlockId::ROSE]->model = BlockModel::PLANT;

    global.blocks[BlockId::BUTTERCUP] = std::make_unique<Block>(BlockId::BUTTERCUP, 12);
    global.blocks[BlockId::BUTTERCUP]->draw_group = 2;
    global.blocks[BlockId::BUTTERCUP]->light_passing = true;
    global.blocks[BlockId::BUTTERCUP]->obstacle = false;
    global.blocks[BlockId::BUTTERCUP]->model = BlockModel::PLANT;

    global.blocks[BlockId::COAL] = std::make_unique<Block>(BlockId::COAL, 13);
    global.blocks[BlockId::COPPER] = std::make_unique<Block>(BlockId::COPPER, 14);

    global.blocks[BlockId::LAVA] = std::make_unique<Block>(BlockId::LAVA, 15);
    global.blocks[BlockId::LAVA]->draw_group = 2;
    global.blocks[BlockId::LAVA]->light_passing = true;
    global.blocks[BlockId::LAVA]->obstacle = false;
    global.blocks[BlockId::LAVA]->emission[0] = 15;
    global.blocks[BlockId::LAVA]->emission[1] = 8;
    global.blocks[BlockId::LAVA]->emission[2] = 2;
}

void broadcast_packet_all(Packet &packet) {
    for (const auto &server : global.network->get_servers()) {
        for (const auto &client : server->get_clients()) {
            client->send_packet(packet);
        }
    }
}

void process_client_packet(const std::shared_ptr<Connection> &client, Packet &packet) {
    packet.reset_read();

    switch (packet.get_type()) {
        case PacketType::EntitySpawn: {
            if (client->get_user_data() != nullptr) break;

            auto spawn_pos = packet.read_vec3();
            auto player_entity_opt = global.ecs->create();
            if (!player_entity_opt) break;

            ECS::Object player_entity = player_entity_opt.value();
            uint32_t entity_id = static_cast<uint32_t>(player_entity.id);

            client->set_user_data(reinterpret_cast<void*>(static_cast<uintptr_t>(entity_id + 1)));

            auto &trans = player_entity.add<TransformComponent>();
            trans.position = spawn_pos;

            auto &hb = player_entity.add<HitboxComponent>();
            hb.halfsize = glm::vec3(0.3f, 0.9f, 0.3f); 
            hb.position = spawn_pos + glm::vec3(0.0f, hb.halfsize.y, 0.0f);
            hb.velocity = glm::vec3(0.0f);
            hb.grounded = false;

            player_entity.add<PlayerInputComponent>();

            for (const auto &srv : global.network->get_servers()) {
                for (const auto &other_client : srv->get_clients()) {
                    if (other_client != client && other_client->get_user_data() != nullptr) {
                        uint32_t existing_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(other_client->get_user_data()));
                        ECS::Object existing_obj { global.ecs.get(), static_cast<EntityId>(existing_id) };

                        Packet spawn_existing(PacketType::EntitySpawn);
                        spawn_existing.write<uint32_t>(existing_id);
                        spawn_existing.write_vec3(existing_obj.get<TransformComponent>().position);
                        client->send_packet(spawn_existing);
                    }
                }
            }

            Packet response_packet(PacketType::EntitySpawn);
            response_packet.write<uint32_t>(entity_id);
            response_packet.write_vec3(trans.position);
            client->send_packet(response_packet);

            Packet broadcast_spawn(PacketType::EntitySpawn);
            broadcast_spawn.write<uint32_t>(entity_id);
            broadcast_spawn.write_vec3(trans.position);

            for (const auto &srv : global.network->get_servers()) {
                for (const auto &other_client : srv->get_clients()) {
                    if (other_client != client) {
                        other_client->send_packet(broadcast_spawn);
                    }
                }
            }
            break;
        }
        case PacketType::PlayerState: {
            if (client->get_user_data() == nullptr) break;

            uint32_t player_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(client->get_user_data())) - 1;

            ECS::Object obj { global.ecs.get(), static_cast<EntityId>(player_id) };
            if (obj.has<PlayerInputComponent>() && obj.has<HitboxComponent>() && obj.has<TransformComponent>()) {
                auto &in = obj.get<PlayerInputComponent>();
                auto &hb = obj.get<HitboxComponent>();
                auto &trans = obj.get<TransformComponent>();

                in.move_forward   = packet.read<bool>();
                in.move_backward  = packet.read<bool>();
                in.move_left      = packet.read<bool>();
                in.move_right     = packet.read<bool>();
                in.jump           = packet.read<bool>();
                in.sprinting      = packet.read<bool>();
                in.shifting       = packet.read<bool>();
                in.is_swimming_up = packet.read<bool>();
                
                in.yaw            = packet.read<float>();
                in.pitch          = packet.read<float>();

                trans.rotation.y = in.yaw;
                trans.rotation.x = in.pitch;

                glm::vec3 move_dir { 0.0f };

                glm::vec3 forward = glm::vec3(-std::sin(in.yaw), 0.0f, -std::cos(in.yaw));
                glm::vec3 right   = glm::vec3( std::cos(in.yaw), 0.0f, -std::sin(in.yaw));

                if (in.move_forward)  move_dir += forward;
                if (in.move_backward) move_dir -= forward;
                if (in.move_right)    move_dir += right;
                if (in.move_left)     move_dir -= right;

                if (glm::length(move_dir) > 0.0001f) {
                    move_dir = glm::normalize(move_dir);
                    float speed = in.sprinting ? 7.0f : 4.5f;
                    hb.velocity.x = move_dir.x * speed;
                    hb.velocity.z = move_dir.z * speed;
                } else {
                    hb.velocity.x = 0.0f;
                    hb.velocity.z = 0.0f;
                }

                if (in.jump && hb.grounded) {
                    hb.velocity.y = 7.5f;
                    hb.grounded = false;
                }
            }
            break;
        }
        case PacketType::BlockModify: {
            auto x = packet.read<int32_t>();
            auto y = packet.read<int32_t>();
            auto z = packet.read<int32_t>();
            auto block_id = packet.read<uint8_t>();

            if (static_cast<std::size_t>(block_id) < global.blocks.size() && global.blocks[block_id] != nullptr) {
                if (global.ecs->level) {
                    global.ecs->level->set(x, y, z, block_id);
                    if (global.lighting) {
                        global.lighting->on_block_set(x, y, z, block_id);
                    }
                }
                broadcast_packet_all(packet);
            }
            break;
        }
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    global.time = std::make_unique<Time>([]() -> uint64_t {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    });

    setup_server_definitions();

    global.ecs = std::make_unique<ECS>();
    register_server_components(*global.ecs);

    global.generator = std::make_unique<WorldGenerator>();
    global.world_files = std::make_unique<WorldFiles>("world/", REGION_VOL * (Chunk::VOLUME * 2 + 8));
    global.lighting = std::make_unique<Lighting>();

    global.ecs->level = std::make_unique<Level>(16, 4, 16, -8, -2, -8);
    
    while (global.ecs->level->load_visible(global.world_files.get())) {}
    while (global.ecs->level->decorate_visible()) {}

    auto physics_solver = std::make_unique<PhysicsSolver>(glm::vec3(0.0f, -16.0f, 0.0f));

    global.network = std::make_unique<Network>();
    auto server = global.network->create_tcp_server(8080);
    if (!server) {
        std::cerr << "[Server] Failed to bind TCP server to port 8080\n";
        return 1;
    }

    std::cout << "[Server] Listening on port 8080...\n";

    constexpr uint64_t TARGET_TPS = 20;
    constexpr uint64_t NANOS_PER_TICK = (Time::NANOS_PER_SECOND / TARGET_TPS);
    uint64_t last_tick_time = global.time->now();

    while (server->is_open()) {
        uint64_t current_time = global.time->now();
        uint64_t elapsed = current_time - last_tick_time;

        if (elapsed >= NANOS_PER_TICK) {
            auto dt = static_cast<float>(elapsed) / static_cast<float>(Time::NANOS_PER_SECOND);
            last_tick_time = current_time;

            global.network->update();

            for (const auto &srv : global.network->get_servers()) {
                for (const auto &client : srv->get_clients()) {
                    auto packets = client->poll_packets();
                    for (auto &packet : packets) {
                        process_client_packet(client, packet);
                    }
                }
            }

            for (std::size_t i = 0; i < global.ecs->size; i++) {
                ECS::Object obj { global.ecs.get(), static_cast<EntityId>(i) };
                if (obj.has<TransformComponent>()) {
                    auto &pos = obj.get<TransformComponent>().position;
                    global.ecs->level->set_center(
                        static_cast<int>(std::floor(pos.x / Chunk::WIDTH)),
                        static_cast<int>(std::floor(pos.y / Chunk::HEIGHT)),
                        static_cast<int>(std::floor(pos.z / Chunk::DEPTH)));

                    global.ecs->level->load_visible(global.world_files.get());
                    global.ecs->level->decorate_visible();
                }
            }

            physics_solver->step(dt, 4);

            for (std::size_t i = 0; i < global.ecs->size; i++) {
                ECS::Object obj { global.ecs.get(), static_cast<EntityId>(i) };

                if (obj.has<TransformComponent>() && obj.has<HitboxComponent>()) {
                    auto &trans = obj.get<TransformComponent>();
                    auto &hb = obj.get<HitboxComponent>();
                    
                    trans.position = hb.position - glm::vec3(0.0f, hb.halfsize.y, 0.0f);

                    Packet update_packet(PacketType::EntityStateUpdate);
                    update_packet.write<uint32_t>(static_cast<uint32_t>(obj.id));
                    update_packet.write_vec3(trans.position);
                    update_packet.write_vec3(hb.velocity);
                    update_packet.write_vec3(trans.rotation);

                    broadcast_packet_all(update_packet);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

#ifdef _WIN32   
    WSACleanup();
#endif

    return 0;
}