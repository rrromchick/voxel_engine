#pragma once

#include <vector>
#include <cstdint>
#include <array>
#include <string>
#include <glm/glm.hpp>
#include <assert.h>

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

const std::array<std::string, 16> block_id_to_str = {{
    "AIR", "GRASS", "DIRT", "LAMP", "STONE", "SAND", "GRAVEL",
    "CLAY", "WATER", "LOG", "LEAVES", "ROSE", "BUTTERCUP", "COAL",
    "COPPER", "LAVA"
}};

static std::string block_id_to_string(const BlockId &id) {
    assert(id < block_id_to_str.size());
    return block_id_to_str[id];
}

struct WorldGenerator {
    WorldGenerator() = default;
    ~WorldGenerator() = default;

    WorldGenerator(const WorldGenerator &other) = delete;
    WorldGenerator(WorldGenerator &&other) = default;
    WorldGenerator &operator=(const WorldGenerator &other) = delete;
    WorldGenerator &operator=(WorldGenerator &&other) = default;

    void generate_terrain(Chunk *chunk) const;
    void carve_caves(Chunk *chunk) const;
    void decorate(Chunk *chunk) const;

private:
    float hash_noise(int x, int y, int z, uint32_t salt) const;
    int rand_range_local(int x, int y, int z, uint32_t salt, int min, int max) const;
    bool rand_chance_local(int x, int y, int z, uint32_t salt, float chance) const;

    float radial2i(glm::vec2 c, glm::vec2 r, glm::vec2 v) const;
    float radial3i(glm::vec3 c, glm::vec3 r, glm::vec3 v) const;

    float octave_compute(int octaves, int offset, float seed, float x, float z) const;
    float combined_compute(int n_oct, int n_off, int m_oct, int m_off, float seed, float x, float z) const;

    void set_voxel_safe(Chunk *chunk, int local_x, int local_y, int local_z, uint8_t block_id) const;
    uint8_t get_voxel_safe(Chunk *chunk, int local_x, int local_y, int local_z) const;

    void tree(Chunk *chunk, int x, int y, int z) const;
    void flowers(Chunk *chunk, int x, int y, int z) const;
    void orevein(Chunk *chunk, int x, int y, int z, uint8_t block) const;
    void lavapool(Chunk *chunk, int x, int y, int z) const;
};