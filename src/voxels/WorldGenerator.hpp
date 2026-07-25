#pragma once

#include <memory>

struct voxel;

struct WorldGenerator {
    WorldGenerator();
    ~WorldGenerator();

    WorldGenerator(const WorldGenerator &other) = delete;
    WorldGenerator(WorldGenerator &&other) = default;
    WorldGenerator &operator=(const WorldGenerator &other) = delete;
    WorldGenerator &operator=(WorldGenerator &&other) = default;

    void generate(std::unique_ptr<voxel[]> &voxels, int x, int y, int z);
};