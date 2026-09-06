#include "StateGame.hpp"
#include "Global.hpp"
#include "Block.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Mesh.hpp"
#include "LineBatch.hpp"
#include "Window.hpp"
#include "Level.hpp"
#include "Lighting.hpp"
#include "WorldFiles.hpp"
#include "WorldGenerator.hpp"
#include "Player.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <algorithm>
#include <iostream>
#include <array>

constexpr std::array<float, 8> vertices = {
    -0.01f, -0.01f,
     0.01f,  0.01f,
    -0.01f,  0.01f,
     0.01f, -0.01f,
};

constexpr std::array<int, 2> attrs = { 2, 0 };

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
    global.ecs->register_type<TransformComponent>();
    global.ecs->register_type<HitboxComponent>();
    global.ecs->register_type<PlayerInputComponent>();
    global.ecs->register_type<RenderComponent>();

    global.generator = std::make_unique<WorldGenerator>();
    global.world_files = std::make_unique<WorldFiles>("world/", REGION_VOL * (Chunk::VOLUME * 2 + 8));

    global.ecs->level = std::make_unique<Level>(16, 4, 16, -8, -2, -8);
    renderer = std::make_unique<VoxelRenderer>(1024 * 1024);
    global.line_batch = std::make_unique<LineBatch>(4096);
    global.lighting = std::make_unique<Lighting>();

    global.crosshair = std::make_unique<Mesh>(vertices, attrs);
    camera = std::make_unique<Camera>(glm::vec3(32, 120.5f, 32), glm::radians(90.0f));
    player = Player::create(glm::vec3(32, 122.0f, 32));

    request_entity_spawn(glm::vec3(32.0f, 122.0f, 32.0f));

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

                if (spawned_id >= global.ecs->size) {
                    global.ecs->resize(spawned_id + 16);
                }

                if (!has_spawned) {
                    local_player_id = spawned_id;
                    has_spawned = true;

                    player = Player(global.ecs.get(), static_cast<EntityId>(spawned_id));
                    
                    if (!player.has<TransformComponent>()) {
                        player.add<TransformComponent>();
                    }
                    if (!player.has<HitboxComponent>()) {
                        auto &hb = player.add<HitboxComponent>();
                        hb.halfsize = glm::vec3(0.3f, 0.9f, 0.3f);
                    }
                    if (!player.has<PlayerInputComponent>()) {
                        player.add<PlayerInputComponent>();
                    }

                    if (player.has<RenderComponent>()) {
                        player.remove<RenderComponent>();
                    }

                    target_position = initial_pos; 
                    player.transform().position = target_position;
                    player.hitbox().position = target_position + glm::vec3(0.0f, player.hitbox().halfsize.y, 0.0f);
                } else if (spawned_id != local_player_id) {
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
                    if (global.lighting) {
                        global.lighting->on_block_set(x, y, z, block_id);
                    }
                    int cx = static_cast<int>(std::floor(static_cast<float>(x) / Chunk::WIDTH));
                    int cy = static_cast<int>(std::floor(static_cast<float>(y) / Chunk::HEIGHT));
                    int cz = static_cast<int>(std::floor(static_cast<float>(z) / Chunk::DEPTH));
                    
                    auto* chunk = global.ecs->level->get_chunk(cx, cy, cz);
                    if (chunk) chunk->modified = true;
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
    for (std::size_t i = 0; i < level->volume; i++) {
        auto *chunk = level->chunks[i].get();
        auto *mesh = level->meshes[i].get();
        if (!chunk || !mesh) continue;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(
            chunk->x * Chunk::WIDTH + 0.5f,
            chunk->y * Chunk::HEIGHT + 0.5f,
            chunk->z * Chunk::DEPTH + 0.5f));
        
        global.shader->uniform_matrix("u_model", model);
        mesh->draw(GL_TRIANGLES);
    }

    for (std::size_t i = 0; i < global.ecs->size; i++) {
        auto current_id = static_cast<EntityId>(i);
        if (current_id == local_player_id) continue;

        ECS::Object obj { global.ecs.get(), current_id };
        if (!obj.has<RenderComponent>() || !obj.has<TransformComponent>()) continue;

        auto &render_comp = obj.get<RenderComponent>();
        auto &trans_comp = obj.get<TransformComponent>();

        if (!render_comp.visible || !render_comp.model) continue;

        glm::vec3 model_center = render_comp.model->tight_aabb.get_size() * 0.5f;
        glm::mat4 model_mat = glm::translate(glm::mat4(1.0f), trans_comp.position);
        
        model_mat = glm::rotate(model_mat, trans_comp.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model_mat = glm::rotate(model_mat, trans_comp.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        
        model_mat = glm::scale(model_mat, glm::vec3(0.1f));
        model_mat = glm::translate(model_mat, -model_center); 

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

    Packet packet(PacketType::PlayerState);
    packet.write<bool>(is_grabbed && keyboard->keys[GLFW_KEY_W].down);
    packet.write<bool>(is_grabbed && keyboard->keys[GLFW_KEY_S].down);
    packet.write<bool>(is_grabbed && keyboard->keys[GLFW_KEY_A].down);
    packet.write<bool>(is_grabbed && keyboard->keys[GLFW_KEY_D].down);
    packet.write<bool>(is_grabbed && keyboard->keys[GLFW_KEY_SPACE].down);
    packet.write<bool>(is_grabbed && keyboard->keys[GLFW_KEY_LEFT_CONTROL].down);
    packet.write<bool>(is_grabbed && keyboard->keys[GLFW_KEY_LEFT_SHIFT].down);
    packet.write<bool>(false);

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

    auto dt = static_cast<float>(wnd->frame_delta) / 1'000'000'000.0f;
    dt = std::min<float>(dt, 0.05f);

    auto *keyboard = wnd->get_keyboard();
    auto *mouse = wnd->get_mouse();
    auto *level = global.ecs->level.get();

    if (keyboard->keys[GLFW_KEY_ESCAPE].pressed) wnd->set_should_close(true);
    if (keyboard->keys[GLFW_KEY_TAB].pressed) wnd->set_grabbed(!global.window->grabbed);

    float lerp_factor = 20.0f * dt;
    player.transform().position = glm::mix(player.transform().position, target_position, lerp_factor);

    auto &player_pos = player.transform().position;
    camera->position = player_pos + glm::vec3(0.0f, 1.6f, 0.0f);

    level->set_center(
        static_cast<int>(std::floor(camera->position.x / Chunk::WIDTH)),
        static_cast<int>(std::floor(camera->position.y / Chunk::HEIGHT)),
        static_cast<int>(std::floor(camera->position.z / Chunk::DEPTH)));

    level->load_visible(global.world_files.get());
    level->decorate_visible();
    level->build_meshes(renderer.get());

    if (wnd->grabbed) {
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

            if (mouse->buttons[GLFW_MOUSE_BUTTON_1].pressed || mouse->buttons[GLFW_MOUSE_BUTTON_2].pressed) {
                glm::ivec3 target_pos = mouse->buttons[GLFW_MOUSE_BUTTON_1].pressed ? hit->voxel_pos : (hit->voxel_pos + hit->normal);
                uint8_t target_block = mouse->buttons[GLFW_MOUSE_BUTTON_1].pressed ? 0 : static_cast<uint8_t>(choosen_block);

                Packet packet(PacketType::BlockModify);
                packet.write<int32_t>(target_pos.x);
                packet.write<int32_t>(target_pos.y);
                packet.write<int32_t>(target_pos.z);
                packet.write<uint8_t>(target_block);
                server_conn->send_packet(packet);
            }
        }
    }
}

void StateGame::render_ui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_FirstUseEver);

    ImGui::Begin("Game Options");

    ImGui::Text("Selected Block ID: %d", choosen_block);
    ImGui::Separator();

    if (ImGui::BeginListBox("Select Block", ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing()))) {
        for (int i = 0; i < static_cast<int>(block_id_to_str.size()); i++) {
            const bool is_selected = (choosen_block == i);

            if (ImGui::Selectable(block_id_to_str[i].c_str(), is_selected)) {
                choosen_block = i;
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }
    ImGui::End();

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

    auto *level = global.ecs ? global.ecs->level.get() : nullptr;
    if (level && global.world_files) {
        for (unsigned int i = 0; i < level->volume; i++) {
            auto *chunk = level->chunks[i].get();
            if (!chunk) continue;

            std::span<const uint8_t> voxel_span {
                reinterpret_cast<const uint8_t*>(chunk->voxels.get()), Chunk::VOLUME };
            global.world_files->put(voxel_span, chunk->x, chunk->y, chunk->z);
        }
        global.world_files->write();
    }
}