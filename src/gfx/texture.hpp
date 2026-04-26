#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "typedefs.hpp"
#include <string>
#include <vector>
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct Texture {
	Texture() :
		handle(0), sz(0) {}

	explicit Texture(const std::string &path) {
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		unsigned char *data =
			stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		assert(data != nullptr);

		this->sz = glm::ivec2(width, height);

		this->load_pixels(data, width, height);
		stbi_image_free(data);
	}

	Texture(const u8 *pixels, usize width, usize height) {
		this->load_pixels(pixels, width, height);
	}

	~Texture() {
		if (this->handle) glDeleteTextures(1, &handle);
	}

	Texture(const Texture &other) = delete;
	Texture &operator=(const Texture &other) = delete;

	Texture(Texture &&other)
		: handle(other.handle), sz(other.sz) {
		other.handle = 0;
	}

	Texture &operator=(Texture &&other) {
		assert(this != &other);
		
		if (handle != 0) glDeleteTextures(1, &handle);
		this->handle = other.handle;
		this->sz = other.sz;
		other.handle = 0;
		return *this;
	}

	inline void bind() const {
		glBindTexture(GL_TEXTURE_2D, handle);
	}

	inline glm::ivec2 size() const {
		return this->sz;
	}

private:
	inline void load_pixels(const u8 *pixels, usize width, usize height) {
		this->sz = glm::ivec2(width, height);

		glGenTextures(1, &this->handle);
		glBindTexture(GL_TEXTURE_2D, this->handle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA8, this->sz.x, this->sz.y, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	}

	GLuint handle;
	glm::ivec2 sz;
	glm::uvec2 uv_unit;
};

struct TextureAtlas {
	TextureAtlas() = default;

	explicit TextureAtlas(const std::string &path, glm::ivec2 sprite_size)
		: tex(path), sprite_sz(sprite_size) {
		this->update_units();
	}

	TextureAtlas(const Texture &tex, glm::ivec2 sprite_size)
		: tex(std::move(tex)), sprite_sz(sprite_size) {
		this->update_units();
	}

	static TextureAtlas create_from_texture(Texture tex, glm::ivec2 sprite_size) {
		TextureAtlas atlas;
		atlas.tex = std::move(tex);
		atlas.sprite_sz = sprite_size;
		atlas.update_units();
		return atlas;
	}

	inline glm::vec2 offset(glm::ivec2 pos) const {
		return glm::vec2(
			pos.x, (tex.size().y / sprite_sz.y) - pos.y - 1) * sprite_unit;
	}

	inline void bind() const {
		this->tex.bind();
	}

	inline const Texture &texture() const {
		return this->tex;
	}

	inline glm::ivec2 sprite_size() const {
		return this->sprite_sz;
	}

	inline glm::vec2 unit() const {
		return this->sprite_unit;
	}

private:
	inline void update_units() {
		if (tex.size().x > 0 && tex.size().y > 0) {
			this->sprite_unit = glm::vec2(sprite_sz) / glm::vec2(tex.size());
		}
	}

	Texture tex;
	glm::ivec2 sprite_sz;
	glm::vec2 sprite_unit;
};