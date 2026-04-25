#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "typedefs.hpp"

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

	template <typename X, typename Mn, typename Mx>
	inline constexpr X clamp(X x, Mn mn, Mx mx) {
		return math::max<X>(mn, math::min(mx, x));
	}
}