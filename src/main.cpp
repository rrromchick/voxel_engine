#include "Global.hpp"
#include "Network.hpp"
#include "Components.hpp"
#include "PhysicsSolver.hpp"
#include "WorldGenerator.hpp"
#include "WorldFiles.hpp"
#include "Lighting.hpp"
#include "Lightmap.hpp"
#include "Level.hpp"
#include "Window.hpp"

#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <cmath>
#include <span>
#include <cstring>

Global global;
static std::unordered_map<uint32_t, float> knockback_timers;

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

void process_client_disconnect(const std::shared_ptr<Connection> &client) {
    if (client->get_user_data() == nullptr) return;

    uint32_t entity_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(client->get_user_data())) - 1;
    client->set_user_data(nullptr);

    ECS::Object player_entity { global.ecs.get(), static_cast<EntityId>(entity_id) };
    if (player_entity.is_valid()) {
        global.ecs->destroy(player_entity);
    }

    Packet despawn_packet(PacketType::EntityDespawn);
    despawn_packet.write<uint32_t>(entity_id);
    broadcast_packet_all(despawn_packet);
}

void resolve_entity_collisions(ECS &ecs) {
    auto is_solid = [&](const glm::vec3 &pos, const glm::vec3 &halfsize) {
        if (!ecs.level) return false;
        
        glm::vec3 min_b = pos - halfsize + glm::vec3(0.05f);
        glm::vec3 max_b = pos + halfsize - glm::vec3(0.05f);

        int x1 = static_cast<int>(std::floor(min_b.x));
        int x2 = static_cast<int>(std::floor(max_b.x));
        int y1 = static_cast<int>(std::floor(min_b.y));
        int y2 = static_cast<int>(std::floor(max_b.y));
        int z1 = static_cast<int>(std::floor(min_b.z));
        int z2 = static_cast<int>(std::floor(max_b.z));

        for (int x = x1; x <= x2; x++) {
            for (int y = y1; y <= y2; y++) {
                for (int z = z1; z <= z2; z++) {
                    uint8_t block_id = ecs.level->get_voxel_id(x, y, z);
                    if (block_id != 0 && global.blocks[block_id] && global.blocks[block_id]->obstacle) {
                        return true;
                    }
                }
            }
        }
        return false;
    };

    for (std::size_t i = 0; i < ecs.size; i++) {
        ECS::Object obj_a { &ecs, static_cast<EntityId>(i) };
        if (!obj_a.is_valid() || !obj_a.has<HitboxComponent>() || !obj_a.has<TransformComponent>()) continue;

        auto &hb_a = obj_a.get<HitboxComponent>();

        for (std::size_t j = i + 1; j < ecs.size; j++) {
            ECS::Object obj_b { &ecs, static_cast<EntityId>(j) };
            if (!obj_b.is_valid() || !obj_b.has<HitboxComponent>() || !obj_b.has<TransformComponent>()) continue;
            
            auto &hb_b = obj_b.get<HitboxComponent>();

            glm::vec3 min_a = hb_a.position - hb_a.halfsize;
            glm::vec3 max_a = hb_a.position + hb_a.halfsize;
            glm::vec3 min_b = hb_b.position - hb_b.halfsize;
            glm::vec3 max_b = hb_b.position + hb_b.halfsize;

            bool overlap_x = (min_a.x < max_b.x) && (max_a.x > min_b.x);
            bool overlap_y = (min_a.y < max_b.y) && (max_a.y > min_b.y);
            bool overlap_z = (min_a.z < max_b.z) && (max_a.z > min_b.z);

            if (overlap_x && overlap_y && overlap_z) {
                float overlap_depth_x = std::min(max_a.x - min_b.x, max_b.x - min_a.x);
                float overlap_depth_y = std::min(max_a.y - min_b.y, max_b.y - min_a.y);
                float overlap_depth_z = std::min(max_a.z - min_b.z, max_b.z - min_a.z);

                if (overlap_depth_y < overlap_depth_x && overlap_depth_y < overlap_depth_z) {
                    if (hb_a.position.y > hb_b.position.y) {
                        float target_y_a = (hb_b.position.y + hb_b.halfsize.y) + hb_a.halfsize.y;
                        glm::vec3 test_pos_a = hb_a.position;
                        test_pos_a.y = target_y_a;

                        if (!is_solid(test_pos_a, hb_a.halfsize)) {
                            hb_a.position = test_pos_a;
                            hb_a.velocity.y = 0.0f;
                            hb_a.grounded = true;
                        }
                    } else {
                        float target_y_b = (hb_a.position.y + hb_a.halfsize.y) + hb_b.halfsize.y;
                        glm::vec3 test_pos_b = hb_b.position;
                        test_pos_b.y = target_y_b;

                        if (!is_solid(test_pos_b, hb_b.halfsize)) {
                            hb_b.position = test_pos_b;
                            hb_b.velocity.y = 0.0f;
                            hb_b.grounded = true;
                        }
                    }
                } else {
                    if (overlap_depth_x < overlap_depth_z) {
                        float push_x = overlap_depth_x * 0.5f;
                        glm::vec3 test_pos_a = hb_a.position;
                        glm::vec3 test_pos_b = hb_b.position;

                        if (hb_a.position.x < hb_b.position.x) {
                            test_pos_a.x -= push_x;
                            test_pos_b.x += push_x;
                        } else {
                            test_pos_a.x += push_x;
                            test_pos_b.x -= push_x;
                        }

                        bool solid_a = is_solid(test_pos_a, hb_a.halfsize);
                        bool solid_b = is_solid(test_pos_b, hb_b.halfsize);

                        if (!solid_a && !solid_b) {
                            hb_a.position = test_pos_a;
                            hb_b.position = test_pos_b;
                        } else if (!solid_a) {
                            hb_a.position = test_pos_a;
                        } else if (!solid_b) {
                            hb_b.position = test_pos_b;
                        }
                    } else {
                        float push_z = overlap_depth_z * 0.5f;
                        glm::vec3 test_pos_a = hb_a.position;
                        glm::vec3 test_pos_b = hb_b.position;

                        if (hb_a.position.z < hb_b.position.z) {
                            test_pos_a.z -= push_z;
                            test_pos_b.z += push_z;
                        } else {
                            test_pos_a.z += push_z;
                            test_pos_b.z -= push_z;
                        }

                        bool solid_a = is_solid(test_pos_a, hb_a.halfsize);
                        bool solid_b = is_solid(test_pos_b, hb_b.halfsize);

                        if (!solid_a && !solid_b) {
                            hb_a.position = test_pos_a;
                            hb_b.position = test_pos_b;
                        } else if (!solid_a) {
                            hb_a.position = test_pos_a;
                        } else if (!solid_b) {
                            hb_b.position = test_pos_b;
                        }
                    }
                }
            }
        }
    }
}

