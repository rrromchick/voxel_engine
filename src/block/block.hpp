#pragma once

#include "util/direction.hpp"
#include <array>
#include <span>

#define BLOCK_ATLAS_FRAMES 5

struct BlockAtlas;
struct World;

enum BlockId : u16 {
	AIR = 0,
	GRASS, DIRT, STONE, SAND, WATER, GLASS, LOG, LEAVES,
	ROSE, BUTTERCUP, COAL, COPPER, LAVA, CLAY, GRAVEL, PLANKS,
	LAST,
};

struct Block {
	BlockId id = BlockId::AIR;

	bool transparent = false;
	bool animated = false;
	bool sprite = false;
	bool liquid = false;

	glm::ivec2 uv_pos = { 0, 0 };

	virtual bool is_transparent(World *world, glm::ivec3 pos) const {
		return false;
	}

	virtual glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const {
		assert(false);
		return glm::ivec2(0.0f);
	}

	virtual void get_animation_frames(
		std::span<glm::ivec2, BLOCK_ATLAS_FRAMES> out) const {
		return;
	}


	virtual bool is_sprite() const {
		return false;
	}

	virtual bool is_liquid() const {
		return false;
	}

	virtual bool is_animated() const {
		return false;
	}
};

struct BlockAir : public Block {
	bool is_transparent(World *world, glm::ivec3 pos) const override {
		return true;
	}

	explicit BlockAir() {
		this->id = BlockId::AIR;
		this->transparent = true;
	}
};

struct BlockButtercup : public Block {
	bool is_transparent(World *world, glm::ivec3 pos) const override {
		return true;
	}

	bool is_sprite() const override {
		return true;
	}

	glm::ivec2 get_texture_location(World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(1, 3);
	}

	explicit BlockButtercup() {
		this->id = BlockId::BUTTERCUP;
		this->transparent = true;
		this->sprite = true;
	}
};

struct BlockClay : public Block {
	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(5, 1);
	}

	explicit BlockClay() {
		this->id = BlockId::CLAY;
	}
};

struct BlockCoal : public Block {
	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(4, 0);
	}

	explicit BlockCoal() {
		this->id = BlockId::COAL;
	}
};

struct BlockCopper : public Block {
	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(5, 0);
	}

	explicit BlockCopper() {
		this->id = BlockId::COPPER;
	}
};

struct BlockDirt : public Block {
	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(2, 0);
	}

	explicit BlockDirt() {
		this->id = BlockId::DIRT;
	}
};

struct BlockGlass : public Block {
	bool is_transparent(World *world, glm::ivec3 pos) const override {
		return true;
	}

	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(1, 1);
	}

	explicit BlockGlass() {
		this->id = BlockId::GLASS;
		this->transparent = true;
	}
};

struct BlockGrass : public Block {
	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		switch (d) {
			case Direction::UP:
				return glm::ivec2(0);
			case Direction::DOWN:
				return glm::ivec2(2, 0);
			default:
				return glm::ivec2(1, 0);
		}
	}

	explicit BlockGrass() {
		this->id = BlockId::GRASS;
	}
};

struct BlockGravel : public Block {
	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(6, 0);
	}

	explicit BlockGravel() {
		this->id = BlockId::GRAVEL;
	}
};

struct BlockLava : public Block {
	bool is_transparent(World *world, glm::ivec3 pos) const override {
		return true;
	}

	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(0, 4);
	}

	bool is_animated() const override {
		return true;
	}

	bool is_liquid() const override {
		return true;
	}

	void get_animation_frames(
		std::span<glm::ivec2, BLOCK_ATLAS_FRAMES> out) const override {
		out[0] = glm::ivec2(0, 4);
		out[1] = glm::ivec2(1, 4);
		out[2] = glm::ivec2(2, 4);
		out[3] = glm::ivec2(3, 4);
		out[4] = glm::ivec2(4, 4);
	}

	explicit BlockLava() {
		this->id = BlockId::LAVA;
		this->transparent = true;
		this->animated = true;
		this->liquid = true;
	}
};

