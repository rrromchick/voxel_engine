#pragma once

#include "chunk.hpp"
#include "block/block.hpp"
#include "player.hpp"
#include <vector>

#define WORLD_UNLOADED_DATA_CAP 64

struct WorldUnloadedData {
	glm::ivec3 pos;
	u32 data;

	WorldUnloadedData() = default;
	~WorldUnloadedData() = default;

	explicit WorldUnloadedData(glm::ivec3 pos, u32 data)
		: pos(pos), data(data) {}

	WorldUnloadedData(const WorldUnloadedData &other) = default;
	WorldUnloadedData(WorldUnloadedData &&other) = default;
	WorldUnloadedData &operator=(const WorldUnloadedData &other) = default;
	WorldUnloadedData &operator=(WorldUnloadedData &&other) = default;
};

#define world_foreach(_w, _cname)\
	Chunk *_cname;\
	for (usize i = 0; i < ((_w)->get_chunks_size() * (_w)->get_chunks_size()) &&\
		(_cname = (_w)->get_chunks()[i]) != (void *) INT64_MAX;\
		i++)

using SetFn = std::function<void(Chunk *, usize, usize, usize, u32)>;
using GetFn = std::function<u32(Chunk *, usize, usize, usize)>;
using NoiseFn = std::function<f32(void *p, f32 s, f32 x, f32 z)>;

//struct INoise {
//	std::array<u8, 512> params;
//
//	INoise() = default;
//
//	virtual ~INoise() = default;
//	virtual inline f32 fn(f32 s, f32 x, f32 z);
//};
//
//struct OctaveNoise : public INoise {
//	OctaveNoise(usize n, usize o) {
//		auto params = { n, o };
//		std::memcpy(&this->params, &params, sizeof(Octave));
//	}
//	
//	inline f32 compute(Octave *p, f32 s, f32 x, f32 z) override {
//		f32 u = 
//	}
};

struct Noise {
	std::array<u8, 512> params;
	NoiseFn compute;

	explicit Noise(NoiseFn &&compute)
		: compute(std::move(compute)) {}
};

struct Octave {
	usize n, o;
};

struct Combined {
	Noise *n, *m;
};

struct World {
	World();
	~World();

	World(const World &other) = delete;
	World(World &&other) = default;
	World &operator=(const World &other) = delete;
	World &operator=(World &&other) = default;

	inline void generate(Chunk *chunk);

	inline bool chunk_in_bounds(glm::ivec3 offset) const;
	inline bool in_bounds(glm::ivec3 pos) const;
	inline bool contains_chunk(glm::ivec3 offset) const;
	inline bool contains(glm::ivec3 pos) const;
	inline Chunk *get_chunk(glm::ivec3 offset) const;
	inline glm::ivec3 pos_to_block(glm::vec3 pos);
	inline glm::ivec3 pos_to_offset(glm::ivec3 pos) const;
	inline glm::ivec3 pos_to_chunk_pos(glm::ivec3 pos);

	inline void set_data(glm::ivec3 pos, u32 data);
	inline u32 get_data(glm::ivec3 pos);
	inline void set_center(glm::ivec3 center_pos);
	inline void render();
	inline void update();
	inline void tick();

	inline usize chunk_index(glm::ivec3 offset) const;

	struct {
		struct {
			usize count, max;
		} load, mesh;
	} throttles;

	inline usize get_chunks_size() const {
		return this->chunks_size;
	}

	inline Chunk **get_chunks() {
		return this->chunks.data();
	}

	inline EntityPlayer *get_player() {
		return &this->player;
	}

	inline usize chunk_index(glm::ivec3 offset) {
		glm::ivec3 p = offset - this->chunks_origin;
		return p.z * this->chunks_size + p.x;
	}

	inline glm::ivec3 chunk_offset(usize i) {
		return this->chunks_origin + 
			glm::ivec3(i % this->chunks_size, 0, i / this->chunks_size);
	}

	inline glm::ivec3 pos_to_offset(glm::ivec3 pos) {
		return glm::ivec3(
			glm::floor(pos.x / CHUNK_SIZE_F.x), 0, glm::floor(pos.z / CHUNK_SIZE_F.z));
	}

	inline glm::ivec3 pos_to_block(glm::vec3 pos) {
		return glm::ivec3(
			glm::floor(pos.x), glm::floor(pos.y), glm::floor(pos.z));
	}

	inline glm::ivec3 pos_to_chunk_pos(glm::ivec3 pos) {
		return glm::mod(glm::mod(pos, CHUNK_SIZE), CHUNK_SIZE) + CHUNK_SIZE;
	}

	inline void load_chunk(glm::ivec3 offset);
	inline void append_unloaded_data(glm::ivec3 pos, u32 data);
	inline void remove_unloaded_data(usize i);

private:
	EntityPlayer player;
	usize chunks_size;
	std::vector<Chunk *> chunks;
	std::vector<WorldUnloadedData> unloaded_data;
	usize last_data;
	glm::ivec3 chunks_origin, center_offset;

	inline void load_empty_chunks();

	std::function<bool(const glm::ivec3*, const glm::ivec3*)> btf_cmp = 
		[this](const glm::ivec3 *a, const glm::ivec3 *b) {
			return glm::distance2(glm::vec3(player.get_camera().position), glm::vec3(*a))
				> glm::distance2(glm::vec3(player.get_camera().position), glm::vec3(*b));
		};

	std::function<bool(const glm::ivec3 *, const glm::ivec3 *)> ftb_cmp =
		[this](const glm::ivec3 *a, const glm::ivec3 *b) {
			return glm::distance2(glm::vec3(player.get_camera().position), glm::vec3(*a))
				< glm::distance2(glm::vec3(player.get_camera().position), glm::vec3(*b));
		};
};