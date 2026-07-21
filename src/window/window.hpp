#pragma once

#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include "typedefs.hpp"
#include "Global.hpp"
#include "std.hpp"

struct Button {
	bool down, last, last_tick, pressed, pressed_tick;

	Button() = default;
	~Button() = default;

	Button(const Button &other) = delete;
	Button(Button &&other) = default;
	Button &operator=(const Button &other) = delete;
	Button &operator=(Button &&other) = default;
};

struct Mouse {
	std::array<Button, GLFW_MOUSE_BUTTON_LAST + 1> buttons{};
	glm::vec2 position, delta;

	Mouse() = default;
	~Mouse() = default;

	Mouse(const Mouse &other) = delete;
	Mouse(Mouse &&other) = default;
	Mouse &operator=(const Mouse &other) = delete;
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

	inline void clear_delta() {
		delta = glm::vec2(0.0f);
	}
};

struct Keyboard {
	std::array<Button, GLFW_KEY_LAST + 1> keys{};

	Keyboard() = default;
	~Keyboard() = default;

	Keyboard(const Keyboard &other) = delete;
	Keyboard(Keyboard &&other) = default;
	Keyboard &operator=(const Keyboard &other) = delete;
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

	bool grabbed = false;

	Window(glm::ivec2 size, std::string &&title)
		: size(size), title(std::move(title)) {
		this->mouse = std::make_unique<Mouse>();
		this->keyboard = std::make_unique<Keyboard>();

		this->last_frame = global.time->now();
		this->last_second = global.time->now();

		if (!glfwInit()) {
			std::fprintf(stderr, "%s", "error initializing GLFW\n");
			std::exit(1);
		}

		glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

		this->handle.reset(
			glfwCreateWindow(size.x, size.y, this->title.c_str(), nullptr, nullptr));
		if (handle == nullptr) {
			std::fprintf(stderr, "%s", "error creating window\n");
			glfwTerminate();
			std::exit(1);
		}

		glfwMakeContextCurrent(handle.get());
		glfwSetWindowUserPointer(handle.get(), this);

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

	~Window() = default;

	Window(const Window &other) = delete;
	Window &operator=(const Window &other) = delete;

	Window(Window &&other) noexcept
		: handle(std::move(other.handle)),
		mouse(std::move(other.mouse)),
		keyboard(std::move(other.keyboard)),
		title(std::move(other.title)),
		size(other.size), tps(other.tps),
		last_second(other.last_second),
		frames(other.frames), fps(other.fps),
		frame_delta(other.frame_delta), ticks(other.ticks),
		tick_remainder(other.tick_remainder),
		last_frame(other.last_frame) {
		if (handle) {
			glfwSetWindowUserPointer(handle.get(), this);
		}

		other.size = glm::ivec2(0);
	}

	Window &operator=(Window &&other) noexcept {
		if (this == &other) return *this;

		handle = std::move(other.handle);
		mouse = std::move(other.mouse);
		keyboard = std::move(other.keyboard);
		title = std::move(other.title);

		size = other.size;
		last_second = other.last_second;
		frames = other.frames;
		fps = other.fps;
		last_frame = other.last_frame;
		frame_delta = other.frame_delta;
		ticks = other.ticks;
		tps = other.tps;
		tick_remainder = other.tick_remainder;

		if (handle) {
			glfwSetWindowUserPointer(handle.get(), this);
		}

		other.size = glm::ivec2(0);
		return *this;
	}

	inline GLFWwindow *get_handle() const {
		return handle.get();
	}

	inline glm::ivec2 get_size() const {
		return size;
	}

	inline void set_grabbed(bool grabbed) {
		this->grabbed = grabbed;
		glfwSetInputMode(handle.get(), GLFW_CURSOR, grabbed ?
			GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}

	inline bool is_should_close() const {
		return glfwWindowShouldClose(handle.get());
	}

	inline void set_should_close(bool value) {
		glfwSetWindowShouldClose(handle.get(), value);
	}

	inline void swap_buffers() {
		glfwSwapBuffers(handle.get());
	}

	inline Mouse *get_mouse() {
		return mouse.get();
	}
	
	inline Keyboard *get_keyboard() {
		return keyboard.get();
	}

	inline void poll_events() {
		glfwPollEvents();
	}

private:
	static inline void size_callback(GLFWwindow *handle, int width, int height) {
		auto *wnd = static_cast<Window *>(glfwGetWindowUserPointer(handle));
		glViewport(0, 0, width, height);
		wnd->size = glm::ivec2(width, height);
	}

	static inline void cursor_callback(GLFWwindow *handle, double xp, double yp) {
		glm::vec2 p{ xp, yp };

		auto *wnd = static_cast<Window *>(glfwGetWindowUserPointer(handle));

		wnd->mouse->delta = p - wnd->mouse->position;
		wnd->mouse->delta.x =
			glm::clamp(wnd->mouse->delta.x, -100.0f, 100.0f);
		wnd->mouse->delta.y =
			glm::clamp(wnd->mouse->delta.y, -100.0f, 100.0f);

		wnd->mouse->position = p;
	}

	static inline void key_callback(
		GLFWwindow *handle, int key, int scancode, int action, int mods) {
		assert(key >= 0 && key <= static_cast<int>(GLFW_KEY_LAST));

		auto *wnd = static_cast<Window *>(glfwGetWindowUserPointer(handle));

		switch (action) {
			case GLFW_PRESS:
				wnd->keyboard->keys[key].down = true;
				break;
			case GLFW_RELEASE:
				wnd->keyboard->keys[key].down = false;
				break;
			default:
				break;
		}
	}

	static inline void mouse_callback(
		GLFWwindow *handle, int button, int action, int mods) {
		assert(button >= 0 && button <= static_cast<int>(GLFW_MOUSE_BUTTON_LAST));

		auto *wnd = static_cast<Window *>(glfwGetWindowUserPointer(handle));

		switch (action) {
			case GLFW_PRESS:
				wnd->mouse->buttons[button].down = true;
				break;
			case GLFW_RELEASE:
				wnd->mouse->buttons[button].down = false;
				break;
			default:
				break;
		}
	}

	struct WindowDeleter {
		void operator()(GLFWwindow *wnd) { if (wnd) glfwDestroyWindow(wnd); }
	};

	std::unique_ptr<GLFWwindow, WindowDeleter> handle;
	std::unique_ptr<Mouse> mouse;
	std::unique_ptr<Keyboard> keyboard;

	glm::ivec2 size;
	std::string title;
};