struct BlockLeaves : public Block {
	bool is_transparent(World *world, glm::ivec3 pos) const override {
		return true;
	}

	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(4, 1);
	}

	explicit BlockLeaves() {
		this->id = BlockId::LEAVES;
		this->transparent = true;
	}
};

struct BlockLog : public Block {
	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		switch (d) {
			case Direction::UP:
			case Direction::DOWN:
				return glm::ivec2(3, 1);
			default:
				return glm::ivec2(2, 1);
		}
	}

	explicit BlockLog() {
		this->id = BlockId::LOG;
	}
};

struct BlockPlanks : public Block {
	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(6, 1);
	}

	explicit BlockPlanks() {
		this->id = BlockId::PLANKS;
	}
};

struct BlockRose : public Block {
	bool is_transparent(World *world, glm::ivec3 pos) const override {
		return true;
	}

	bool is_sprite() const override {
		return true;
	}

	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(0, 3);
	}

	explicit BlockRose() {
		this->id = BlockId::ROSE;
		this->transparent = true;
		this->sprite = true;
	}
};

struct BlockSand : public Block {
	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(0, 1);
	}

	explicit BlockSand() {
		this->id = BlockId::SAND;
	}
};

struct BlockStone : public Block {
	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(3, 0);
	}

	explicit BlockStone() {
		this->id = BlockId::STONE;
	}
};

struct BlockWater : public Block {
	bool is_transparent(World *world, glm::ivec3 pos) const override {
		return true;
	}

	glm::ivec2 get_texture_location(
		World *world, glm::ivec3 pos, Direction d) const override {
		return glm::ivec2(0, 2);
	}

	bool is_animated() const override {
		return true;
	}

	bool is_liquid() const override {
		return true;
	}

	void get_animation_frames(
		std::span<glm::ivec2, BLOCK_ATLAS_FRAMES> out) const override {
		out[0] = glm::ivec2(0, 2);
		out[1] = glm::ivec2(1, 2);
		out[2] = glm::ivec2(2, 2);
		out[3] = glm::ivec2(2, 2);
		out[4] = glm::ivec2(4, 2);
	}

	explicit BlockWater() {
		this->id = BlockId::WATER;
		this->transparent = true;
		this->animated = true;
		this->liquid = true;
	}
};

struct Blocks {
	Blocks() = default;

	inline Block &operator[](BlockId id) {
		switch (id) {
			case BlockId::AIR: return air;
			case BlockId::BUTTERCUP: return buttercup;
			case BlockId::CLAY: return clay;
			case BlockId::COPPER: return copper;
			case BlockId::COAL: return coal;
			case BlockId::DIRT: return dirt;
			case BlockId::GRASS: return grass;
			case BlockId::GLASS: return glass;
			case BlockId::GRAVEL: return gravel;
			case BlockId::LAVA: return lava;
			case BlockId::LEAVES: return leaves;
			case BlockId::LOG: return log;
			case BlockId::PLANKS: return planks;
			case BlockId::ROSE: return rose;
			case BlockId::SAND: return sand;
			case BlockId::STONE: return stone;
			case BlockId::WATER: return water;
			default: return air;
		}
	}

	inline const Block &operator[](BlockId id) const {
		return const_cast<Blocks *>(this)->operator[](id);
	}

	BlockAir air;
	BlockButtercup buttercup;
	BlockClay clay;
	BlockCopper copper;
	BlockCoal coal;
	BlockDirt dirt;
	BlockGrass grass;
	BlockGlass glass;
	BlockGravel gravel;
	BlockLava lava;
	BlockLeaves leaves;
	BlockLog log;
	BlockPlanks planks;
	BlockRose rose;
	BlockSand sand;
	BlockStone stone;
	BlockWater water;
};

extern Blocks blocks;