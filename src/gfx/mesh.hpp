#pragma once

#include "std.hpp"
#include "vbo.hpp"
#include "vao.hpp"

struct Mesh {
	explicit Mesh(const f32 *buffer, usize vertices, const int *attrs) 
		: vertices(vertices), vao(), vbo(GL_ARRAY_BUFFER, false) {
		int vertex_size = 0;
		for (int i = 0; attrs[i]; i++) {
			vertex_size += attrs[i];
		}
	}

	~Mesh() {

	}

	Mesh(const Mesh &other) = delete;
	Mesh &operator=(const Mesh &other) = delete;

	Mesh(Mesh &&other) noexcept
		: vao(std::move(other.vao)),
		vbo(std::move(other.vbo)) {

	}

	Mesh &operator=(Mesh &&other) noexcept {
		vao = std::move(other.vao);
		vbo = std::move(other.vbo);
	}

	inline void draw(uint primitive) {

	}

private:
	VAO vao;
	VBO vbo;
	usize vertices;
};