void process_client_packet(const std::shared_ptr<Connection> &client, Packet &packet) {
    packet.reset_read();

    switch (packet.get_type()) {
        case PacketType::EntitySpawn: {
            if (client->get_user_data() != nullptr) break;

            auto spawn_pos = packet.read_vec3();
            spawn_pos.y += 0.05f;

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
            if (!player_entity.has<HealthComponent>()) {
                player_entity.add<HealthComponent>();
            }

            auto spawn_cx = static_cast<int>(std::floor(spawn_pos.x / Chunk::WIDTH));
            auto spawn_cy = static_cast<int>(std::floor(spawn_pos.y / Chunk::HEIGHT));
            auto spawn_cz = static_cast<int>(std::floor(spawn_pos.z / Chunk::DEPTH));

            for (int dy = -1; dy <= 1; dy++) {
                for (int dz = -1; dz <= 1; dz++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        global.ecs->level->ensure_loaded_around(spawn_cx + dx, spawn_cy + dy, spawn_cz + dz, 0, global.world_files.get());
                    }
                }
            }

            for (const auto &srv : global.network->get_servers()) {
                for (const auto &other_client : srv->get_clients()) {
                    if (other_client != client && other_client->get_user_data() != nullptr) {
                        uint32_t existing_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(other_client->get_user_data())) - 1;
                        ECS::Object existing_obj { global.ecs.get(), static_cast<EntityId>(existing_id) };

                        if (existing_obj.has<TransformComponent>()) {
                            Packet spawn_existing(PacketType::EntitySpawn);
                            spawn_existing.write<uint32_t>(existing_id);
                            spawn_existing.write_vec3(existing_obj.get<TransformComponent>().position);
                            spawn_existing.write<bool>(false);
                            client->send_packet(spawn_existing);
                        }
                    }
                }
            }

            Packet spawn_response(PacketType::EntitySpawn);
            spawn_response.write<uint32_t>(entity_id);
            spawn_response.write_vec3(trans.position);
            spawn_response.write<bool>(true);
            client->send_packet(spawn_response);

            Packet broadcast_spawn(PacketType::EntitySpawn);
            broadcast_spawn.write<uint32_t>(entity_id);
            broadcast_spawn.write_vec3(trans.position);
            broadcast_spawn.write<bool>(false);

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

                uint8_t client_health = packet.read<uint8_t>();
                if (obj.has<HealthComponent>()) {
                    obj.get<HealthComponent>().health = client_health; 
                }

                glm::vec3 move_dir { 0.0f };

                glm::vec3 forward = glm::vec3(-std::sin(in.yaw), 0.0f, -std::cos(in.yaw));
                glm::vec3 right   = glm::vec3( std::cos(in.yaw), 0.0f, -std::sin(in.yaw));

                if (in.move_forward)  move_dir += forward;
                if (in.move_backward) move_dir -= forward;
                if (in.move_right)    move_dir += right;
                if (in.move_left)     move_dir -= right;

                if (knockback_timers.find(player_id) == knockback_timers.end()) {
                    if (glm::length(move_dir) > 0.0001f) {
                        move_dir = glm::normalize(move_dir);
                        float speed = in.sprinting ? 7.0f : 4.5f;
                        hb.velocity.x = move_dir.x * speed;
                        hb.velocity.z = move_dir.z * speed;
                    } else {
                        hb.velocity.x = 0.0f;
                        hb.velocity.z = 0.0f;
                    }
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
                        global.lighting->solve(); 
                    }
                }

                Packet broadcast_modify(PacketType::BlockModify);
                broadcast_modify.write<int32_t>(x);
                broadcast_modify.write<int32_t>(y);
                broadcast_modify.write<int32_t>(z);
                broadcast_modify.write<uint8_t>(block_id);

                broadcast_packet_all(broadcast_modify);
            }
            break;
        }
        case PacketType::EntityDespawn: {
            if (client->get_user_data() == nullptr) break;

            auto entity_id = packet.read<uint32_t>();
            client->set_user_data(nullptr);

            ECS::Object player_entity { global.ecs.get(), static_cast<EntityId>(entity_id) };
            if (player_entity.is_valid()) {
                global.ecs->destroy(player_entity);
            }

            Packet broadcast_despawn(PacketType::EntityDespawn);
            broadcast_despawn.write<uint32_t>(entity_id);

            for (const auto &srv : global.network->get_servers()) {
                for (const auto &other_client : srv->get_clients()) {
                    if (other_client != client) {
                        other_client->send_packet(broadcast_despawn);
                    }
                }
            }
            break;
        }
        case PacketType::EntityAttack: {
            if (client->get_user_data() == nullptr) break;

            uint32_t attacker_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(client->get_user_data())) - 1;
            uint32_t target_id = packet.read<uint32_t>();

            ECS::Object attacker_obj { global.ecs.get(), static_cast<EntityId>(attacker_id) };
            ECS::Object target_obj { global.ecs.get(), static_cast<EntityId>(target_id) };

            if (!attacker_obj.is_valid() || !target_obj.is_valid()) break;
            if (!attacker_obj.has<TransformComponent>() || !target_obj.has<HitboxComponent>()) break;

            auto &attacker_trans = attacker_obj.get<TransformComponent>();
            auto &target_hb = target_obj.get<HitboxComponent>();

            glm::vec3 attacker_cam_pos = attacker_trans.position + glm::vec3(0.0f, 1.6f, 0.0f);
            float dist = glm::distance(attacker_cam_pos, target_hb.position);

            if (dist <= 5.0f) {
                glm::vec3 knockback_dir = glm::normalize(target_hb.position - attacker_cam_pos);
                knockback_dir.y = 0.0f; 
                target_hb.velocity = glm::normalize(knockback_dir) * 7.0f + glm::vec3(0.0f, 4.5f, 0.0f);
                target_hb.grounded = false;
                knockback_timers[target_id] = 0.25f;

                if (target_obj.has<HealthComponent>()) {
                    auto &health_comp = target_obj.get<HealthComponent>();
                    health_comp.remove_health(1);
                    std::cout << "[Server] Player " << attacker_id << " dealt damage to Player " << target_id << 
                        ". Target HP: " << static_cast<int>(health_comp.health) << std::endl;
                }

                Packet broadcast_attack(PacketType::EntityAttack);
                broadcast_attack.write<uint32_t>(attacker_id);
                broadcast_attack.write<uint32_t>(target_id);
                broadcast_packet_all(broadcast_attack);
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

    global.ecs->level = std::make_unique<Level>();
    global.ecs->level->ensure_loaded_around(0, 0, 0, 4, global.world_files.get());
    
    std::vector<glm::vec3> initial_positions = { glm::vec3(0.0f) };
    global.ecs->level->decorate_visible(initial_positions);

    if (global.lighting) {
        for (const auto &[pos, chunk] : global.ecs->level->chunks) {
            if (chunk) {
                global.lighting->on_chunk_loaded(chunk->x, chunk->y, chunk->z);
            }
        }
        global.lighting->solve();
    }

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

    constexpr uint64_t SAVE_INTERVAL_NANOS = 30ULL * Time::NANOS_PER_SECOND;
    uint64_t last_save_time = global.time->now();

    while (server->is_open()) {
        uint64_t current_time = global.time->now();
        uint64_t elapsed = current_time - last_tick_time;

        if (elapsed >= NANOS_PER_TICK) {
            auto dt = static_cast<float>(elapsed) / static_cast<float>(Time::NANOS_PER_SECOND);
            last_tick_time = current_time;

            global.network->update();

            for (const auto &srv : global.network->get_servers()) {
                auto clients = srv->get_clients();
                for (auto it = clients.begin(); it != clients.end();) {
                    auto &client = *it;
                    if (!client->is_open()) {
                        process_client_disconnect(client);
                        it = clients.erase(it);
                        continue;
                    }

                    auto packets = client->poll_packets();
                    for (auto &packet : packets) {
                        process_client_packet(client, packet);
                    }
                    ++it;
                }
            }

            std::vector<glm::vec3> player_positions;
            for (std::size_t i = 0; i < global.ecs->size; i++) {
                ECS::Object obj { global.ecs.get(), static_cast<EntityId>(i) };
                if (obj.is_valid() && obj.has<TransformComponent>() && obj.has<PlayerInputComponent>()) {
                    auto &pos = obj.get<TransformComponent>().position;
                    player_positions.push_back(pos);

                    global.ecs->level->ensure_loaded_around(
                        static_cast<int>(std::floor(pos.x / Chunk::WIDTH)),
                        static_cast<int>(std::floor(pos.y / Chunk::HEIGHT)),
                        static_cast<int>(std::floor(pos.z / Chunk::DEPTH)),
                        2,
                        global.world_files.get());
                }
            }

            if (!player_positions.empty()) {
                global.ecs->level->decorate_visible(player_positions);
            }

            dt = std::min<float>(dt, 0.05f);

            for (auto it = knockback_timers.begin(); it != knockback_timers.end(); ) {
                it->second -= dt;
                if (it->second <= 0.0f) {
                    it = knockback_timers.erase(it);
                } else {
                    ++it;
                }
            }

            physics_solver->step(dt, 4);
            resolve_entity_collisions(*global.ecs);

            for (std::size_t i = 0; i < global.ecs->size; i++) {
                ECS::Object obj_a { global.ecs.get(), static_cast<EntityId>(i) };
                if (!obj_a.is_valid() || !obj_a.has<HitboxComponent>()) continue;
                auto &hb_a = obj_a.get<HitboxComponent>();

                for (std::size_t j = 0; j < global.ecs->size; j++) {
                    if (i == j) continue;
                    ECS::Object obj_b { global.ecs.get(), static_cast<EntityId>(j) };
                    if (!obj_b.is_valid() || !obj_b.has<HitboxComponent>()) continue;
                    auto &hb_b = obj_b.get<HitboxComponent>();

                    glm::vec3 min_a = hb_a.position - hb_a.halfsize;
                    glm::vec3 max_a = hb_a.position + hb_a.halfsize;
                    glm::vec3 min_b = hb_b.position - hb_b.halfsize;
                    glm::vec3 max_b = hb_b.position + hb_b.halfsize;

                    bool horizontal_overlap = (max_a.x > min_b.x && min_a.x < max_b.x) &&
                        (max_a.z > min_b.z && min_a.z < max_b.z);

                    float expected_y = max_b.y + hb_a.halfsize.y;
                    if (horizontal_overlap && std::abs(hb_a.position.y - expected_y) < 0.2f) {
                        hb_a.position.y = expected_y;
                        hb_a.velocity.y = 0.0f;
                        hb_a.grounded = true;
                    }
                }
            }

            for (std::size_t i = 0; i < global.ecs->size; i++) {
                ECS::Object obj { global.ecs.get(), static_cast<EntityId>(i) };

                if (obj.is_valid() && obj.has<TransformComponent>() && obj.has<HitboxComponent>()) {
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

            if (current_time - last_save_time >= SAVE_INTERVAL_NANOS) {
                last_save_time = current_time;
                if (global.ecs && global.ecs->level && global.world_files) {
                    for (auto &[pos, chunk] : global.ecs->level->chunks) {
                        if (chunk && chunk->modified) {
                            std::span<const uint8_t> voxel_span{ 
                                reinterpret_cast<const uint8_t*>(chunk->voxels.get()), 
                                Chunk::VOLUME 
                            };
                            global.world_files->put(voxel_span, chunk->x, chunk->y, chunk->z);
                            chunk->modified = false;
                        }
                    }
                    global.world_files->write();
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    if (global.ecs && global.ecs->level && global.world_files) {
        for (auto &[pos, chunk] : global.ecs->level->chunks) {
            if (chunk) {
                std::span<const uint8_t> voxel_span{ 
                    reinterpret_cast<const uint8_t*>(chunk->voxels.get()), 
                    Chunk::VOLUME 
                };
                global.world_files->put(voxel_span, pos.x, pos.y, pos.z);
            }
        }
        global.world_files->write();
    }

#ifdef _WIN32   
    WSACleanup();
#endif

    return 0;
}