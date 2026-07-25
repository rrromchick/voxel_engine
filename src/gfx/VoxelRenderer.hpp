#pragma once

#include <vector>
#include <array>
#include <memory>

struct Chunk;
struct Mesh;

struct VoxelRenderer {
	explicit VoxelRenderer(std::size_t capacity);
	~VoxelRenderer() = default;

	VoxelRenderer(const VoxelRenderer &other) = delete;
	VoxelRenderer &operator=(const VoxelRenderer &other) = delete;
	VoxelRenderer(VoxelRenderer &&other) noexcept = default;
	VoxelRenderer &operator=(VoxelRenderer &&other) noexcept = default;

	std::unique_ptr<Mesh> render(Chunk *chunk,
        const std::vector<Chunk*> &chunks);

private:
	std::size_t capacity;
	std::vector<float> buffer;
};