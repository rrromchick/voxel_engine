#pragma once

#include "std.hpp"
#include "gfx/vao.hpp"
#include "gfx/vbo.hpp"
#include "typedefs.hpp"

struct Mesh {
	Mesh(std::span<const float> buffer, std::span<const int> attrs);
	~Mesh() = default;

	Mesh(const Mesh &other) = delete;
	Mesh &operator=(const Mesh &other) = delete;
	Mesh(Mesh &&other) noexcept = default;
	Mesh &operator=(Mesh &&other) noexcept = default;

	void reload(std::span<const float> buffer);
	void draw(uint primitive);

	inline usize get_vertices_count() const {
		return vertices_count;
	}

private:
	VAO vao;
	VBO vbo;
	usize vertices_count;
	int vertex_size;
};