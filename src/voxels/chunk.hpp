#pragma once

#include "std.hpp"
#include "typedefs.hpp"

struct voxel {
	u8 id;
};

struct Chunk {
	static constexpr auto WIDTH = 16;
	static constexpr auto HEIGHT = 16;
	static constexpr auto DEPTH = 16;

	static constexpr auto VOLUME = 
		WIDTH * HEIGHT * DEPTH;

    bool modified = true;
    int x, y, z;

	std::unique_ptr<voxel[]> voxels;

	explicit Chunk(int xpos, int ypos, int zpos);
	~Chunk() = default;

	Chunk(const Chunk &other) = delete;
	Chunk &operator=(const Chunk &other) = delete;
	Chunk(Chunk &&other) noexcept = default;
	Chunk &operator=(Chunk &&other) noexcept = default;
};