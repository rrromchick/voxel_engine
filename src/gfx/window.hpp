#pragma once

#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include "typedefs.hpp"
#include "util/math.hpp"
#include "util/time.hpp"
#include <array>
#include <functional>
#include <memory>
#include <string>

struct Button {
	bool down, last, last_tick, pressed, pressed_tick;

	Button() = default;
	~Button() = default;

	Button(Button &other) = delete;
	Button(Button &&other) = default;
	Button &operator=(Button &other) = delete;
	Button &operator=(Button &&other) = default;
};

struct Mouse {
	std::array<Button, GLFW_MOUSE_BUTTON_LAST> buttons;
	glm::vec2 position, delta;

	Mouse() = default;
	~Mouse() = default;
	
	Mouse(Mouse &other) = delete;
	Mouse(Mouse &&other) = default;
	Mouse &operator=(Mouse &other) = delete;
	Mouse &operator=(Mouse &&other) = default;

	inline void tick() {
		for (usize i = 0; i < buttons.size(); i++) {
			buttons[i].pressed_tick = buttons[i].down && !buttons[i].last_tick;
			buttons[i].last_tick = buttons[i].down;
		}
	}

	inline void update() {
		for (usize i = 0; i < buttons.size(); i++) {
			buttons[i].pressed = buttons[i].down && !buttons[i].last;
			buttons[i].last = buttons[i].down;
		}
	}
};

struct Keyboard {
	std::array<Button, GLFW_KEY_LAST> keys;

	Keyboard() = default;
	~Keyboard() = default;

	Keyboard(Keyboard &other) = delete;
	Keyboard(Keyboard &&other) = default;
	Keyboard &operator=(Keyboard &other) = delete;
	Keyboard &operator=(Keyboard &&other) = default;

	inline void tick() {
		for (usize i = 0; i < keys.size(); i++) {
			keys[i].pressed_tick = keys[i].down && !keys[i].last_tick;
			keys[i].last_tick = keys[i].down;
		}
	}

	inline void update() {
		for (usize i = 0; i < keys.size(); i++) {
			keys[i].pressed = keys[i].down && !keys[i].last;
			keys[i].last = keys[i].down;
		}
	}
};

struct Window {
	u64 last_second;
	u64 frames, fps, last_frame, frame_delta;
	u64 ticks, tps, tick_remainder;

	explicit Window(glm::ivec2 sz, const std::string &title)
		: sz(sz), title(std::move(title)) {
		this->last_frame = time::now();
		this->last_second = time::now();

		if (!glfwInit()) {
			std::fprintf(stderr, "%s", "error initializing GLFW\n");
			std::exit(1);
		}

		glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_CORE_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

		this->handle.reset(glfwCreateWindow(
			sz.x, sz.y, title.c_str(), nullptr, nullptr));
		if (this->handle == nullptr) {
			std::fprintf(stderr, "%s", "error creating window\n");
			glfwTerminate();
			std::exit(1);
		}

		glfwSetWindowUserPointer(this->handle.get(), this);

		glfwMakeContextCurrent(this->handle.get());

		glfwSetFramebufferSizeCallback(this->handle.get(), size_callback);
		glfwSetCursorPosCallback(this->handle.get(), cursor_callback);
		glfwSetKeyCallback(this->handle.get(), key_callback);
		glfwSetMouseButtonCallback(this->handle.get(), mouse_callback);
	
		if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
			std::fprintf(stderr, "%s", "error initializing GLAD\n");
			glfwTerminate();
			std::exit(1);
		}

		glfwSwapInterval(1);
	}

	~Window() {
		glfwTerminate();
	}

	Window(Window &other) = delete;
	Window &operator=(Window &other) = delete;

	Window(Window &&other)
		: handle(std::move(other.handle)),
		mouse(std::move(other.mouse)),
		keyboard(std::move(other.keyboard)),
		title(std::move(other.title)),
		sz(other.sz) {
		if (this->handle) {
			glfwSetWindowUserPointer(this->handle.get(), this);
		}
		other.sz = glm::ivec2(0, 0);
	}

	Window &operator=(Window &&other) {
		assert(this != &other);
		this->handle = std::move(other.handle);
		this->mouse = std::move(other.mouse);
		this->keyboard = std::move(other.keyboard);
		this->title = std::move(other.title);
		this->sz = other.sz;

		if (this->handle) {
			glfwSetWindowUserPointer(this->handle.get(), this);
		}

		other.sz = glm::ivec2(0, 0);
		return *this;
	}

	inline void tick() {
		this->ticks++;
		this->mouse.tick();
		this->keyboard.tick();
	}

	inline void update() {
		this->mouse.update();
		this->keyboard.update();

		this->mouse.delta = glm::ivec2(0);
	}

	inline void render() {
		this->frames++;
	}

	inline GLFWwindow *get_handle() const {
		return this->handle.get();
	}

	inline glm::ivec2 size() const {
		return this->sz;
	}

	inline void loop() {
		while (!glfwWindowShouldClose(handle.get())) {
			const u64 now = time::now();

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
		
	inline void set_grabbed(bool grabbed) {
		glfwSetInputMode(this->handle.get(), GLFW_CURSOR, grabbed ?
			GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}

	inline const Mouse &get_mouse() const {
		return this->mouse;
	}

	inline const Keyboard &get_keyboard() const {
		return this->keyboard;
	}

private:
	static inline void size_callback(GLFWwindow *handle, int width, int height) {
		auto *wnd = static_cast<Window *>(glfwGetWindowUserPointer(handle));
		glViewport(0, 0, width, height);
		wnd->sz = glm::ivec2(width, height);
	}

	static inline void cursor_callback(GLFWwindow *handle, double xp, double yp) {
		glm::vec2 p(xp, yp);

		auto *wnd = static_cast<Window *>(glfwGetWindowUserPointer(handle));

		wnd->mouse.delta = p - wnd->mouse.position;
		wnd->mouse.delta.x = 
			math::clamp(wnd->mouse.delta.x, -100.0f, 100.0f);
		wnd->mouse.delta.y =
			math::clamp(wnd->mouse.delta.y, -100.0f, 100.0f);

		wnd->mouse.position = p;
	}

	static inline void key_callback(
		GLFWwindow *handle, int key, int scancode, int action, int mods) {
		assert(key >= 0);

		auto *wnd = static_cast<Window *>(glfwGetWindowUserPointer(handle));

		switch (action) {
			case GLFW_PRESS:
				wnd->keyboard.keys[key].down = true;
				break;
			case GLFW_RELEASE:
				wnd->keyboard.keys[key].down = false;
				break;
			default:
				break;
		}
	}

	static inline void mouse_callback(
		GLFWwindow *handle, int button, int action, int mods) {
		assert(button >= 0);

		auto *wnd = static_cast<Window *>(glfwGetWindowUserPointer(handle));

		switch (action) {
			case GLFW_PRESS:
				wnd->mouse.buttons[button].down = true;
				break;
			case GLFW_RELEASE:
				wnd->mouse.buttons[button].down = false;
				break;
			default:
				break;
		}
	}

	struct WindowDeleter {
		void operator()(GLFWwindow *wnd) { if (wnd) glfwDestroyWindow(wnd); }
	};
	
	std::unique_ptr<GLFWwindow, WindowDeleter> handle;
	Mouse mouse;
	Keyboard keyboard;

	glm::ivec2 sz;
	std::string title;
};