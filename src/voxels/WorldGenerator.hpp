#pragma once

#include <vector>
#include <random>
#include <cstdint>
#include <glm/glm.hpp>

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
    WorldGenerator() = default;
    ~WorldGenerator() = default;

    WorldGenerator(const WorldGenerator &other) = delete;
    WorldGenerator(WorldGenerator &&other) = default;
    WorldGenerator &operator=(const WorldGenerator &other) = delete;
    WorldGenerator &operator=(WorldGenerator &&other) = default;

    void generate_terrain(Chunk *chunk) const;
    void carve_caves(Chunk *chunk) const;
    void decorate(Chunk *chunk, const std::vector<Chunk*> &chunks) const;

private:
    mutable std::mt19937 rng;

    float radial2i(glm::vec2 c, glm::vec2 r, glm::vec2 v) const;
    float radial3i(glm::vec3 c, glm::vec3 r, glm::vec3 v) const;
    int rand_range(int min, int max) const;
    bool rand_chance(float chance) const;

    float octave_compute(int octaves, int offset, float seed, float x, float z) const;
    float combined_compute(int n_oct, int n_off, int m_oct, int m_off, float seed, float x, float z) const;

    void set_voxel_safe(Chunk *chunk, int local_x, int world_y, int local_z, uint8_t block_id) const;
    uint8_t get_voxel_safe(Chunk *chunk, int local_x, int world_y, int local_z) const;

    void set_voxel_neighbor(const std::vector<Chunk*> &chunks, Chunk *origin, int x, int world_y, int z, uint8_t block_id) const;
    uint8_t get_voxel_neighbor(const std::vector<Chunk*> &chunks, Chunk *origin, int x, int world_y, int z) const;

    void tree(const std::vector<Chunk*> &chunks, Chunk *origin, int x, int y, int z) const;
    void flowers(const std::vector<Chunk*> &chunks, Chunk *origin, int x, int y, int z) const;
    void orevein(const std::vector<Chunk*> &chunks, Chunk *origin, int x, int y, int z, uint8_t block) const;
    void lavapool(const std::vector<Chunk*> &chunks, Chunk *origin, int x, int y, int z) const;
};