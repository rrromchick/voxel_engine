#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <memory>

struct Texture {
	Texture() : handle(0), size(0) {}

	Texture(const std::string &path);
	Texture(const uint8_t *pixels, unsigned int width, unsigned int height);
	~Texture();

	Texture(const Texture &other) = delete;
	Texture &operator=(const Texture &other) = delete;
	Texture(Texture &&other) noexcept;
	Texture &operator=(Texture &&other) noexcept;

	inline void bind() const { glBindTexture(GL_TEXTURE_2D, handle); }
	inline glm::ivec2 get_size() const { return size; }

private:
	void load_pixels(const uint8_t *pixels, std::size_t width, std::size_t height);

	GLuint handle{ 0 };
	glm::ivec2 size{};
	glm::uvec2 uv_unit{};
};

struct Atlas {
	Atlas() = default;
	Atlas(const std::string &path, glm::ivec2 sprite_size);
	Atlas(Texture texture, glm::ivec2 sprite_size);
	Atlas(Texture *texture, glm::ivec2 sprite_size);

	inline glm::vec2 offset(glm::ivec2 pos) const {
		int total_rows = texture->get_size().y / sprite_size.y;
		int flipped_y = total_rows - pos.y - 1;
		return glm::vec2(pos.x, flipped_y) * sprite_unit;
	}

	inline void bind() const { texture->bind(); }
	inline std::shared_ptr<Texture> get_texture() { return texture; }
	inline glm::ivec2 get_sprite_size() const { return sprite_size; }
	inline glm::vec2 get_sprite_unit() const { return sprite_unit; }

private:
	void update_units();

	std::shared_ptr<Texture> texture;
	glm::ivec2 sprite_size{};
	glm::vec2 sprite_unit{};
};