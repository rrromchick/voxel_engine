#include "gfx/vao.hpp"
#include "gfx/vbo.hpp"
#include "gfx/shader.hpp"
#include "gfx/camera.hpp"
#include "state.hpp"
#include "block/block.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Window::Window(State *state)
	: state(state), mouse(), keyboard() {
	if (!glfwInit()) {
		std::fprintf(stderr, "%s", "error initializing GLFW\n");
		std::exit(1);
	}

	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	this->sz = glm::ivec2(1280, 720);
	this->handle.reset(
		glfwCreateWindow(sz.x, sz.y, "Voxel Engine", nullptr, nullptr));
	if (this->handle.get() == nullptr) {
		std::fprintf(stderr, "%s", "error creating window\n");
		glfwTerminate();
		std::exit(1);
	}

	glfwSetWindowUserPointer(this->handle.get(), this);

	glfwMakeContextCurrent(handle.get());

	glfwSetFramebufferSizeCallback(handle.get(), size_callback);
	glfwSetCursorPosCallback(handle.get(), cursor_callback);
	glfwSetKeyCallback(handle.get(), key_callback);
	glfwSetMouseButtonCallback(handle.get(), mouse_callback);

	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
		std::fprintf(stderr, "%s", "error initializing GLAD\n");
		glfwTerminate();
		std::exit(1);
	}

	glfwSwapInterval(1);
}

Window::~Window() {
	if (handle) {
		glfwSetWindowUserPointer(handle.get(), nullptr);

		glfwSetFramebufferSizeCallback(handle.get(), nullptr);
		glfwSetKeyCallback(handle.get(), nullptr);
		glfwSetCursorPosCallback(handle.get(), nullptr);
		glfwSetMouseButtonCallback(handle.get(), nullptr);

		handle.reset();
	}
}

void Window::tick() {
	this->ticks++;
	this->mouse.tick();
	this->keyboard.tick();

	state->block_atlas->tick();
	state->world->tick();

	auto world = state->world.get();

	world->set_center(
		world->pos_to_block(
		world->get_player()->get_camera()->position));
	
	if (state->window->keyboard.keys[GLFW_KEY_C].pressed_tick) {
		for (int x = 0; x < 32; x++) {
			for (int y = 0; y < 80; y++) {
				world->set_data(glm::ivec3(x, y, 4), BlockId::GLASS);
				world->set_data(glm::ivec3(x, y, 8), BlockId::LAVA);
			}
		}

		world->set_data(glm::ivec3(40, 80, 4), BlockId::ROSE);
	}
}

void Window::update() {
	this->mouse.update();
	this->keyboard.update();
	
	state->world.get()->update();

	if (state->window->keyboard.keys[GLFW_KEY_T].pressed) {
		state->wireframe = !state->wireframe;
	}

	this->mouse.delta = glm::vec2(0.0f);
}

void Window::render() {
	this->frames++;

	glClearColor(0.5f, 0.8f, 0.9f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPolygonMode(GL_FRONT_AND_BACK, state->wireframe ? GL_LINE : GL_FILL);
	state->world.get()->render();
}

void Window::loop() {
	state->shader = std::make_unique<Shader>("res/shaders/basic.vs", "res/shaders/basic.fs");
	state->block_atlas = std::make_unique<BlockAtlas>("res/images/blocks.png");

	state->world = std::make_unique<World>(this);
	state->wireframe = false;
	state->window.get()->set_grabbed(true);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	const auto pos = glm::vec3(0, 80, 0);
	state->world.get()->get_player()->get_camera()->position = pos;

	this->last_frame = ntime::now();
	this->last_second = ntime::now();
	this->tick_remainder = 0;

	while (!glfwWindowShouldClose(handle.get())) {
		const u64 now = ntime::now();

		this->frame_delta = now - last_frame;
		this->last_frame = now;

		if (now - this->last_second > NS_PER_SECOND) {
			this->fps = frames;
			this->tps = ticks;
			this->frames = 0;
			this->ticks = 0;
			this->last_second = now;

			std::printf("FPS: %lld | TPS: %lld\n", this->fps, this->tps);
		}

		const u64 NS_PER_TICK = (NS_PER_SECOND / 60);
		u64 tick_time = frame_delta + tick_remainder;
		while (tick_time > NS_PER_TICK) {
			this->tick();
			tick_time -= NS_PER_TICK;
		}
		this->tick_remainder = math::max<u64>(tick_time, 0);

		this->update();
		this->render();
		glfwSwapBuffers(this->handle.get());
		glfwPollEvents();
	}
}

int main(int argc, char *argv[]) {
	auto state = std::make_unique<State>();
		
	state->window = std::make_unique<Window>(state.get());
	state->window->loop();

	state.reset();
	glfwTerminate();
	return 0;
}