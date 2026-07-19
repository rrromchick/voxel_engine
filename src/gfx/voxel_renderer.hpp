#pragma once

#include "std.hpp"
#include "typedefs.hpp"

struct Chunk;
struct Mesh;

struct VoxelRenderer {
	explicit VoxelRenderer(usize capacity);
	~VoxelRenderer() = default;

	VoxelRenderer(const VoxelRenderer &other) = delete;
	VoxelRenderer &operator=(const VoxelRenderer &other) = delete;
	VoxelRenderer(VoxelRenderer &&other) noexcept = default;
	VoxelRenderer &operator=(VoxelRenderer &&other) noexcept = default;

	std::unique_ptr<Mesh> render(Chunk *chunk,
        const std::vector<Chunk*> &chunks);

private:
	usize capacity;
	std::vector<float> buffer;
};