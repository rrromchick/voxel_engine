#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "gfx/shader.hpp"
#include "gfx/camera.hpp"
#include "gfx/model.hpp"

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, f64 xpos, f64 ypos);
void scroll_callback(GLFWwindow *window, f64 xoffset, f64 yoffset);
void process_input(GLFWwindow *window);

constexpr uint SCREEN_WIDTH = 800;
constexpr uint SCREEN_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
f32 last_x = SCREEN_WIDTH / 2.0f;
f32 last_y = SCREEN_HEIGHT / 2.0f;
bool first_mouse = true;

float delta_time = 0.0f;
float last_frame = 0.0f;

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARR_COMPAT, GL_TRUE);
#endif

	GLFWwindow *window = glfwCreateWindow(
		SCREEN_WIDTH, SCREEN_HEIGHT, "VoxelEngine", nullptr, nullptr);
	if (window == nullptr) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	stbi_set_flip_vertically_on_load(true);

	glEnable(GL_DEPTH_TEST);

	Shader our_shader(
		"res/shaders/model_loading.glslv", "res/shaders/model_loading.glslf");
	Model our_model("res/obj/backpack/backpack.obj");

	while (!glfwWindowShouldClose(window)) {
		auto current_frame = static_cast<float>(glfwGetTime());
		delta_time = current_frame - last_frame;
		last_frame = current_frame;

		process_input(window);

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		our_shader.use();

		glm::mat4 projection = glm::perspective(
			glm::radians(camera.zoom), static_cast<float>(SCREEN_WIDTH / SCREEN_HEIGHT),
			0.1f, 100.0f);
		glm::mat4 view = camera.get_view_matrix();
		our_shader.set_mat4("projection", projection);
		our_shader.set_mat4("view", view);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
		our_shader.set_mat4("model", model);
		our_model.draw(our_shader);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

void process_input(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		camera.process_keyboard(FORWARD, delta_time);
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		camera.process_keyboard(BACKWARD, delta_time);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		camera.process_keyboard(LEFT, delta_time);
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		camera.process_keyboard(RIGHT, delta_time);
	}
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, f64 xpos_in, f64 ypos_in) {
	auto xpos = static_cast<f32>(xpos_in);
	auto ypos = static_cast<f32>(ypos_in);

	if (first_mouse) {
		last_x = xpos;
		last_y = ypos;
		first_mouse = false;
	}

	f32 xoffset = xpos - last_x;
	f32 yoffset = last_y - ypos;

	last_x = xpos;
	last_y = ypos;

	camera.process_mouse_movement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, f64 xoffset, f64 yoffset) {
	camera.process_mouse_scroll(static_cast<f32>(yoffset));
}