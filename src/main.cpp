#define STB_IMAGE_IMPLEMENTATION
#include "global.hpp"
#include "gfx/shader.hpp"
#include "gfx/texture.hpp"
#include "gfx/vao.hpp"
#include "gfx/vbo.hpp"
#include "window/window.hpp"
#include "window/camera.hpp"
#include "std.hpp"

Global global;

constexpr auto WIDTH = 1280;
constexpr auto HEIGHT = 720;

constexpr f32 vertices[] = {
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,

	1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
};

constexpr std::array<int, 2> attrs { 2, 0 };

int main(int argc, char *argv[]) {
	global.time = std::make_unique<Time>([]() -> u64 {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::high_resolution_clock::now().time_since_epoch())
				.count();
	});

	global.window = std::make_unique<Window>(glm::ivec2(WIDTH, HEIGHT), "Window");

	auto *wnd = global.window.get();

	std::unique_ptr<Shader> shader(Shader::load("res/shaders/main.glslv", "res/shaders/main.glslf"));
	if (shader == nullptr) {
		std::cerr << "failed to load shader" << std::endl;
		return 1;
	}

	auto texture = std::make_unique<Texture>("res/images/img.png");
	if (texture == nullptr) {
		std::cerr << "failed to load texture" << std::endl;
		return 1;
	}

	VBO vbo(GL_ARRAY_BUFFER, false);
	vbo.data(vertices, sizeof(vertices));

	VAO vao;
	GLsizei stride = 5 * sizeof(f32);

	vao.attr(vbo, 0, 3, GL_FLOAT, stride, 0 * sizeof(f32));
	vao.attr(vbo, 1, 2, GL_FLOAT, stride, 3 * sizeof(f32));

	vao.unbind();

	glClearColor(0.6f, 0.62f, 0.65f, 1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	auto camera = std::make_unique<Camera>(
		glm::vec3(0, 0, 1), glm::radians(90.0f));
	
	glm::mat4 model(1.0f);
	model = glm::translate(model, glm::vec3(0.5f, 0, 0));

	wnd->last_frame = global.time->now();
	wnd->frame_delta = 0.0f;

	f32 cam_x = 0.0f;
	f32 cam_y = 0.0f;

	f32 speed = 5;

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

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader->use();
		shader->uniform_matrix("model", model);
		shader->uniform_matrix("projview", camera->get_projection() * camera->get_view());

		glActiveTexture(GL_TEXTURE0);
		texture->bind();
		shader->uniform_1i("u_texture0", 0);

		vao.bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);
		vao.unbind();

		wnd->swap_buffers();
		mouse->clear_delta();
		wnd->poll_events();

		wnd->frames++;

		keyboard->update();
		mouse->update();
	}

	return 0;
}