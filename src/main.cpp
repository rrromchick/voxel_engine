#define STB_IMAGE_IMPLEMENTATION
#include "Global.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "VAO.hpp"
#include "VBO.hpp"
#include "Mesh.hpp"
#include "VoxelRenderer.hpp"
#include "LineBatch.hpp"
#include "Window.hpp"
#include "Camera.hpp"
#include "Chunk.hpp"
#include "Chunks.hpp"
#include "File.hpp"
#include "LightSolver.hpp"
#include "Lightmap.hpp"
#include "Lighting.hpp"
#include "Block.hpp"
#include "WorldFiles.hpp"
#include "WorldGenerator.hpp"
#include "Hitbox.hpp"
#include "PhysicsSolver.hpp"

Global global;

constexpr auto WIDTH = 1280;
constexpr auto HEIGHT = 720;

constexpr float vertices[] = {
    -0.01f, -0.01f,
    0.01f, 0.01f,

    -0.01f, 0.01f,
    0.01f, -0.01f,
};

constexpr std::array<int, 2> attrs = { 2, 0 };

void setup_definitions() {
    // AIR
    global.blocks[0] = std::make_unique<Block>(0, 0);
    global.blocks[0]->draw_group = 1;
    global.blocks[0]->light_passing = true;
    global.blocks[0]->obstacle = false;

    // STONE
    global.blocks[1] = std::make_unique<Block>(1, 2);
    
    // GRASS
    global.blocks[2] = std::make_unique<Block>(2, 4);
    global.blocks[2]->texture_faces[2] = 2;
    global.blocks[2]->texture_faces[3] = 1;

    // LAMP
    global.blocks[3] = std::make_unique<Block>(3, 3);
    global.blocks[3]->emission[0] = 15;
    global.blocks[3]->emission[1] = 14;
    global.blocks[3]->emission[2] = 13;

    // GLASS
    global.blocks[4] = std::make_unique<Block>(4, 5);
    global.blocks[4]->draw_group = 2;
    global.blocks[4]->light_passing = true;

    // PLANKS
    global.blocks[5] = std::make_unique<Block>(5, 6);
}

int init_assets() {
    global.shader = Shader::load("res/shaders/main.glslv", "res/shaders/main.glslf");
    if (global.shader == nullptr) {
        std::cerr << "failed to load shader" << std::endl;
        return 1;
    }

    global.crosshair_shader = Shader::load("res/shaders/crosshair.glslv", "res/shaders/crosshair.glslf");
    if (global.crosshair_shader == nullptr) {
        std::cerr << "failed to load crosshair shader" << std::endl;
        return 1;
    }

    global.lines_shader = Shader::load("res/shaders/lines.glslv", "res/shaders/lines.glslf");
    if (global.lines_shader == nullptr) {
        std::cerr << "failed to load lines shader" << std::endl;
        return 1;
    }

    global.texture = std::make_unique<Texture>("res/images/block.png");
    if (global.texture == nullptr) {
        std::cerr << "faield to load texture" << std::endl;
        return 1;
    }

    return 0;
}

