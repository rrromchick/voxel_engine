#pragma once

#include "chunk.hpp"
#include "block/block.hpp"
#include "player.hpp"

struct WorldUnloadedData {
	glm::ivec3 pos;
	u32 data;

	WorldUnloadedData() = default;
	~WorldUnloadedData() = default;

	WorldUnloadedData(const WorldUnloadedData &other) = delete;
	WorldUnloadedData(WorldUnloadedData &&other) = default;
	WorldUnloadedData &operator=(const WorldUnloadedData &other) = delete;
	WorldUnloadedData &operator=(WorldUnloadedData &&other) = default;
};

#define world_foreach(_w, _cname)\
	Chunk *_cname;\
	for (usize i = 0; i < (_w)->get_chunks_size() * (_w)->get_chunks_size()) &&\
		(_cname = (_w)->get_chunks()[i]) != (void *) INT64_MAX;\
		i++)

constexpr int btf_pcmp(glm::ivec3 *center, const glm::ivec3 **a, const glm::ivec3 **b) {
	return -(glm::length2(*center - **a) - glm::length2(*center - **b));
}

constexpr int ftb_pcmp(glm::ivec3 *center, const glm::ivec3 **a, const glm::ivec3 **b) {
	return glm::length2(*center - **a) - glm::length2(*center - **b);
}

constexpr int btf_cmp(glm::ivec3 *center, const glm::ivec3 *a, const glm::ivec3 *b) {
	return -(glm::length2(*center - *a) - glm::length2(*center - *b));
}

constexpr int ftb_cmp(glm::ivec3 *center, const glm::ivec3 *a, const glm::ivec3 *b) {
	return -(glm::length2(*center - *b) - glm::length2(*center - *a));
}

struct World {
	World();
	~World();

	World(const World &other) = delete;
	World(World &&other) = default;
	World &operator=(const World &other) = delete;
	World &operator=(World &&other) = default;

	inline void generate(Chunk *chunk);

	inline bool in_bounds(glm::ivec3 pos) const;
	inline bool contains(glm::ivec3 pos) const;
	inline Chunk *get_chunk(glm::ivec3 offset);
	inline glm::ivec3 pos_to_block(glm::vec3 pos);
	inline glm::ivec3 pos_to_offset(glm::ivec3 pos);
	inline glm::ivec3 pos_to_chunk_pos(glm::ivec3 pos);

	inline void set_data(glm::ivec3 pos, u32 data);
	inline u32 get_data(glm::ivec3 pos);
	inline void set_center(glm::ivec3 center_pos);
	inline void render();
	inline void update();
	inline void tick();

	struct {
		WorldUnloadedData *list;
		usize size, capacity;
	} unloaded_data;

	struct {
		struct {
			usize count, max;
		} load, mesh;
	} throttles;

	inline usize get_chunks_size() const {
		return this->chunks_size;
	}

	inline Chunk **get_chunks() {
		return this->chunks;
	}

private:
	EntityPlayer player;
	usize chunks_size;
	Chunk **chunks;
	glm::ivec3 chunks_origin, center_offset;
};