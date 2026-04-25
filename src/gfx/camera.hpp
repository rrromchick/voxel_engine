#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "typedefs.hpp"

enum CameraMovement : u8 {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
};

constexpr f32 YAW = -90.0f;
constexpr f32 PITCH = 0.0f;
constexpr f32 SPEED = 2.5f;
constexpr f32 SENSITIVITY = 0.1f;
constexpr f32 ZOOM = 45.0f;

struct Camera {
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 world_up;
	glm::vec3 direction;

	f32 yaw;
	f32 pitch;
	f32 movement_speed;
	f32 mouse_sensitivity;
	f32 zoom;
	f32 fov;
	f32 aspect, znear, zfar;
	
	glm::mat4 view, proj;

	Camera(
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
		f32 yaw = YAW,
		f32 pitch = PITCH)
		: front(glm::vec3(0.0f, 0.0f, -1.0f)),
		movement_speed(SPEED),
		mouse_sensitivity(SENSITIVITY),
		zoom(ZOOM) {
		this->position = position;
		this->world_up = up;
		this->yaw = yaw;
		this->pitch = pitch;
		this->update_camera_vectors();
	}

	Camera(
		f32 pos_x, f32 pos_y, f32 pos_z,
		f32 up_x, f32 up_y, f32 up_z,
		f32 yaw, f32 pitch)
		: front(glm::vec3(0.0f, 0.0f, -1.0f)),
		movement_speed(SPEED),
		mouse_sensitivity(SENSITIVITY),
		zoom(ZOOM) {
		this->position = glm::vec3(pos_x, pos_y, pos_z);
		this->world_up = glm::vec3(up_x, up_y, up_z);
		this->yaw = yaw;
		this->pitch = pitch;
		this->update_camera_vectors();
	}

	Camera(f32 fov) 
		: fov(fov), znear(0.01f), zfar(1000.0f) {
		this->aspect = 
			static_cast<f32>(window.get()->size().x / window.get()->size().y);
		this->update();
	}

	inline glm::mat4 get_view_matrix() {
		return glm::lookAt(this->position, position + front, this->up);
	}

	inline void process_keyboard(CameraMovement direction, f32 delta_time) {
		f32 velocity = this->movement_speed * delta_time;
		if (direction == FORWARD) {
			this->position += front * velocity;
		}
		if (direction == BACKWARD) {
			this->position -= front * velocity;
		}
		if (direction == LEFT) {
			this->position -= right * velocity;
		}
		if (direction == RIGHT) {
			this->position += right * velocity;
		}
	}

	inline void process_mouse_movement(
		f32 xoffset, f32 yoffset,
		GLboolean constrain_pitch = true) {
		xoffset *= this->mouse_sensitivity;
		yoffset *= this->mouse_sensitivity;

		this->yaw += xoffset;
		this->pitch += yoffset;

		if (constrain_pitch) {
			if (this->pitch > 89.0f) {
				this->pitch = 89.0f;
			} 
			if (this->pitch < -89.0f) {
				this->pitch = -89.0f;
			}
		}

		this->update_camera_vectors();
	}

	inline void process_mouse_scroll(f32 yoffset) {
		this->zoom -= static_cast<float>(yoffset);
		if (this->zoom < 1.0f) {
			this->zoom = 1.0f;
		}
		if (this->zoom > 45.0f) {
			this->zoom = 45.0f;
		}
	}

	inline void update() {
		this->pitch = math::clamp(this->pitch, -math::PI_2, math::PI_2);
		this->yaw = 
			(this->yaw < 0 ? math::TAU : 0.0f) + glm::mod(this->yaw, math::TAU);

		this->direction = glm::vec3(glm::cos(pitch) * glm::sin(yaw),
			glm::sin(pitch), glm::cos(this->pitch) * glm::cos(this->yaw));
		glm::normalize(this->direction);

		this->right = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), this->direction);
		this->up = glm::cross(this->direction, this->right);
		this->view = glm::mat4(1.0f);
		this->proj = glm::mat4(1.0f);

		this->view = glm::lookAt(this->position, position + direction, up);
		this->proj = glm::perspective(fov, aspect, znear, zfar);
	}

private:
	inline void update_camera_vectors() {
		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		this->front = glm::normalize(front);

		this->right = glm::normalize(glm::cross(this->front, this->world_up));
		this->up = glm::normalize(glm::cross(this->right, this->front));
	}

	std::shared_ptr<Window> window;
};