void draw_world(Camera *camera) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    global.shader->use();
    global.shader->uniform_matrix("u_projview", camera->get_projection() * camera->get_view());
    global.shader->uniform_1f("u_gamma", 1.6f);
    global.shader->uniform_3f("u_sky_light_color", 0.1 * 2, 0.15 * 2, 0.2 * 2);
    global.texture->bind();
    mat4 model(1.0f);

    auto *chunks = global.chunks.get();
    for (std::size_t i = 0; i < chunks->volume; i++) {
        auto *chunk = chunks->chunks[i].get();
        if (chunk == nullptr) {
            continue;
        }

        auto *mesh = chunks->meshes[i].get();
        if (mesh == nullptr) {
            continue;
        }

        model = glm::translate(mat4(1.0f), vec3(chunk->x * Chunk::WIDTH + 0.5f,
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

void write_world() {
    auto *chunks = global.chunks.get();
    for (unsigned int i = 0; i < chunks->volume; i++) {
        auto *chunk = chunks->chunks[i].get();
        if (chunk == nullptr) {
            continue;
        }

        std::span<const uint8_t> voxel_span {
            reinterpret_cast<const uint8_t*>(chunk->voxels.get()), Chunk::VOLUME };
        global.world_files->put(voxel_span, chunk->x, chunk->z);
    }

    global.world_files->write();
}

int main(int argc, char *argv[]) {
    setup_definitions();

	global.time = std::make_unique<Time>([]() -> u64 {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::high_resolution_clock::now().time_since_epoch())
				.count();
	});

	global.window = std::make_unique<Window>(glm::ivec2(WIDTH, HEIGHT), "Voxel Engine");
	auto *wnd = global.window.get();

    auto result = init_assets();
    if (result) {
        return result;
    }

    global.generator = std::make_unique<WorldGenerator>();
    global.world_files = std::make_unique<WorldFiles>("world/", REGION_VOL * (Chunk::VOLUME * 2 + 8));
    global.chunks = std::make_unique<Chunks>(32, 1, 32, 0, 0, 0);
    auto *chunks = global.chunks.get();

	auto renderer = std::make_unique<VoxelRenderer>(1024 * 1024);
    global.line_batch = std::make_unique<LineBatch>(4096);
    
    auto physics_solver = std::make_unique<PhysicsSolver>(vec3(0, -16.0f, 0));

    global.lighting = std::make_unique<Lighting>();
    auto *lighting = global.lighting.get();

	glClearColor(0.0f, 0.0f, 0.0f, 1);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    global.crosshair = std::make_unique<Mesh>(vertices, attrs);
	auto camera = std::make_unique<Camera>(
		glm::vec3(32, 32.5f, 32), glm::radians(90.0f));
    auto hitbox = std::make_unique<Hitbox>(vec3(32, 32, 32), vec3(0.2f, 0.9f, 0.2f));

	wnd->last_frame = global.time->now();
	wnd->frame_delta = 0.0f;

	float cam_x = 0.0f;
	float cam_y = 0.0f;

	float player_speed = 4.0f;
    long frame = 0;

    int choosen_block = 1;

    glfwSwapInterval(1);

	while (!wnd->is_should_close()) {
        frame++;
		auto current_time = global.time->now();
        uint64_t raw_delta = current_time - wnd->last_frame;
		wnd->last_frame = current_time;

		if (current_time - wnd->last_second > Time::NANOS_PER_SECOND) {
			wnd->fps = wnd->frames;
			wnd->tps = wnd->ticks;
            wnd->frames = 0;
            wnd->ticks = 0;
            wnd->last_second = current_time;

            std::printf("FPS: %lld | TPS: %lld\n", wnd->fps, wnd->tps);
        }

        auto *keyboard = wnd->get_keyboard();
        auto *mouse = wnd->get_mouse();

        const uint64_t NANOS_PER_TICK = (Time::NANOS_PER_SECOND / 60);
        uint64_t tick_time = wnd->frame_delta + wnd->tick_remainder;
        while (tick_time > NANOS_PER_TICK) {
            wnd->ticks++;
            mouse->tick();
            keyboard->tick();

            tick_time -= NANOS_PER_TICK;
        }

        wnd->tick_remainder = std::max<uint64_t>(tick_time, 0);

        for (int i = 1; i < 6; i++) {
            if (keyboard->keys[GLFW_KEY_0 + i].pressed) {
                choosen_block = i;
            }
        }

        if (keyboard->keys[GLFW_KEY_ESCAPE].pressed) {
            wnd->set_should_close(true);
        }
        if (keyboard->keys[GLFW_KEY_TAB].pressed) {
            wnd->set_grabbed(!global.window->grabbed);
        }

        bool sprint = keyboard->keys[GLFW_KEY_LEFT_CONTROL].down;
        bool shift = keyboard->keys[GLFW_KEY_LEFT_SHIFT].down &&
            hitbox->grounded && !sprint;

        auto speed = player_speed;
        vec3 dir(0, 0, 0);

        if (keyboard->keys[GLFW_KEY_W].down) {
            dir.x += camera->dir.x;
            dir.z += camera->dir.z;
        }
        if (keyboard->keys[GLFW_KEY_S].down) {
            dir.x -= camera->dir.x;
            dir.z -= camera->dir.z;
        }
        if (keyboard->keys[GLFW_KEY_D].down) {
            dir.x += camera->right.x;
            dir.z += camera->right.z;
        }
        if (keyboard->keys[GLFW_KEY_A].down) {
            dir.x -= camera->right.x;
            dir.z -= camera->right.z;
        }

        float delta_seconds = static_cast<float>(raw_delta) / 1'000'000'000.0f;
        delta_seconds = std::min(delta_seconds, 0.05f);

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
            speed *= 1.5f;
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
        chunks->build_meshes(renderer.get());

        if (wnd->grabbed) {
            cam_y += -wnd->get_mouse()->delta.y / wnd->get_size().y * 2;
            cam_x += -wnd->get_mouse()->delta.x / wnd->get_size().x * 2;

            if (cam_y < -glm::radians(89.0f)) {
                cam_y = -glm::radians(89.0f);
            }
            if (cam_y > glm::radians(89.0f)) {
                cam_y = glm::radians(89.0f);
            }
            
            camera->rotation = glm::mat4(1.0f);
            camera->rotate(cam_y, cam_x, 0);
        }

        {
            vec3 end;
            vec3 norm;
            vec3 iend;
            auto *vox = chunks->ray_cast(
                camera->position, camera->front, 10.0f, end, norm, iend);
            if (vox != nullptr) {
                global.line_batch->box(iend.x + 0.5f, iend.y + 0.5f, iend.z + 0.5f,
                    1.005f, 1.005f, 1.005f, 0, 0, 0, 0.5f);

                if (mouse->buttons[GLFW_MOUSE_BUTTON_1].pressed) {
                    auto x = static_cast<int>(iend.x);
                    auto y = static_cast<int>(iend.y);
                    auto z = static_cast<int>(iend.z);
                    chunks->set(x, y, z, 0);
                    lighting->on_block_set(x, y, z, 0);
                }

                if (mouse->buttons[GLFW_MOUSE_BUTTON_2].pressed) {
                    auto x = static_cast<int>(iend.x) + static_cast<int>(norm.x);
                    auto y = static_cast<int>(iend.y) + static_cast<int>(norm.y);
                    auto z = static_cast<int>(iend.z) + static_cast<int>(norm.z);
                    chunks->set(x, y, z, choosen_block);
                    lighting->on_block_set(x, y, z, choosen_block);
                }
            }
        }

		draw_world(camera.get());

		wnd->swap_buffers();
		mouse->clear_delta();
		wnd->poll_events();

		wnd->frames++;

		keyboard->update();
		mouse->update();
	}

    write_world();
	return 0;
}