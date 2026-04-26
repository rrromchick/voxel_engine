#pragma once

#include "gfx/camera.hpp"
#include "block/block.hpp"

struct World;

struct EntityPlayer {
	EntityPlayer(World *world);
	~EntityPlayer();
	
	EntityPlayer(const EntityPlayer &other) = delete;
	EntityPlayer &operator=(const EntityPlayer &other) = delete;
	EntityPlayer(EntityPlayer &&other) = default;
	EntityPlayer &operator=(EntityPlayer &&other) = default;

	inline void render();
	inline void update();
	inline void tick();

	bool has_look_block;
	glm::ivec3 look_block;
	Direction look_face;

	glm::ivec3 offset;
	glm::ivec3 block_pos;

	bool offset_changed, block_pos_changed;

	BlockId selected_block;

	inline const Camera &get_camera() {
		return this->camera;
	}

private:
	World *world;
	Camera camera;
};