#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include "gfx/vbo.hpp"
#include "gfx/vao.hpp"
#include "world.hpp"
#include "typedefs.hpp"
#include <memory>
#include <algorithm>

constexpr auto CHUNK_SIZE = glm::ivec3(16, 256, 16);
constexpr auto CHUNK_SIZE_F = glm::vec3(16, 256, 16);

constexpr auto CHUNK_VOLUME = CHUNK_SIZE.x * CHUNK_SIZE.y * CHUNK_SIZE.z;

struct Face {
	usize indices_base;
	glm::vec3 position;
	f32 distance;

	Face() = default;
	~Face() = default;

	Face(const Face &other) = delete;
	Face(Face &&other) = default;
	Face &operator=(const Face &other) = delete;
	Face &operator=(Face &&other) = default;
};

struct MeshBuffer {
	void *data;
	usize index, count, capacity;

	MeshBuffer() = default;
	~MeshBuffer() = default;

	MeshBuffer(const MeshBuffer &other) = delete;
	MeshBuffer(MeshBuffer &&other) = default;
	MeshBuffer &operator=(const MeshBuffer &other) = delete;
	MeshBuffer &operator=(MeshBuffer &&other) = default;
};

struct Mesh {
	Chunk *chunk;
	MeshBuffer data, faces, indices;
	usize vertex_count;
	VAO vao;
	VBO vbo, ibo;

	Mesh() = default;

	Mesh(Chunk *chunk);
	~Mesh() = default;

	Mesh(const Mesh &other) = delete;
	Mesh(Mesh &&other) = default;
	Mesh &operator=(const Mesh &other) = delete;
	Mesh &operator=(Mesh &&other) = default;

	inline void depth_sort(glm::vec3 center);
	inline void prepare(usize global_buffers_index);
	inline void finalize(bool depth_sort);
	inline void render();
};

constexpr u16 FACE_INDICES[] = { 1, 0, 3, 1, 3, 2 };
constexpr u16 UNIQUE_INDICES[] = { 1, 0, 5, 2 };

constexpr u16 CUBE_INDICES[] = {
	1, 0, 3, 1, 3, 2,
	4, 5, 6, 4, 5, 7,
	5, 1, 2, 5, 2, 6,
	0, 4, 7, 0, 7, 3,
	2, 3, 7, 2, 7, 6,
	5, 4, 0, 5, 0, 1,
};

constexpr u16 SPRITE_INDICES[] = {
	3, 0, 5, 3, 5, 6,
	4, 2, 1, 4, 2, 7,
};

constexpr f32 CUBE_VERTICES[] = {
	0, 0, 0,
	1, 0, 0,
	1, 1, 0,
	0, 1, 0,

	0, 0, 1,
	1, 0, 1,
	1, 1, 1,
	0, 1, 1,
};

constexpr f32 CUBE_UVS[] = {
	1, 0,
	0, 0,
	0, 1,
	1, 1,
};

constexpr auto DATA_BUFFER_SIZE = (16 * 256 * 16) * 8 * 5 * sizeof(f32);
constexpr auto INDICES_BUFFER_SIZE = (16 * 256 * 16) * 36 * sizeof(u16);
constexpr auto FACES_BUFFER_SIZE = (16 * 256 * 16) * 96 * sizeof(u16);

#define chunk_foreach(_pname)\
	glm::ivec3 _pname = glm::ivec3(0);\
	for (usize x = 0; x < CHUNK_SIZE.x; x++)\
		for (usize y = 0; y < CHUNK_SIZE.y; y++)\
			for (usize z = 0;\
				z < CHUNK_SIZE.z &&\
				((_pname.x = x) != INT32_MAX) &&\
				((_pname.y = y) != INT32_MAX) &&\
				((_pname.z = z) != INT32_MAX);\
				z++)

#define chunk_pos_to_index(p)\
	(p.x * CHUNK_SIZE.y * CHUNK_SIZE.z + p.y * CHUNK_SIZE.z + p.z)

static int depth_sort_cmp(const Face *a, const Face *b) {
	return static_cast<int>(-glm::sign(a->distance - b->distance));
}

enum class MeshPass {
	FULL,
	TRANSPARENCY,
};

struct Chunk {
	Chunk() = default;

	explicit Chunk(World *world, glm::ivec3 offset)
		: world(world), offset(offset), base(this), transparent(this) {
		this->position = offset * CHUNK_SIZE;
		this->data = std::make_unique<u32[]>(CHUNK_VOLUME);
	}

	~Chunk() = default;

	Chunk(const Chunk &other) = delete;
	Chunk &operator=(const Chunk &other) = delete;

	Chunk(Chunk &&other)
		: offset(other.offset), 
		base(std::move(other.base)),
		transparent(std::move(other.transparent)),
		position(other.position),
		world(std::move(other.world)),
		data(std::move(other.data))
	{}

	Chunk &operator=(Chunk &&other) {
		assert(this != &other);
		
		this->world = std::move(other.world);
		this->data = std::move(other.data);
		this->base = std::move(other.base);
		this->transparent = std::move(other.transparent);
		this->position = other.position;
		this->offset = other.offset;

		other.position = glm::ivec3(0);
		other.offset = glm::ivec3(0);
		return *this;
	}

	inline void set_data(glm::ivec3 pos, u32 data);
	inline u32 get_data(glm::ivec3 pos) const;
	inline void render();
	inline void render_transparent();
	inline void update();
	inline void tick();
	
	inline bool in_bounds(glm::ivec3 position) const {
		return position.x >= 0 && position.y >= 0 && position.z >= 0 &&
			position.x < CHUNK_SIZE.x && position.y < CHUNK_SIZE.y && 
			position.z < CHUNK_SIZE.z;
	}

	inline bool on_bounds(glm::ivec3 position) const {
		return position.x == 0 || position.z == 0 || position.x == (CHUNK_SIZE.x - 1) ||
			position.z == (CHUNK_SIZE.z - 1);
	}

	inline void get_bordering_chunks(glm::ivec3 pos, std::span<Chunk *, 2> dest);

	inline World *get_world() {
		return this->world;
	}

	inline glm::vec3 get_pos() const {
		return this->position;
	}

	inline glm::ivec3 get_offset() const {
		return this->offset;
	}

private:
	inline void emit_sprite(
		Mesh *mesh, glm::vec3 position, glm::vec2 uv_offset, glm::vec2 uv_unit) const;
	
	inline void emit_face(
		Mesh *mesh, glm::vec3 position, Direction direction, glm::vec2 uv_offset,
		glm::vec2 uv_unit, bool transparent, bool shorten_y) const;

	inline void mesh(MeshPass pass);

	World *world;
	std::unique_ptr<u32[]> data;
	glm::ivec3 offset, position;
	
	bool dirty;
	bool depth_sort;

	Mesh base, transparent;
};

struct GlobalBuffer {
	std::array<f32, DATA_BUFFER_SIZE> data;
	std::array<u16, INDICES_BUFFER_SIZE> indices;
	std::array<Face, FACES_BUFFER_SIZE> faces;

	inline void *operator[](usize i) const {
		switch (i) {
			case 0:
				return (void *) this->data.data();
			case 1:
				return (void *) this->indices.data();
			case 2:
				return (void *) this->faces.data();
			default:
				return nullptr;
		}
	}
};

extern GlobalBuffer global_buffers[3];