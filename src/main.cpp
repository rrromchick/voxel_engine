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

    // AIR
    global.blocks[0] = std::make_unique<Block>(0, 0);
    global.blocks[0].get()->draw_group = 1;
    global.blocks[0].get()->light_passing = true;

    // STONE
    global.blocks[1] = std::make_unique<Block>(1, 2);
    
    // GRASS
    global.blocks[2] = std::make_unique<Block>(2, 4);
    global.blocks[2].get()->texture_faces[2] = 2;
    global.blocks[2].get()->texture_faces[3] = 1;

    // LAMP
    global.blocks[3] = std::make_unique<Block>(3, 3);
    global.blocks[3].get()->emission[0] = 10;
    global.blocks[3].get()->emission[1] = 0;
    global.blocks[3].get()->emission[2] = 0;

    // GLASS
    global.blocks[4] = std::make_unique<Block>(4, 5);
    global.blocks[4].get()->draw_group = 2;
    global.blocks[4].get()->light_passing = true;

    // PLANKS
    global.blocks[5] = std::make_unique<Block>(5, 6);

    global.generator = std::make_unique<WorldGenerator>();
    global.world_files = std::make_unique<WorldFiles>("world/", 24 * 1024 * 1024);
    global.chunks = std::make_unique<Chunks>(16 * 4, 1, 16 * 4, 0, 0, 0);
    auto *chunks = global.chunks.get();

	auto renderer = std::make_unique<VoxelRenderer>(1024 * 1024 * 8);
    auto line_batch = std::make_unique<LineBatch>(4096);

    global.lighting = std::make_unique<Lighting>();
    auto *lighting = global.lighting.get();

	glClearColor(0.0f, 0.0f, 0.0f, 1);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto crosshair = std::make_unique<Mesh>(vertices, attrs);
	auto camera = std::make_unique<Camera>(
		glm::vec3(32, 32, 32), glm::radians(90.0f));

	wnd->last_frame = global.time->now();
	wnd->frame_delta = 0.0f;

	float cam_x = 0.0f;
	float cam_y = 0.0f;

	float speed = 15;

    int choosen_block = 1;

    glfwSwapInterval(0);

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

        //if (keyboard->keys[GLFW_KEY_F1].pressed) {
        //    const std::size_t buffer_size = chunks->volume * Chunk::VOLUME;
        //    auto buffer = std::make_unique<uint8_t[]>(buffer_size);

        //    chunks->write(buffer.get());

        //    std::span<const uint8_t> buffer_span { buffer.get(), buffer_size };
        //    File::write_binary_file("world.bin", buffer_span);

        //    std::cout << "world saved in " << buffer_size << " bytes" << std::endl;
        //}

        //if (keyboard->keys[GLFW_KEY_F2].pressed) {
        //    const std::size_t buffer_size = chunks->volume * Chunk::VOLUME;
        //    auto buffer = std::make_unique<uint8_t[]>(buffer_size);

        //    std::span<uint8_t> buffer_span { buffer.get(), buffer_size };

        //    if (File::read_binary_file("world.bin", buffer_span)) {
        //        chunks->read(buffer.get());

        //        lighting->clear();
        //        lighting->on_world_loaded();
        //        std::cout << "world loaded (" << buffer_size << " bytes)" << std::endl;
        //    } else {
        //        std::cerr << "failed to load world.bin" << std::endl;
        //    }
        //}

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

        chunks->set_center(camera->position.x, 0, camera->position.z);
        chunks->build_meshes(renderer.get());
        chunks->load_visible(global.world_files.get());

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

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader->use();
		shader->uniform_matrix("projview", camera->get_projection() * camera->get_view());
		texture->bind();
        mat4 model(1.0f);
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

    for (unsigned int i = 0; i < chunks->volume; i++) {
        auto *chunk = chunks->chunks[i].get();
        if (chunk == nullptr) {
            continue;
        }

        std::span<const uint8_t> voxel_span { 
            reinterpret_cast<uint8_t*>(chunk->voxels.get()), Chunk::VOLUME };
        global.world_files->put(voxel_span, chunk->x, chunk->z);
    }

    global.world_files->write();
	return 0;
}