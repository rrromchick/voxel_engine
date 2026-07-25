#pragma once

#include "VAO.hpp"
#include "VBO.hpp"
#include <span>

struct Mesh {
	Mesh(std::span<const float> buffer, std::span<const int> attrs);
	~Mesh() = default;

	Mesh(const Mesh &other) = delete;
	Mesh &operator=(const Mesh &other) = delete;
	Mesh(Mesh &&other) noexcept = default;
	Mesh &operator=(Mesh &&other) noexcept = default;

	void reload(std::span<const float> buffer);
	void draw(unsigned int primitive);

	inline std::size_t get_vertices_count() const {
		return vertices_count;
	}

private:
	VAO vao;
	VBO vbo;
	std::size_t vertices_count;
	int vertex_size;
};