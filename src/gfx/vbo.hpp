#pragma once

#include <glad/glad.h>
#include "typedefs.hpp"
#include <assert.h>

struct VBO {
	GLuint handle;
	GLint type;
	bool dynamic;

	explicit VBO(GLint type, bool dynamic)
		: type(type), dynamic(dynamic) {
		glGenBuffers(1, &this->handle);
	}

	~VBO() {
		if (handle != 0) glDeleteBuffers(1, &this->handle);
	}

	VBO(const VBO &other) = delete;
	VBO &operator=(const VBO &other) = delete;

	VBO(VBO &&other)
		: handle(other.handle), type(other.type), dynamic(other.dynamic) {
		other.handle = 0;
	}

	VBO &operator=(VBO &&other) {
		assert(this != &other);
		
		if (this->handle != 0) glDeleteBuffers(1, &handle);
		this->handle = other.handle;
		this->type = other.type;
		this->dynamic = other.dynamic;
		other.handle = 0;

		return *this;
	}

	inline void bind() const {
		glBindBuffer(this->type, this->handle);
	}

	inline void data(const void *data, usize size_bytes) {
		this->bind();
		glBufferData(
			type,
			size_bytes,
			data,
			dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	}

	inline void sub_data(const void *data, usize offset, usize size_bytes) {
		this->bind();
		glBufferSubData(type, offset, size_bytes, data);
	}
};