//#include "state_game.hpp"
//#include "window/window.hpp"
//#include "gfx/shader.hpp"
//#include "gfx/texture.hpp"
//#include "std.hpp"
//
//constexpr float vertices[] = {
//	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
//	1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
//	-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
//
//	1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
//	1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
//	-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
//};
//
//constexpr int attrs[] = {
//	3, 2, 0,
//};
//
//void StateGame::init() {
//	window = std::make_unique<Window>(
//		glm::ivec2(1280, 720), "Voxel Engine");
//
//	window->set_grabbed(true);
//
//	glEnable(GL_DEPTH_TEST);
//	glDepthFunc(GL_LESS);
//
//	glEnable(GL_CULL_FACE);
//	glCullFace(GL_BACK);
//
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//}
//
//void StateGame::destroy() {}
//
//void StateGame::tick() {
//	window->ticks++;
//	window->get_keyboard()->tick();
//	window->get_mouse()->tick();
//
//	if (window->get_keyboard()->keys[GLFW_KEY_C].pressed_tick) {
//		std::printf("Hello, world");
//	}
//}
//
//void StateGame::update() {
//	window->get_keyboard()->update();
//	window->get_mouse()->update();
//
//	if (window->get_keyboard()->keys[GLFW_KEY_T].pressed) {
//		this->wireframe = !this->wireframe;
//	}
//}
//
//void StateGame::render() {
//	window->frames++;
//
//	glClearColor(0.5f, 0.8f, 0.9f, 1.0f);
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//	glPolygonMode(GL_FRONT_AND_BACK, this->wireframe ? GL_LINE : GL_FILL);
//}
//
//void StateGame::loop() {
//	auto shader = std::make_unique<Shader>(
//		Shader::load("res/main.glslv", "res/main.glslf"));
//	if (shader == nullptr) {
//		std::cerr << "faield to load shader" << std::endl;
//		std::exit(1);
//	}
//
//	auto texture = std::make_unique<Texture>("res/block.png");
//
//	while (!glfwWindowShouldClose(window->get_handle())) {
//		const u64 now = global.time->now();
//
//		window->frame_delta = now - window->last_frame;
//		window->last_frame = now;
//
//		if (now - window->last_second > Time::NANOS_PER_SECOND) {
//			window->fps = window->frames;
//			window->tps = window->ticks;
//			window->frames = 0;
//			window->ticks = 0;
//			window->last_second = now;
//
//			std::printf("FPS: %lld | TPS: %lld\n", window->fps, window->tps);
//		}
//
//		const u64 NS_PER_TICK = (Time::NANOS_PER_SECOND / 60);
//		u64 tick_time = window->frame_delta + window->tick_remainder;
//		while (tick_time > NS_PER_TICK) {
//			tick();
//			tick_time -= NS_PER_TICK;
//		}
//		window->tick_remainder = std::max<u64>(tick_time, 0);
//
//		update();
//		render();
//		glfwSwapBuffers(window->get_handle());
//		glfwPollEvents();
//	}
//
//	destroy();
//	std::exit(0);
//}