#pragma once

#include <glm/glm.hpp>
#include <cstdint>

struct Chunk;

enum class Biome {
    OCEAN,
    PLAINS,
    BEACH,
    MOUNTAIN
};

enum BlockId : uint8_t {
    AIR = 0,
    GRASS = 1,
    DIRT = 2,
    LAMP = 3,
    STONE = 4,
    SAND = 5,
    GRAVEL = 6,
    CLAY = 7,
    WATER = 8,
    LOG = 9,
    LEAVES = 10,
    ROSE = 11,
    BUTTERCUP = 12,
    COAL = 13,
    COPPER = 14,
    LAVA = 15
};

struct WorldGenerator {
    explicit WorldGenerator(float seed = 1337.0f);
    ~WorldGenerator() = default;

    WorldGenerator(const WorldGenerator &other) = delete;
    WorldGenerator(WorldGenerator &&other) = default;
    WorldGenerator &operator=(const WorldGenerator &other) = delete;
    WorldGenerator &operator=(WorldGenerator &&other) = default;

    void generate(Chunk *chunk) const;

private:
    float seed = 1337.0f;

    float octave_noise(float x, float z, int octaves, float persistence, float scale) const;
    float noise3d(float x, float y, float z, float scale) const;
    float hash2d(int x, int z) const;
    void set_voxel_safe(Chunk *chunk, int x, int y, int z, uint8_t block_id) const;
    void place_tree(Chunk *chunk, int local_x, int surface_y, int local_z) const;
};