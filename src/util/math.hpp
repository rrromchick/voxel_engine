#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "typedefs.hpp"
#include "util/direction.hpp"

namespace math {
	static constexpr f32 PI_2 = glm::pi<f32>() / 2.0f;
	static constexpr f32 PI = glm::pi<f32>();
	static constexpr f32 TAU = glm::pi<f32>() * 2;

	template <typename T>
	inline constexpr T max(T x, T y) {
		return (x > y) ? x : y;
	}

	template <typename T>
	inline constexpr T min(T x, T y) {
		return (x < y) ? x : y;
	}

	template <typename X, typename Mn = X, typename Mx = X>
	inline constexpr X clamp(X x, Mn mn, Mx mx) {
		return math::max<X>(mn, math::min<Mn>(mx, x));
	}

	inline glm::vec3 intbound(glm::vec3 s, glm::vec3 ds) {
		glm::vec3 v;
		
		for (usize i = 0; i < 3; i++) {
			v[i] = (ds[i] > 0 ? (glm::ceil(s[i]) - s[i]) : (s[i] - glm::floor(s[i])));
		}
		return v;
	}

	struct Ray {
		glm::vec3 origin;
		glm::vec3 direction;

		Ray() = default;

		explicit Ray(glm::vec3 origin, glm::vec3 direction)
			: origin(origin), direction(direction) {}

		Ray(const Ray &other) = delete;
		Ray(Ray &&other) = default;
		Ray &operator=(const Ray &other) = delete;
		Ray &operator=(Ray &&other) = default;
	};

	inline bool ray_block(
		Ray ray, f32 max_distance, std::function<bool(glm::vec3)> f,
		glm::ivec3 *out, Direction *d_out) {
		glm::vec3 p, step;
		glm::vec3 d, tmax, tdelta;
		f32 radius;

		p = glm::ivec3(
			glm::floor(ray.origin.x), glm::floor(ray.origin.y), glm::floor(ray.origin.z));
		
		d = ray.direction;
		step = glm::ivec3(glm::sign(d.x), glm::sign(d.y), glm::sign(d.z));
		tmax = intbound(ray.origin, d);
		tdelta = glm::vec3(step) / d;
		radius = max_distance / glm::length(d);

		while (true) {
			if (f(p)) {
				*out = p;
				return true;
			}

			if (tmax.x < tmax.y) {
				if (tmax.x < tmax.z) {
					if (tmax.x > radius) {
						break;
					}

					p.x += step.x;
					tmax.x += tdelta.x;
					*d_out = direction::vec3_to_dir(glm::vec3(-step.x, 0, 0));
				} else {
					if (tmax.z > radius) {
						break;
					}

					p.z += step.z;
					tmax.z += tdelta.z;
					*d_out = direction::vec3_to_dir(glm::vec3(0, 0, -step.z));;
				}
			} else {
				if (tmax.y < tmax.z) {
					if (tmax.y > radius) {
						break;
					}

					p.y += step.y;
					tmax.y += tdelta.y;
					*d_out = direction::vec3_to_dir(glm::vec3(0, -step.y, 0));
				} else {
					if (tmax.z > radius) {
						break;
					}

					p.z += step.z;
					tmax.z += tdelta.z;
					*d_out = direction::vec3_to_dir(glm::vec3(0, 0, -step.z));
				}
			}
		}

		return false;
	}

	constexpr u64 ivec3hash(glm::ivec3 v) {
		u64 h = 0;
		for (int i = 0; i < 3; i++) {
			h ^= v[i] + 0x9e3779b9 + (h << 6) + (h >> 2);
		}
		return h;
	}
}