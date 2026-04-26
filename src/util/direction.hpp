#pragma once

#include "typedefs.hpp"

enum Direction : u8 {
	NORTH = 0,
	SOUTH,
	EAST,
	WEST,
	UP,
	DOWN,
};

namespace direction {
	constexpr std::array<glm::vec3, 6> dir_vec = {
		glm::vec3({ 0, 0, -1 }),
		glm::vec3({ 0, 0, 1 }),
		glm::vec3({ 1, 0, 0 }),
		glm::vec3({ -1, 0, 0 }),
		glm::vec3({ 0, 1, 0 }),
		glm::vec3({ 0, -1, 0 }),
	};

	inline Direction vec3_to_dir(glm::vec3 v) {
		for (usize i = 0; i < dir_vec.size(); i++) {
			if (std::memcmp(&dir_vec[i], &v, sizeof(glm::vec3))) {
				return static_cast<Direction>(i);
			}
		}

		assert(false);
		return static_cast<Direction>(-1);
	}

	inline glm::ivec3 dir_to_vec3(const Direction &d) {
		return dir_vec[d];
	}
}