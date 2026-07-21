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
#include "std.hpp"

Global global;

constexpr auto WIDTH = 1280;
constexpr auto HEIGHT = 720;

constexpr f32 vertices[] = {
    -0.01f, -0.01f,
    0.01f, 0.01f,

    -0.01f, 0.01f,
    0.01f, -0.01f,
};

constexpr std::array<int, 2> attrs = { 2, 0 };

int main(int argc, char *argv[]) {
	global.time = std::make_unique<Time>([]() -> u64 {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::high_resolution_clock::now().time_since_epoch())
				.count();
	});

	global.window = std::make_unique<Window>(glm::ivec2(WIDTH, HEIGHT), "Window");

	auto *wnd = global.window.get();

	std::unique_ptr<Shader> shader(
        Shader::load("res/shaders/main.glslv", "res/shaders/main.glslf"));
	if (shader == nullptr) {
		std::cerr << "failed to load shader" << std::endl;
		return 1;
	}

    std::unique_ptr<Shader> crosshair_shader(
        Shader::load("res/shaders/crosshair.glslv", "res/shaders/crosshair.glslf"));
    if (crosshair_shader == nullptr) {
        std::cerr << "failed to load crosshair shader" << std::endl;
        return 1;
    }

    std::unique_ptr<Shader> lines_shader(
        Shader::load("res/shaders/lines.glslv", "res/shaders/lines.glslf"));
    if (lines_shader == nullptr) {
        std::cerr << "failed to load lines shader" << std::endl;
        return 1;
    }

	auto texture = std::make_unique<Texture>("res/images/block.png");
	if (texture == nullptr) {
		std::cerr << "failed to load texture" << std::endl;
		return 1;
	}

    global.chunks = std::make_unique<Chunks>(16, 16, 16);
    auto *chunks = global.chunks.get();

    std::vector<std::unique_ptr<Mesh>> meshes(chunks->volume);
    for (usize i = 0; i < chunks->volume; i++) {
        meshes[i] = nullptr;
    }
	
	auto renderer = std::make_unique<VoxelRenderer>(1024 * 1024 * 8);
    auto line_batch = std::make_unique<LineBatch>(4096);

	glClearColor(0.0f, 0.0f, 0.1f, 1);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto crosshair = std::make_unique<Mesh>(vertices, attrs);

	auto camera = std::make_unique<Camera>(
		glm::vec3(0, 0, 20), glm::radians(90.0f));

	wnd->last_frame = global.time->now();
	wnd->frame_delta = 0.0f;

	float cam_x = glm::radians(-90.0f);
	float cam_y = glm::radians(-40.0f);

	float speed = 15;

    int choosen_block = 1;

    auto solver_r = std::make_unique<LightSolver>(0);
    auto solver_g = std::make_unique<LightSolver>(1);
    auto solver_b = std::make_unique<LightSolver>(2);
    auto solver_s = std::make_unique<LightSolver>(3);

    for (int y = 0; y < chunks->h * Chunk::HEIGHT; y++) {
        for (int z = 0; z < chunks->d * Chunk::DEPTH; z++) {
            for (int x = 0; x < chunks->w * Chunk::WIDTH; x++) {
                auto *vox = chunks->get(x, y, z);
                if (vox->id == 3) {
                    solver_r->add(x, y, z, 15);
                    solver_g->add(x, y, z, 15);
                    solver_b->add(x, y, z, 15);
                }
            }
        }
    }

    for (int z = 0; z < chunks->d * Chunk::DEPTH; z++) {
        for (int x = 0; x < chunks->w * Chunk::WIDTH; x++) {
            for (int y = chunks->h * Chunk::HEIGHT - 1; y >= 0; y--) {
                auto *vox = chunks->get(x, y, z);
                if (vox->id != 0) {
                    break;
                }
                chunks->get_chunk_by_voxel(x, y, z)->lightmap->set_s(
                    x % Chunk::WIDTH, y % Chunk::HEIGHT, z % Chunk::DEPTH, 0xF);
            }
        }
    }

    for (int z = 0; z < chunks->d * Chunk::DEPTH; z++) {
        for (int x = 0; x < chunks->w * Chunk::WIDTH; x++) {
            for (int y = chunks->h * Chunk::HEIGHT - 1; y >= 0; y--) {
                auto *vox = chunks->get(x, y, z);
                if (vox->id != 0) {
                    break;
                }

                if (chunks->get_light(x - 1, y, z, 3) == 0 ||
                    chunks->get_light(x + 1, y, z, 3) == 0 ||
                    chunks->get_light(x, y - 1, z, 3) == 0 ||
                    chunks->get_light(x, y + 1, z, 3) == 0 ||
                    chunks->get_light(x, y, z - 1, 3) == 0 ||
                    chunks->get_light(x, y, z + 1, 3) == 0) {
                    solver_s->add(x, y, z);
                }
                chunks->get_chunk_by_voxel(x, y, z)->lightmap->set_s(
                    x % Chunk::WIDTH, y % Chunk::HEIGHT, z % Chunk::DEPTH, 0xF);
            }
        }
    }

    solver_r->solve();
    solver_g->solve();
    solver_b->solve();
    solver_s->solve();

	while (!wnd->is_should_close()) {
		auto current_time = global.time->now();
		wnd->frame_delta = current_time - wnd->last_frame;
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

        const u64 NANOS_PER_TICK = (Time::NANOS_PER_SECOND / 60);
        u64 tick_time = wnd->frame_delta + wnd->tick_remainder;
        while (tick_time > NANOS_PER_TICK) {
            wnd->ticks++;
            mouse->tick();
            keyboard->tick();

            tick_time -= NANOS_PER_TICK;
        }

        wnd->tick_remainder = std::max<u64>(tick_time, 0);

        for (int i = 1; i < 4; i++) {
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

        if (keyboard->keys[GLFW_KEY_F1].pressed) {
            auto buffer = std::make_unique<unsigned char[]>(
                chunks->volume * Chunk::VOLUME);
            chunks->write(buffer.get());
            File::write_binary_file("world.bin", reinterpret_cast<const char*>(
                buffer.get()), chunks->volume * Chunk::VOLUME);
            std::cout << "world saved in " << (chunks->volume * Chunk::VOLUME) 
                << " bytes" << std::endl;
        }

        if (keyboard->keys[GLFW_KEY_F2].pressed) {
            auto buffer = std::make_unique<unsigned char[]>(
                chunks->volume * Chunk::VOLUME);
            File::read_binary_file("world.bin", reinterpret_cast<char *>(
                buffer.get()), chunks->volume * Chunk::VOLUME);
            chunks->read(buffer.get());

            for (int y = 0; y < chunks->h * Chunk::HEIGHT; y++) {
                for (int z = 0; z < chunks->d * Chunk::DEPTH; z++) {
                    for (int x = 0; x < chunks->w * Chunk::WIDTH; x++) {
                        auto *vox = chunks->get(x, y, z);
                        if (vox->id == 3) {
                            solver_r->add(x, y, z, 15);
                            solver_g->add(x, y, z, 15);
                            solver_b->add(x, y, z, 15);
                        }
                    }
                }
            }

            solver_r->solve();
            solver_g->solve();
            solver_b->solve();
            solver_s->solve();
        }

        float dt = static_cast<float>(wnd->frame_delta) / 1'000'000'000.0f;

        if (keyboard->keys[GLFW_KEY_W].down) {
            camera->position += camera->front * dt * speed;
        }
        if (keyboard->keys[GLFW_KEY_S].down) {
            camera->position -= camera->front * dt * speed;
        }
        if (keyboard->keys[GLFW_KEY_D].down) {
            camera->position += camera->right * dt * speed;
        }
        if (keyboard->keys[GLFW_KEY_A].down) {
            camera->position -= camera->right * dt * speed;
        }

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
                line_batch->box(iend.x + 0.5f, iend.y + 0.5f, iend.z + 0.5f,
                    1.005f, 1.005f, 1.005f, 0, 0, 0, 0.5f);

                if (mouse->buttons[GLFW_MOUSE_BUTTON_1].pressed) {
                    auto x = static_cast<int>(iend.x);
                    auto y = static_cast<int>(iend.y);
                    auto z = static_cast<int>(iend.z);
                    chunks->set(x, y, z, 0);

                    solver_r->remove(x, y, z);
                    solver_g->remove(x, y, z);
                    solver_b->remove(x, y, z);

                    solver_r->solve();
                    solver_g->solve();
                    solver_b->solve();

                    if (chunks->get_light(x, y + 1, z, 3) == 0xF) {
                        for (int i = y; i >= 0; i--) {
                            if (chunks->get(x, i, z)->id != 0) {
                                break;
                            }
                            solver_s->add(x, i, z, 0xF);
                        }
                    }

                    constexpr std::array<std::array<int, 3>, 6> dirs = {{
                        { -1, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 },
                        { 0, 0, -1 }, { 0, 0, 1 }
                    }};

                    for (const auto &dir : dirs) {
                        solver_r->add(x + dir[0], y + dir[1], z + dir[2]);
                        solver_g->add(x + dir[0], y + dir[1], z + dir[2]);
                        solver_b->add(x + dir[0], y + dir[1], z + dir[2]);
                        solver_s->add(x + dir[0], y + dir[1], z + dir[2]);
                    }

                    solver_r->solve();
                    solver_g->solve();
                    solver_b->solve();
                    solver_s->solve();
                }

                if (mouse->buttons[GLFW_MOUSE_BUTTON_2].pressed) {
                    auto x = static_cast<int>(iend.x) + static_cast<int>(norm.x);
                    auto y = static_cast<int>(iend.y) + static_cast<int>(norm.y);
                    auto z = static_cast<int>(iend.z) + static_cast<int>(norm.z);
                    chunks->set(x, y, z, choosen_block);

                    solver_r->remove(x, y, z);
                    solver_g->remove(x, y, z);
                    solver_b->remove(x, y, z);
                    solver_s->remove(x, y, z);
                   
                    for (int i = y - 1; i >= 0; i--) {
                        solver_s->remove(x, i, z);
                        if (i == 0 || chunks->get(x, i - 1, z)->id != 0) {
                            break;
                        }
                    }

                    solver_r->solve();
                    solver_g->solve();
                    solver_b->solve();
                    solver_s->solve();
                    
                    if (choosen_block == 3) {
                        solver_r->add(x, y, z, 10);
                        solver_g->add(x, y, z, 10);
                        solver_b->add(x, y, z, 0);
                        solver_r->solve();
                        solver_g->solve();
                        solver_b->solve();
                    }
                }
            }
        }

        std::vector<Chunk*> closes(27);
        for (usize i = 0; i < chunks->volume; i++) {
            auto *chunk = chunks->chunks[i].get();
            if (!chunk->modified) {
                continue;
            }
            chunk->modified = false;
            if (meshes[i] != nullptr) {
                meshes[i] = nullptr;
            }

            for (usize i = 0; i < closes.size(); i++) {
                closes[i] = nullptr;
            }

            for (usize j = 0; j < chunks->volume; j++) {
                auto *other = chunks->chunks[j].get();

                int ox = other->x - chunk->x;
                int oy = other->y - chunk->y;
                int oz = other->z - chunk->z;

                if (std::abs(ox) > 1 || std::abs(oy) > 1 || std::abs(oz) > 1) {
                    continue;
                }

                ox += 1;
                oy += 1;
                oz += 1;
                closes[(oy * 3 + oz) * 3 + ox] = other;
            }
            meshes[i] = renderer.get()->render(chunk, closes);
        }

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader->use();
		shader->uniform_matrix("projview", camera->get_projection() * camera->get_view());
		texture->bind();
        mat4 model(1.0f);
        for (usize i = 0; i < chunks->volume; i++) {
            auto *chunk = chunks->chunks[i].get();
            auto *mesh = meshes[i].get();

            if (mesh == nullptr) {
                continue;
            }

            model = glm::translate(mat4(1.0f), vec3(chunk->x * Chunk::WIDTH + 0.5f,
                chunk->y * Chunk::HEIGHT + 0.5f, chunk->z * Chunk::DEPTH + 0.5f));
            shader->uniform_matrix("model", model);
            mesh->draw(GL_TRIANGLES);
        }

        crosshair_shader->use();
        crosshair->draw(GL_LINES);

        lines_shader->use();
        lines_shader->uniform_matrix("projview", camera->get_projection() * camera->get_view());
        glLineWidth(2.0f);
        line_batch->render();

		wnd->swap_buffers();
		mouse->clear_delta();
		wnd->poll_events();

		wnd->frames++;

		keyboard->update();
		mouse->update();
	}

	return 0;
}