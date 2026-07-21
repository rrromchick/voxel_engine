#include "Mesh.hpp"

Mesh::Mesh(std::span<const float> buffer, std::span<const int> attrs)
	: vbo(GL_ARRAY_BUFFER, false) {
	vertex_size = 0;
	for (const auto &attr : attrs) {
		if (attr == 0) break;
		vertex_size += attr;
	}

	GLsizei stride = vertex_size * sizeof(float);
	this->vertices_count = buffer.size() / vertex_size;
	vbo.data(buffer.data(), buffer.size_bytes());

	int offset = 0;
	for (usize i = 0; i < attrs.size(); i++) {
		auto size = attrs[i];
        if (size == 0) break;
		vao.attr(
			vbo, static_cast<int>(i), size, GL_FLOAT,
			stride, offset * sizeof(float));
		offset += size;
	}

	vao.unbind();
}

void Mesh::reload(std::span<const float> buffer) {
	vbo.data(buffer.data(), buffer.size_bytes());
	this->vertices_count = buffer.size() / vertex_size;
}

void Mesh::draw(uint primitive) {
	vao.bind();
	glDrawArrays(primitive, 0, vertices_count);
	vao.unbind();
}