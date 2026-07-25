#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Texture.hpp"

Texture::Texture(const std::string &path) {
	int width, height, channels;
	stbi_set_flip_vertically_on_load(true);

	unsigned char *data = stbi_load(
		path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	assert(data != nullptr);

	this->size = glm::ivec2(width, height);
	this->load_pixels(data, width, height);
	stbi_image_free(data);
}

Texture::Texture(const uint8_t *pixels, unsigned int width, unsigned int height) {
	this->load_pixels(pixels, width, height);
}

Texture::~Texture() {
	if (handle) glDeleteTextures(1, &handle);
}

Texture::Texture(Texture &&other) noexcept
	: handle(other.handle), size(other.size) {
	other.handle = 0;
}

Texture &Texture::operator=(Texture &&other) noexcept {
	if (this == &other) return *this;
	if (handle != 0) glDeleteTextures(1, &handle);
	this->handle = other.handle;
	this->size = other.size;
	other.handle = 0;
	return *this;
}

void Texture::load_pixels(const uint8_t *pixels, std::size_t width, std::size_t height) {
	this->size = glm::ivec2(width, height);
	
	glGenTextures(1, &handle);
	glBindTexture(GL_TEXTURE_2D, handle);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

Atlas::Atlas(const std::string &path, glm::ivec2 sprite_size)
	: sprite_size(sprite_size) {
	this->texture = std::make_shared<Texture>(path);
	update_units();
}

Atlas::Atlas(Texture texture, glm::ivec2 sprite_size)
	: sprite_size(sprite_size) {
	this->texture = std::make_shared<Texture>(std::move(texture));
	update_units();
}

Atlas::Atlas(Texture *texture, glm::ivec2 sprite_size)
	: sprite_size(sprite_size), texture(texture) {
	update_units();
}

void Atlas::update_units() {
	if (texture && texture->get_size().x > 0 && texture->get_size().y > 0) {
		this->sprite_unit = glm::vec2(sprite_size) / glm::vec2(texture->get_size());
	}
}