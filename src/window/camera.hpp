#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "typedefs.hpp"
#include "Window.hpp"
#include "Global.hpp"
#include <cmath>

using namespace glm;

struct Camera {
	vec3 front;
	vec3 up;
	vec3 right;
	vec3 position;

	float fov;
	mat4 rotation;

	Camera() = default;

	Camera(vec3 position, float fov)
		: position(position), fov(fov), rotation(1.0f) {
		update_vectors();
	}

	Camera(const Camera &other) = delete;
	Camera &operator=(const Camera &other) = delete;
	Camera(Camera &&other) = default;
	Camera &operator=(Camera &&other) = default;

	inline void rotate(float x, float y, float z) {
		rotation = glm::rotate(rotation, z, vec3(0, 0, 1));
		rotation = glm::rotate(rotation, y, vec3(0, 1, 0));
		rotation = glm::rotate(rotation, x, vec3(1, 0, 0));

		update_vectors();
	}

	mat4 get_projection() const {
		auto *wnd = global.window.get();
		auto aspect = static_cast<float>(wnd->get_size().x) 
			/ static_cast<float>(wnd->get_size().y);
		return glm::perspective(fov, aspect, 0.1f, 100.0f);
	}

	mat4 get_view() {
		return glm::lookAt(position, position + front, up);
	}

private:
	inline void update_vectors() {
		front = vec3(rotation * vec4(0, 0, -1, 1));
		right = vec3(rotation * vec4(1, 0, 0, 1));
		up = vec3(rotation * vec4(0, 1, 0, 1));
	}
};