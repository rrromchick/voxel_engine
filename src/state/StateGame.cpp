#include "StateGame.hpp"
#include "Texture.hpp"
#include "Global.hpp"
#include "Block.hpp"
#include "Shader.hpp"
#include "Mesh.hpp"
#include "LineBatch.hpp"
#include "Window.hpp"
#include "Level.hpp"
#include "Lighting.hpp"
#include "WorldFiles.hpp"
#include "WorldGenerator.hpp"
#include "Player.hpp"
#include "Lightmap.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <algorithm>
#include <iostream>
#include <array>
#include <thread>

constexpr std::array<float, 8> vertices = {
    -0.01f, -0.01f,
     0.01f,  0.01f,
    -0.01f,  0.01f,
     0.01f, -0.01f,
};

constexpr std::array<int, 2> attrs = { 2, 0 };

static float block_action_cooldown = 0.0f;

static void setup_definitions() {
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

static bool init_assets() {
    global.shader = Shader::load("res/shaders/main.glslv", "res/shaders/main.glslf");
    if (!global.shader) return false;

    global.crosshair_shader = Shader::load("res/shaders/crosshair.glslv", "res/shaders/crosshair.glslf");
    if (!global.crosshair_shader) return false;

    global.lines_shader = Shader::load("res/shaders/lines.glslv", "res/shaders/lines.glslf");
    if (!global.lines_shader) return false;

    global.texture = std::make_unique<Texture>("res/images/texture_atlas.png");
    return global.texture != nullptr;
}

void StateGame::request_entity_spawn(const glm::vec3 &spawn_pos) {
    if (!server_conn || !server_conn->is_open()) return;

    Packet packet(PacketType::EntitySpawn);
    packet.write_vec3(spawn_pos);
    server_conn->send_packet(packet);
}

void StateGame::init() {
    setup_definitions();
    init_assets();

    player_model = std::make_shared<VoxelModel>();
    player_model->load_from_vox("res/vox/chr_knight.vox");

    global.ecs = std::make_unique<ECS>();
    register_client_components(*global.ecs);

    global.generator = std::make_unique<WorldGenerator>();
    global.world_files = std::make_unique<WorldFiles>("world/", REGION_VOL * (Chunk::VOLUME * 2 + 8));
    global.ecs->level = std::make_unique<Level>();

    renderer = std::make_unique<VoxelRenderer>(1024 * 1024);
    global.line_batch = std::make_unique<LineBatch>(4096);
    global.crosshair = std::make_unique<Mesh>(vertices, attrs);
    global.lighting = std::make_unique<Lighting>();

    camera = std::make_unique<Camera>(glm::vec3(32, 120.5f, 32), glm::radians(90.0f));
    player = Player::create(glm::vec3(32, 122.0f, 32));

    request_entity_spawn(glm::vec3(32.0f, 122.0f, 32.0f));

    global.ecs->level->ensure_loaded_around(
        static_cast<int>(std::floor(camera->position.x / Chunk::WIDTH)),
        static_cast<int>(std::floor(camera->position.y / Chunk::HEIGHT)),
        static_cast<int>(std::floor(camera->position.z / Chunk::DEPTH)),
        2,
        global.world_files.get()
    );

    if (global.lighting) {
        for (const auto &[pos, chunk] : global.ecs->level->chunks) {
            if (chunk) {
                global.lighting->on_chunk_loaded(chunk->x, chunk->y, chunk->z);
            }
        }
        global.lighting->solve();
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(global.window->get_handle(), true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void StateGame::process_network_packets() {
    if (!server_conn || !server_conn->is_open()) return;

    global.network->update();
    auto packets = server_conn->poll_packets();
    
    for (auto &packet : packets) {
        packet.reset_read();
        switch (packet.get_type()) {
            case PacketType::EntitySpawn: {
                auto spawned_id = packet.read<uint32_t>();
                glm::vec3 initial_pos = packet.read_vec3();
                bool is_local = packet.read<bool>();

                if (spawned_id >= global.ecs->size) {
                    global.ecs->resize(spawned_id + 16);
                }

                if (is_local) {
                    local_player_id = spawned_id;
                    has_spawned = true;

                    player = Player(global.ecs.get(), static_cast<EntityId>(spawned_id));
                    if (!player.has<TransformComponent>()) player.add<TransformComponent>();
                    if (!player.has<TransformComponent>()) {
                        auto &hb = player.add<HitboxComponent>();
                        hb.halfsize = glm::vec3(0.3f, 0.9f, 0.3f);
                    }
                    
                    if (!player.has<PlayerInputComponent>()) player.add<PlayerInputComponent>();
                    if (!player.has<InventoryComponent>()) {
                        auto &inv = player.add<InventoryComponent>();

                        inv.slots[0] = { BlockId::GRASS, 64 };
                        inv.slots[1] = { BlockId::DIRT, 64 };
                        inv.slots[2] = { BlockId::STONE, 64 };
                        inv.slots[3] = { BlockId::LAMP, 16 };
                        inv.slots[4] = { BlockId::LOG, 32 };
                    }
                    if (player.has<RenderComponent>()) player.remove<RenderComponent>();

                    target_position = initial_pos;
                    player.transform().position = target_position;
                    player.hitbox().position = target_position + glm::vec3(0.0f, player.hitbox().halfsize.y, 0.0f);

                    auto scx = static_cast<int>(std::floor(initial_pos.x / Chunk::WIDTH));
                    auto scy = static_cast<int>(std::floor(initial_pos.y / Chunk::HEIGHT));
                    auto scz = static_cast<int>(std::floor(initial_pos.z / Chunk::DEPTH));

                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dz = -1; dz <= 1; dz++) {
                            for (int dx = -1; dx <= 1; dx++) {
                                global.ecs->level->ensure_loaded_around(scx + dx, scy + dy, scz + dz, 0, global.world_files.get());
                            }
                        }
                    }
                } else {
                    ECS::Object obj { global.ecs.get(), static_cast<EntityId>(spawned_id) };
                    if (!obj.has<TransformComponent>()) {
                        auto &trans = obj.add<TransformComponent>();
                        trans.position = initial_pos;
                        trans.rotation = glm::vec3(0.0f);
                    }
                    if (!obj.has<RenderComponent>()) {
                        auto &rc = obj.add<RenderComponent>(RenderComponent(player_model));
                        rc.visible = true;
                    }
                }
                break;
            }
            case PacketType::EntityDespawn: {
                auto despawned_id = packet.read<uint32_t>();
                if (despawned_id < global.ecs->size) {
                    ECS::Object obj { global.ecs.get(), static_cast<EntityId>(despawned_id) };
                    if (obj.has<RenderComponent>()) obj.remove<RenderComponent>();
                    if (obj.has<TransformComponent>()) obj.remove<TransformComponent>();
                    if (obj.has<HitboxComponent>()) obj.remove<HitboxComponent>();
                    
                    global.ecs->destroy(obj);
                }
                break;
            }
            case PacketType::EntityStateUpdate: {
                auto entity_id = packet.read<uint32_t>();
                auto pos = packet.read_vec3();
                auto vel = packet.read_vec3();
                auto rot = packet.read_vec3();

                if (has_spawned && entity_id == local_player_id) {
                    target_position = pos;
                    player.hitbox().velocity = vel;
                } else {
                    if (entity_id >= global.ecs->size) {
                        global.ecs->resize(entity_id + 16);
                    }

                    ECS::Object obj { global.ecs.get(), static_cast<EntityId>(entity_id) };

                    if (!obj.has<TransformComponent>()) {
                        obj.add<TransformComponent>();
                    }
                    if (!obj.has<RenderComponent>()) {
                        auto &rc = obj.add<RenderComponent>(RenderComponent(player_model));
                        rc.visible = true; 
                    }

                    auto &trans = obj.get<TransformComponent>();
                    trans.position = pos;
                    trans.rotation = rot;
                }
                break;
            }
            case PacketType::BlockModify: {
                auto x = packet.read<int32_t>();
                auto y = packet.read<int32_t>();
                auto z = packet.read<int32_t>();
                auto block_id = packet.read<uint8_t>();

                if (global.ecs->level) {
                    global.ecs->level->set(x, y, z, block_id);

                    auto* level = global.ecs->level.get();
                    auto* chunk = level->get_chunk_by_voxel(x, y, z);
                    if (chunk) {
                        chunk->modified = true;

                        int lx = (x % Chunk::WIDTH + Chunk::WIDTH) % Chunk::WIDTH;
                        int ly = (y % Chunk::HEIGHT + Chunk::HEIGHT) % Chunk::HEIGHT;
                        int lz = (z % Chunk::DEPTH + Chunk::DEPTH) % Chunk::DEPTH;

                        auto invalidate_neighbor = [&](int nx, int ny, int nz) {
                            auto* n_chunk = level->get_chunk_by_voxel(nx, ny, nz);
                            if (n_chunk) {
                                n_chunk->modified = true;
                                level->meshes.erase(ChunkPos{ n_chunk->x, n_chunk->y, n_chunk->z });
                            }
                        };

                        if (lx == 0) invalidate_neighbor(x - 1, y, z);
                        if (lx == Chunk::WIDTH - 1) invalidate_neighbor(x + 1, y, z);
                        if (ly == 0) invalidate_neighbor(x, y - 1, z);
                        if (ly == Chunk::HEIGHT - 1) invalidate_neighbor(x, y + 1, z);
                        if (lz == 0) invalidate_neighbor(x, y, z - 1);
                        if (lz == Chunk::DEPTH - 1) invalidate_neighbor(x, y, z + 1);
                    }

                    if (global.lighting) {
                        global.lighting->on_block_set(x, y, z, block_id);
                        global.lighting->solve();
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

void StateGame::render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    global.shader->use();
    global.shader->uniform_matrix("u_projview", camera->get_projection() * camera->get_view());
    global.shader->uniform_1f("u_gamma", 2.2f);
    global.shader->uniform_3f("u_sky_light_color", 0.2f, 0.3f, 0.4f);

    global.texture->bind();
    auto *level = global.ecs->level.get();
    for (auto &[pos, chunk] : level->chunks) {
        if (!chunk) continue;
        auto mesh_it = level->meshes.find(pos);
        if (mesh_it == level->meshes.end() || !mesh_it->second) continue;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(
            chunk->x * Chunk::WIDTH + 0.5f,
            chunk->y * Chunk::HEIGHT + 0.5f,
            chunk->z * Chunk::DEPTH + 0.5f));
        
        global.shader->uniform_matrix("u_model", model);
        mesh_it->second->draw(GL_TRIANGLES);
    }

    for (std::size_t i = 0; i < global.ecs->size; i++) {
        auto current_id = static_cast<EntityId>(i);
        if (has_spawned && current_id == local_player_id) continue;

        ECS::Object obj { global.ecs.get(), current_id };
        if (!obj.is_valid()) continue; 

        if (!obj.has<RenderComponent>() || !obj.has<TransformComponent>()) continue;

        auto &render_comp = obj.get<RenderComponent>();
        auto &trans_comp = obj.get<TransformComponent>();

        if (!level->get_chunk_by_voxel(
                static_cast<int>(std::floor(trans_comp.position.x)),
                static_cast<int>(std::floor(trans_comp.position.y)),
                static_cast<int>(std::floor(trans_comp.position.z)))) {
            continue; 
        }

        if (!render_comp.visible || !render_comp.model) continue;

        glm::vec3 aabb_size = render_comp.model->tight_aabb.get_size();
        float scale_factor = 1.8f / std::max(aabb_size.y, 0.001f);

        glm::vec3 feet_pivot = glm::vec3(
            render_comp.model->tight_aabb.min_bounds.x + aabb_size.x * 0.5f,
            render_comp.model->tight_aabb.min_bounds.y, 
            render_comp.model->tight_aabb.min_bounds.z + aabb_size.z * 0.5f
        );

        glm::mat4 model_mat = glm::translate(glm::mat4(1.0f), trans_comp.position);
        model_mat = glm::rotate(model_mat, trans_comp.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model_mat = glm::scale(model_mat, glm::vec3(scale_factor));
        model_mat = glm::translate(model_mat, -feet_pivot); 

        global.shader->uniform_matrix("u_model", model_mat);
        render_comp.model->draw();
    }

    global.crosshair_shader->use();
    global.crosshair->draw(GL_LINES);

    global.lines_shader->use();
    global.lines_shader->uniform_matrix("u_projview", camera->get_projection() * camera->get_view());
    glLineWidth(2.0f);
    global.line_batch->render();
}

void StateGame::send_player_state() {
    if (!server_conn || !server_conn->is_open() || !has_spawned) return;

    auto *keyboard = global.window->get_keyboard();
    bool is_grabbed = global.window->grabbed;

    auto &pos = player.transform().position;
    uint8_t current_block = global.ecs->level ? global.ecs->level->get_voxel_id(
        static_cast<int>(std::floor(pos.x)),
        static_cast<int>(std::floor(pos.y)),
        static_cast<int>(std::floor(pos.z))) : 0;
    
    bool in_fluid = (current_block == BlockId::WATER || current_block == BlockId::LAVA);
    bool space_down = is_grabbed && keyboard->keys[GLFW_KEY_SPACE].down;

    Packet packet(PacketType::PlayerState);
    packet.write<bool>(is_grabbed && keyboard->keys[GLFW_KEY_W].down);
    packet.write<bool>(is_grabbed && keyboard->keys[GLFW_KEY_S].down);
    packet.write<bool>(is_grabbed && keyboard->keys[GLFW_KEY_A].down);
    packet.write<bool>(is_grabbed && keyboard->keys[GLFW_KEY_D].down);
    packet.write<bool>(space_down);
    packet.write<bool>(is_grabbed && keyboard->keys[GLFW_KEY_LEFT_CONTROL].down);
    packet.write<bool>(is_grabbed && keyboard->keys[GLFW_KEY_LEFT_SHIFT].down);

    packet.write<bool>(in_fluid && space_down);
    packet.write<float>(cam_x);
    packet.write<float>(cam_y);

    server_conn->send_packet(packet);
}

void StateGame::tick() {
    auto *wnd = global.window.get();
    wnd->get_mouse()->tick();
    wnd->get_keyboard()->tick();
    
    send_player_state();
}

void StateGame::update() {
    process_network_packets();

    auto *wnd = global.window.get();
    auto *keyboard = wnd->get_keyboard();
    auto *mouse = wnd->get_mouse();
    auto *level = global.ecs->level.get();

    if (keyboard->keys[GLFW_KEY_ESCAPE].pressed) wnd->set_should_close(true);
    
    if (keyboard->keys[GLFW_KEY_E].pressed) {
        show_inventory = !show_inventory;
        wnd->set_grabbed(!show_inventory);
    }

    if (keyboard->keys[GLFW_KEY_TAB].pressed) {
        wnd->set_grabbed(!global.window->grabbed);
        if (!global.window->grabbed) show_inventory = false;
    }

    if (player.has<InventoryComponent>()) {
        auto &inv = player.get<InventoryComponent>();
        for (int i = 0; i < static_cast<int>(HOTBAR_SIZE); i++) {
            if (keyboard->keys[GLFW_KEY_1 + i].pressed) {
                inv.selected_slot = static_cast<uint8_t>(i);
            }
        }
    }

    float dt = static_cast<float>(wnd->frame_delta) / 1'000'000'000.0f;
    dt = std::min<float>(dt, 0.05f);

    if (block_action_cooldown > 0.0f) {
        block_action_cooldown -= dt;
    }

    float lerp_factor = std::min(20.0f * dt, 1.0f);
    player.transform().position = glm::mix(player.transform().position, target_position, lerp_factor);

    auto &player_pos = player.transform().position;
    camera->position = player_pos + glm::vec3(0.0f, 1.6f, 0.0f);

    std::vector<glm::vec3> player_positions = { camera->position };

    auto current_chunk_x = static_cast<int>(std::floor(camera->position.x / Chunk::WIDTH));
    auto current_chunk_y = static_cast<int>(std::floor(camera->position.y / Chunk::HEIGHT));
    auto current_chunk_z = static_cast<int>(std::floor(camera->position.z / Chunk::DEPTH));

    level->ensure_loaded_around(
        current_chunk_x, current_chunk_y, current_chunk_z,
        2, global.world_files.get()
    );

    level->decorate_visible(player_positions);
    level->build_meshes(renderer.get(), player_positions);

    if (wnd->grabbed && !show_inventory) {
        float sensitivity = 1.5f;
        cam_x += (-mouse->delta.x / wnd->get_size().x) * sensitivity;
        cam_y += (-mouse->delta.y / wnd->get_size().y) * sensitivity;
        cam_y = glm::clamp(cam_y, -glm::half_pi<float>() + 0.1f, glm::half_pi<float>() - 0.1f);

        camera->rotation = glm::mat4(1.0f);
        camera->rotate(cam_y, cam_x, 0);

        if (auto hit = level->raycast(camera->position, camera->front, 10.0f)) {
            global.line_batch->box(
                hit->voxel_pos.x + 0.5f, hit->voxel_pos.y + 0.5f, hit->voxel_pos.z + 0.5f,
                1.005f, 1.005f, 1.005f, 0.0f, 0.0f, 0.0f, 0.5f);

            if (block_action_cooldown <= 0.0f) {
                if (mouse->buttons[GLFW_MOUSE_BUTTON_1].down) { 
                    uint8_t broken_block = hit->block_id;

                    Packet packet(PacketType::BlockModify);
                    packet.write<int32_t>(hit->voxel_pos.x);
                    packet.write<int32_t>(hit->voxel_pos.y);
                    packet.write<int32_t>(hit->voxel_pos.z);
                    packet.write<uint8_t>(0); // AIR
                    server_conn->send_packet(packet);

                    if (player.has<InventoryComponent>()) {
                        player.get<InventoryComponent>().add_item(broken_block, 1);
                    }

                    block_action_cooldown = 0.2f;
                } else if (mouse->buttons[GLFW_MOUSE_BUTTON_2].down) { 
                    if (player.has<InventoryComponent>()) {
                        auto &inv = player.get<InventoryComponent>();
                        const auto &active = inv.active_item();

                        if (active.id != 0 && active.count > 0) {
                            glm::ivec3 target_pos = hit->voxel_pos + hit->normal;

                            Packet packet(PacketType::BlockModify);
                            packet.write<int32_t>(target_pos.x);
                            packet.write<int32_t>(target_pos.y);
                            packet.write<int32_t>(target_pos.z);
                            packet.write<uint8_t>(active.id);
                            server_conn->send_packet(packet);

                            inv.consume_active_item();
                            block_action_cooldown = 0.2f;
                        }
                    }
                }
            }
        }
    }
}

struct ImUV {
    ImVec2 uv0;
    ImVec2 uv1;
};

static ImUV get_block_gui_uvs(uint32_t tex_idx) {
    constexpr float UV_TILE_SIZE = 1.0f / 16.0f;

    uint32_t col = tex_idx % 16;
    uint32_t row = tex_idx / 16;

    float u0 = col * UV_TILE_SIZE;
    float u1 = (col + 1) * UV_TILE_SIZE;

    float v0 = 1.0f - row * UV_TILE_SIZE;
    float v1 = 1.0f - (row + 1) * UV_TILE_SIZE;

    return { ImVec2(u0, v0), ImVec2(u1, v1) };
}

static bool draw_inventory_slot(
    const char* str_id,
    const ItemStack &slot,
    bool is_selected,
    ImTextureID atlas_tex_id
) {
    ImGui::PushID(str_id);

    if (is_selected) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    }

    ImVec2 image_size(32.0f, 32.0f);
    bool clicked = false;

    if (slot.id != 0 && global.blocks[slot.id] != nullptr) {
        uint32_t tex_idx = global.blocks[slot.id]->texture_faces[0];
        ImUV uvs = get_block_gui_uvs(tex_idx);

        clicked = ImGui::ImageButton(
            str_id,
            atlas_tex_id,
            image_size,
            uvs.uv0,
            uvs.uv1,
            ImVec4(0.15f, 0.15f, 0.15f, 0.6f), 
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f)   
        );

        if (slot.count > 1) {
            ImVec2 rect_max = ImGui::GetItemRectMax();
            std::string count_str = std::to_string(slot.count);
            ImVec2 text_size = ImGui::CalcTextSize(count_str.c_str());
            ImVec2 text_pos = ImVec2(rect_max.x - text_size.x - 4.0f, rect_max.y - text_size.y - 2.0f);

            ImGui::GetWindowDrawList()->AddText(
                ImVec2(text_pos.x + 1.0f, text_pos.y + 1.0f), 
                IM_COL32(0, 0, 0, 255), 
                count_str.c_str()
            );
            ImGui::GetWindowDrawList()->AddText(
                text_pos, 
                IM_COL32(255, 255, 255, 255), 
                count_str.c_str()
            );
        }
    } else {
        clicked = ImGui::Button("", ImVec2(40.0f, 40.0f));
    }

    if (ImGui::IsItemHovered() && slot.id != 0 && global.blocks[slot.id] != nullptr) {
        ImGui::SetTooltip("Block ID: %d\nCount: %d", slot.id, slot.count);
    }

    if (is_selected) {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    ImGui::PopID();
    return clicked;
}

void StateGame::render_ui() {
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiIO &io = ImGui::GetIO();

    if (player.has<InventoryComponent>()) {
        auto &inv = player.get<InventoryComponent>();

        ImTextureID atlas_tex_id = 0;
        if (global.texture) {
            atlas_tex_id = static_cast<ImTextureID>(
                static_cast<intptr_t>(global.texture->get_handle()));
        }

        float hotbar_width = HOTBAR_SIZE * 48.0f + 30.0f;
        ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - hotbar_width) * 0.5f, io.DisplaySize.y - 70.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(hotbar_width, 64.0f));

        ImGuiWindowFlags hud_flags = ImGuiWindowFlags_NoDecoration | 
            ImGuiWindowFlags_AlwaysAutoResize | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("Hotbar HUD", nullptr, hud_flags)) {
            for (std::size_t i = 0; i < HOTBAR_SIZE; ++i) {
                if (i > 0) ImGui::SameLine();

                std::string slot_id = "hb_" + std::to_string(i);
                if (draw_inventory_slot(slot_id.c_str(), inv.slots[i], inv.selected_slot == i, atlas_tex_id)) {
                    inv.selected_slot = static_cast<uint8_t>(i);
                }
            }
        }
        ImGui::End();

        if (show_inventory) {
            static int dragged_slot = -1;

            ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - 490.0f) * 0.5f, (io.DisplaySize.y - 360.0f) * 0.5f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(490.0f, 360.0f));

            if (ImGui::Begin("Player Inventory", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
                ImGui::Text("Main Inventory");
                ImGui::Separator();

                constexpr int COLUMNS = 9;
                for (std::size_t i = HOTBAR_SIZE; i < INVENTORY_SIZE; ++i) {
                    int grid_index = static_cast<int>(i - HOTBAR_SIZE);
                    if (grid_index % COLUMNS != 0) ImGui::SameLine();

                    std::string slot_id = "inv_" + std::to_string(i);
                    bool is_dragged = (dragged_slot == static_cast<int>(i));

                    if (draw_inventory_slot(slot_id.c_str(), inv.slots[i], is_dragged, atlas_tex_id)) {
                        if (dragged_slot == -1) {
                            if (inv.slots[i].id != 0) dragged_slot = static_cast<int>(i);
                        } else {
                            inv.swap_slots(static_cast<std::size_t>(dragged_slot), i);
                            dragged_slot = -1;
                        }
                    }
                }

                ImGui::Spacing();
                ImGui::Text("Hotbar Quick-Move");
                ImGui::Separator();

                for (std::size_t i = 0; i < HOTBAR_SIZE; ++i) {
                    if (i > 0) ImGui::SameLine();

                    std::string slot_id = "inv_hb_" + std::to_string(i);
                    bool is_dragged = (dragged_slot == static_cast<int>(i));

                    if (draw_inventory_slot(slot_id.c_str(), inv.slots[i], is_dragged, atlas_tex_id)) {
                        if (dragged_slot == -1) {
                            if (inv.slots[i].id != 0) dragged_slot = static_cast<int>(i);
                        } else {
                            inv.swap_slots(static_cast<std::size_t>(dragged_slot), i);
                            dragged_slot = -1;
                        }
                    }
                }

                if (dragged_slot != -1) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Slot %d selected. Click another slot to swap.", dragged_slot);
                }
            }
            ImGui::End();
        }
    }

    ImGui::SetNextWindowPos(ImVec2(10.0f, 350.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f); 

    ImGuiWindowFlags window_flags = 
        ImGuiWindowFlags_NoDecoration | 
        ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoSavedSettings | 
        ImGuiWindowFlags_NoFocusOnAppearing | 
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("Performance Overlay", nullptr, window_flags)) {
        ImGui::Text("FPS: %llu", global.window->fps);
        ImGui::Text("TPS: %llu", global.window->tps);
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void StateGame::destroy() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (global.world_files) {
        global.world_files->write();
    }

    if (server_conn && server_conn->is_open() && has_spawned) {
        Packet despawn_packet(PacketType::EntityDespawn);
        despawn_packet.write<uint32_t>(local_player_id);
        server_conn->send_packet(despawn_packet);

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        server_conn->close();
    }
}