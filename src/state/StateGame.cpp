#define STB_IMAGE_IMPLEMENTATION
#include "StateGame.hpp"
#include "Global.hpp"
#include "Block.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Mesh.hpp"
#include "LineBatch.hpp"
#include "Window.hpp"
#include "Chunks.hpp"
#include "Lighting.hpp"
#include "WorldFiles.hpp"
#include "WorldGenerator.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>
#include <array>

constexpr std::array<float, 8> vertices = {
    -0.01f, -0.01f,
    0.01f, 0.01f,
    -0.01f, 0.01f,
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

void StateGame::init() {
    setup_definitions();
    if (!init_assets()) {
        std::cerr << "Failed to initialize game assets." << std::endl;
        return;
    }

    global.generator = std::make_unique<WorldGenerator>();
    global.world_files = std::make_unique<WorldFiles>("world/", REGION_VOL * (Chunk::VOLUME * 2 + 8));
    global.chunks = std::make_unique<Chunks>(16, 4, 16, 0, 0, 0);
    
    renderer = std::make_unique<VoxelRenderer>(1024 * 1024);
    global.line_batch = std::make_unique<LineBatch>(4096);
    physics_solver = std::make_unique<PhysicsSolver>(glm::vec3(0, -16.0f, 0));
    global.lighting = std::make_unique<Lighting>();

    global.crosshair = std::make_unique<Mesh>(vertices, attrs);
    camera = std::make_unique<Camera>(glm::vec3(32, 120.5f, 32), glm::radians(90.0f));
    hitbox = std::make_unique<Hitbox>(glm::vec3(32, 120, 32), glm::vec3(0.2f, 0.9f, 0.2f));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(global.window->get_handle(), true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void StateGame::tick() {
    auto *wnd = global.window.get();
    auto *mouse = wnd->get_mouse();
    auto *keyboard = wnd->get_keyboard();
    
    mouse->tick();
    keyboard->tick();
}

void StateGame::update() {
    auto *wnd = global.window.get();
    auto *keyboard = wnd->get_keyboard();
    auto *mouse = wnd->get_mouse();
    auto *chunks = global.chunks.get();

    if (keyboard->keys[GLFW_KEY_ESCAPE].pressed) {
        wnd->set_should_close(true);
    }
    if (keyboard->keys[GLFW_KEY_TAB].pressed) {
        wnd->set_grabbed(!global.window->grabbed);
    }

    bool sprint = keyboard->keys[GLFW_KEY_LEFT_CONTROL].down;
    bool shift = keyboard->keys[GLFW_KEY_LEFT_SHIFT].down && hitbox->grounded && !sprint;

    auto speed = static_cast<float>(player_speed);
    glm::vec3 dir(0, 0, 0);

    if (keyboard->keys[GLFW_KEY_W].down) { dir.x += camera->dir.x; dir.z += camera->dir.z; }
    if (keyboard->keys[GLFW_KEY_S].down) { dir.x -= camera->dir.x; dir.z -= camera->dir.z; }
    if (keyboard->keys[GLFW_KEY_D].down) { dir.x += camera->right.x; dir.z += camera->right.z; }
    if (keyboard->keys[GLFW_KEY_A].down) { dir.x -= camera->right.x; dir.z -= camera->right.z; }

    auto delta_seconds = static_cast<float>(wnd->frame_delta) / 1'000'000'000.0f;
    delta_seconds = std::min<float>(delta_seconds, 0.05f);

    auto substeps = static_cast<int>(wnd->frame_delta * 1000);
    substeps = (substeps <= 0 ? 1 : (substeps > 100 ? 100 : substeps));
    physics_solver->step(hitbox.get(), delta_seconds, substeps, shift);

    camera->position.x = hitbox->position.x;
    camera->position.y = hitbox->position.y + 0.5f;
    camera->position.z = hitbox->position.z;

    auto dt = std::min<float>(1.0f, wnd->frame_delta * 16);
    if (shift) {
        speed *= 0.25f;
        camera->position.y -= 0.2f;
        camera->zoom = 0.9f * dt + camera->zoom * (1.0f - dt);
    } else if (sprint) {
        speed *= 5.0f;
        camera->zoom = 1.1f * dt + camera->zoom * (1.0f - dt);
    } else {
        camera->zoom = dt + camera->zoom * (1.0f - dt);
    }

    if (glm::length(dir) > 0.0f) {
        dir = glm::normalize(dir);
    }

    hitbox->velocity.x = dir.x * speed;
    hitbox->velocity.z = dir.z * speed;

    if (keyboard->keys[GLFW_KEY_SPACE].down && hitbox->grounded) {
        hitbox->velocity.y = 6.0f;
    }

    chunks->set_center(camera->position.x, 0, camera->position.z);
    chunks->load_visible(global.world_files.get());
    chunks->decorate_visible();
    chunks->build_meshes(renderer.get());

    if (wnd->grabbed) {
        cam_y += -mouse->delta.y / wnd->get_size().y * 2;
        cam_x += -mouse->delta.x / wnd->get_size().x * 2;

        cam_y = std::clamp(cam_y, -glm::radians(89.0f), glm::radians(89.0f));
        camera->rotation = glm::mat4(1.0f);
        camera->rotate(cam_y, cam_x, 0);
    }

    glm::vec3 end, norm, iend;
    auto *vox = chunks->ray_cast(camera->position, camera->front, 10.0f, end, norm, iend);
    if (vox != nullptr) {
        global.line_batch->box(iend.x + 0.5f, iend.y + 0.5f, iend.z + 0.5f,
            1.005f, 1.005f, 1.005f, 0, 0, 0, 0.5f);
            
        if (mouse->buttons[GLFW_MOUSE_BUTTON_1].pressed) {
            auto x = static_cast<int>(iend.x);
            auto y = static_cast<int>(iend.y);
            auto z = static_cast<int>(iend.z);
            chunks->set(x, y, z, 0);
            global.lighting->on_block_set(x, y, z, 0);
        }

        if (mouse->buttons[GLFW_MOUSE_BUTTON_2].pressed) {
            auto x = static_cast<int>(iend.x) + static_cast<int>(norm.x);
            auto y = static_cast<int>(iend.y) + static_cast<int>(norm.y);
            auto z = static_cast<int>(iend.z) + static_cast<int>(norm.z);
            chunks->set(x, y, z, choosen_block);
            global.lighting->on_block_set(x, y, z, choosen_block);
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

    auto *chunks = global.chunks.get();
    for (std::size_t i = 0; i < chunks->volume; i++) {
        auto *chunk = chunks->chunks[i].get();
        auto *mesh = chunks->meshes[i].get();
        if (!chunk || !mesh) continue;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(chunk->x * Chunk::WIDTH + 0.5f,
            chunk->y * Chunk::HEIGHT + 0.5f, chunk->z * Chunk::DEPTH + 0.5f));
        global.shader->uniform_matrix("u_model", model);
        mesh->draw(GL_TRIANGLES);
    }

    global.crosshair_shader->use();
    global.crosshair->draw(GL_LINES);

    global.lines_shader->use();
    global.lines_shader->uniform_matrix("u_projview", camera->get_projection() * camera->get_view());
    glLineWidth(2.0f);
    global.line_batch->render();
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

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void StateGame::destroy() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    auto *chunks = global.chunks.get();
    if (chunks && global.world_files) {
        for (unsigned int i = 0; i < chunks->volume; i++) {
            auto *chunk = chunks->chunks[i].get();
            if (!chunk) continue;

            std::span<const uint8_t> voxel_span {
                reinterpret_cast<const uint8_t*>(chunk->voxels.get()), Chunk::VOLUME };
            global.world_files->put(voxel_span, chunk->x, chunk->y, chunk->z);
        }
        global.world_files->write();
    }
}