#pragma once

#include "gfx/texture.hpp"
#include "block.hpp"
#include "typedefs.hpp"

#define BLOCK_ATLAS_FPS 5
#define BLOCK_ATLAS_FRAMES 5

struct BlockAtlas {
	static constexpr auto SPRITE_SIZE = glm::ivec2(16, 16);

	BlockAtlas() : ticks(0) {}

	explicit BlockAtlas(const std::string &path)
		: ticks(0) {
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		u8 *base_pixels =
			stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		assert(base_pixels != nullptr);

		usize pixels_size = width * height * 4;

		for (usize i = 0; i < BLOCK_ATLAS_FRAMES; i++) {
			std::vector<u8> frame_buffer(base_pixels, base_pixels + pixels_size);

			for (usize n = 0; n < BlockId::LAST; n++) {
				const auto &block = blocks[static_cast<BlockId>(n)];
				if (block.id == 0 || !block.is_animated()) continue;

				std::array<glm::ivec2, BLOCK_ATLAS_FRAMES> anim_frames;
				block.get_animation_frames(anim_frames);

				this->copy_offset(
					frame_buffer.data(), glm::ivec2(width, height),
					anim_frames[i], anim_frames[0]);
			}

			Texture tex(frame_buffer.data(), width, height);
			this->frames.push_back(
				TextureAtlas::create_from_texture(std::move(tex), SPRITE_SIZE));
		}

		stbi_image_free(base_pixels);
	}

	~BlockAtlas() = default;

	inline void tick() {
		this->ticks++;
	}

	inline const TextureAtlas &get_current_atlas() const {
		usize frame_idx = (ticks / BLOCK_ATLAS_FPS) % BLOCK_ATLAS_FRAMES;
		return this->frames[frame_idx];
	}

private:
	inline void copy_offset(
		u8 *pixels, glm::ivec2 image_size, glm::ivec2 from, glm::ivec2 to) {
		glm::ivec2 from_px = from * SPRITE_SIZE;
		glm::ivec2 to_px = to * SPRITE_SIZE;

		for (int y = 0; y < SPRITE_SIZE.y; y++) {
			u8 *src_row = &pixels[((from_px.y + y) * image_size.x + from_px.x) * 4];
			u8 *dst_row = &pixels[((to_px.y + y) * image_size.x + to_px.x) * 4];
			std::memcpy(dst_row, src_row, SPRITE_SIZE.x * 4);
		}
	}

	std::vector<TextureAtlas> frames;
	usize ticks;
};