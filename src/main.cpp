#define STB_IMAGE_IMPLEMENTATION
#include "global.hpp"
#include "gfx/shader.hpp"
#include "gfx/texture.hpp"
#include "gfx/vao.hpp"
#include "gfx/vbo.hpp"
#include "gfx/mesh.hpp"
#include "gfx/voxel_renderer.hpp"
#include "gfx/line_batch.hpp"
#include "window/window.hpp"
#include "window/camera.hpp"
#include "voxels/chunk.hpp"
#include "voxels/chunks.hpp"
#include "std.hpp"
#include "util/file.hpp"

Global global;

constexpr auto WIDTH = 1280;
constexpr auto HEIGHT = 720;

constexpr f32 vertices[] = {
    -0.01f, -0.01f,
    0.01f, 0.01f,

    -0.01f, 0.01f,
    0.01f, -0.01f,
    //-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	//1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	//-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,

	//1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	//1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	//-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
};

//constexpr std::array<int, 3> attrs = { 3, 2, 1 };

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

    auto chunks = std::make_unique<Chunks>(8, 1, 8);
    std::vector<std::unique_ptr<Mesh>> meshes(chunks->volume);
    for (usize i = 0; i < chunks->volume; i++) {
        meshes[i] = nullptr;
    }
	
	auto renderer = std::make_unique<VoxelRenderer>(1024 * 1024 * 8);
    auto line_batch = std::make_unique<LineBatch>(4096);

	glClearColor(0.6f, 0.62f, 0.65f, 1);

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto crosshair = std::make_unique<Mesh>(vertices, attrs);

	auto camera = std::make_unique<Camera>(
		glm::vec3(0, 0, 20), glm::radians(90.0f));
	
	//glm::mat4 model(1.0f);
	//model = glm::translate(model, glm::vec3(-8.0f, -8.0f, -8.0f));

	wnd->last_frame = global.time->now();
	wnd->frame_delta = 0.0f;

	float cam_x = glm::radians(-90.0f);
	float cam_y = glm::radians(-40.0f);

	float speed = 5;

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
            file::write_binary_file("world.bin", reinterpret_cast<const char*>(
                buffer.get()), chunks->volume * Chunk::VOLUME);
            std::cout << "world saved in " << (chunks->volume * Chunk::VOLUME) 
                << " bytes" << std::endl;
        }

        if (keyboard->keys[GLFW_KEY_F2].pressed) {
            auto buffer = std::make_unique<unsigned char[]>(
                chunks->volume * Chunk::VOLUME);
            file::read_binary_file("world.bin", reinterpret_cast<char *>(
                buffer.get()), chunks->volume * Chunk::VOLUME);
            chunks->read(buffer.get());
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
                if (mouse->buttons[GLFW_MOUSE_BUTTON_1].pressed) {
                    chunks->set(static_cast<int>(iend.x), static_cast<int>(iend.y),
                        static_cast<int>(iend.z), 0);
                }
                if (mouse->buttons[GLFW_MOUSE_BUTTON_2].pressed) {
                    chunks->set(static_cast<int>(iend.x) + static_cast<int>(norm.x),
                        static_cast<int>(iend.y) + static_cast<int>(norm.y),
                        static_cast<int>(iend.z) + static_cast<int>(norm.z), 2);
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