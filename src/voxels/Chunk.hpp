#pragma once

#include <memory>

struct voxel {
    uint8_t id;
};

struct Lightmap;

struct Chunk {
	static constexpr auto WIDTH = 16;
	static constexpr auto HEIGHT = 128;
	static constexpr auto DEPTH = 16;

	static constexpr auto VOLUME = 
		WIDTH * HEIGHT * DEPTH;

    bool modified = true;
	bool decorated = false;
    int x, y, z;

	std::unique_ptr<voxel[]> voxels;
    std::unique_ptr<Lightmap> lightmap;

	explicit Chunk(int xpos, int ypos, int zpos);
	~Chunk();

	Chunk(const Chunk &other) = delete;
	Chunk &operator=(const Chunk &other) = delete;
	Chunk(Chunk &&other) noexcept = default;
	Chunk &operator=(Chunk &&other) noexcept = default;

    bool is_empty() const;
};