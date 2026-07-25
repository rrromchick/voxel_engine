#pragma once

#include "VBO.hpp"
#include <cstddef>

struct VAO {
	GLuint handle;

	explicit VAO() {
		glGenVertexArrays(1, &this->handle);
	}

	~VAO() {
		if (handle != 0) glDeleteVertexArrays(1, &this->handle);
	}

	VAO(const VAO &other) = delete;
	VAO &operator=(const VAO &other) = delete;

	VAO(VAO &&other) noexcept
		: handle(other.handle) {
		other.handle = 0;
	}

	VAO &operator=(VAO &&other) noexcept {
		assert(this != &other);

		if (this->handle != 0) glDeleteVertexArrays(1, &this->handle);
		this->handle = other.handle;
		other.handle = 0;

		return *this;
	}

	inline void bind() const {
		glBindVertexArray(this->handle);
	}

	inline void unbind() const {
		glBindVertexArray(0);
	}

	inline void attr(
		const VBO &vbo, GLuint index, GLint size,
		GLenum type, GLsizei stride, std::size_t offset) {
		this->bind();
		vbo.bind();
		glVertexAttribPointer(
			index, size, type, GL_FALSE, stride,
			reinterpret_cast<void*>(offset));
		glEnableVertexAttribArray(index);
	}
};