#include "WorldGenerator.hpp"
#include "Chunk.hpp"
#include "Block.hpp"
#include <glm/gtc/noise.hpp>
#include <algorithm>
#include <cmath>

constexpr int WATER_LEVEL = 64;

float WorldGenerator::hash_noise(int x, int y, int z, uint32_t salt) const {
    uint32_t h = salt ^ (x * 73856093) ^ (y * 19349663) ^ (z * 83492791);
    h = (h ^ 61) ^ (h >> 16);
    h += (h << 3);
    h ^= (h >> 4);
    h *= 0x27d4eb2d;
    h ^= (h >> 15);
    return static_cast<float>(h) / static_cast<float>(0xFFFFFFFF);
}

int WorldGenerator::rand_range_local(int x, int y, int z, uint32_t salt, int min, int max) const {
    float raw = hash_noise(x, y, z, salt);
    return min + static_cast<int>(raw * (max - min + 1));
}

bool WorldGenerator::rand_chance_local(int x, int y, int z, uint32_t salt, float chance) const {
    return hash_noise(x, y, z, salt) <= chance;
}

float WorldGenerator::radial2i(glm::vec2 c, glm::vec2 r, glm::vec2 v) const {
    return glm::length(c - v) / glm::length(r);
}

float WorldGenerator::radial3i(glm::vec3 c, glm::vec3 r, glm::vec3 v) const {
    return glm::length(c - v) / glm::length(r);
}

float WorldGenerator::octave_compute(int octaves, int offset, float seed, float x, float z) const {
    float u = 1.0f;
    float v = 0.0f;
    for (int i = 0; i < octaves; i++) {
        auto sample_z = seed + static_cast<float>(i) + static_cast<float>(offset * 32);
        float n = glm::perlin(glm::vec3(x / u, z / u, sample_z));
        v += n * u;
        u *= 2.0f;
    }
    return v;
}

float WorldGenerator::combined_compute(int n_oct, int n_off, int m_oct, int m_off, float seed, float x, float z) const {
    float m_val = octave_compute(m_oct, m_off, seed, x, z);
    return octave_compute(n_oct, n_off, seed, x + m_val, z);
}   

void WorldGenerator::set_voxel_safe(Chunk *chunk, int local_x, int local_y, int local_z, uint8_t block_id) const {
    if (local_x < 0 || local_x >= Chunk::WIDTH ||
        local_y < 0 || local_y >= Chunk::HEIGHT ||
        local_z < 0 || local_z >= Chunk::DEPTH) {
        return;
    }
    int index = (local_y * Chunk::DEPTH + local_z) * Chunk::WIDTH + local_x;
    chunk->voxels[index].id = block_id;
}

uint8_t WorldGenerator::get_voxel_safe(Chunk *chunk, int local_x, int local_y, int local_z) const {
    if (local_x < 0 || local_x >= Chunk::WIDTH ||
        local_y < 0 || local_y >= Chunk::HEIGHT ||
        local_z < 0 || local_z >= Chunk::DEPTH) {
        return BlockId::AIR;
    }
    int index = (local_y * Chunk::DEPTH + local_z) * Chunk::WIDTH + local_x;
    return chunk->voxels[index].id;
}

void WorldGenerator::tree(Chunk *chunk, int x, int y, int z) const {
    // Проверяем локальные границы внутри одного чанка
    uint8_t under = get_voxel_safe(chunk, x, y - 1, z);
    if (under != BlockId::GRASS && under != BlockId::DIRT) return;

    int h = rand_range_local(chunk->x, y, chunk->z, 12, 4, 5);
    for (int yy = y; yy <= (y + h); yy++) {
        set_voxel_safe(chunk, x, yy, z, BlockId::LOG);
    }

    int lh = rand_range_local(chunk->x, y, chunk->z, 34, 2, 3);
    for (int xx = (x - 2); xx <= (x + 2); xx++) {
        for (int zz = (z - 2); zz <= (z + 2); zz++) {
            for (int yy = (y + h); yy <= (y + h + 1); yy++) {
                int c = (xx == (x - 2) || xx == (x + 2)) + (zz == (z - 2) || zz == (z + 2));
                if (!(xx == x && zz == z) && c < 2) {
                    if (get_voxel_safe(chunk, xx, yy, zz) == BlockId::AIR) {
                        set_voxel_safe(chunk, xx, yy, zz, BlockId::LEAVES);
                    }
                }
            }
        }
    }

    for (int xx = (x - 1); xx <= (x + 1); xx++) {
        for (int zz = (z - 1); zz <= (z + 1); zz++) {
            for (int yy = (y + h + 2); yy <= (y + h + lh); yy++) {
                int c = (xx == (x - 1) || xx == (x + 1)) + (zz == (z - 1) || zz == (z + 1));
                if (c < 2) {
                    if (get_voxel_safe(chunk, xx, yy, zz) == BlockId::AIR) {
                        set_voxel_safe(chunk, xx, yy, zz, BlockId::LEAVES);
                    }
                }
            }
        }
    }
}

void WorldGenerator::flowers(Chunk *chunk, int x, int y, int z) const {
    uint8_t flower = rand_chance_local(chunk->x, y, chunk->z, 55, 0.6f) ? BlockId::ROSE : BlockId::BUTTERCUP;
    if (get_voxel_safe(chunk, x, y, z) == BlockId::GRASS) {
        set_voxel_safe(chunk, x, y + 1, z, flower);    
    }
}

void WorldGenerator::orevein(Chunk *chunk, int x, int y, int z, uint8_t block) const {
    int h = rand_range_local(chunk->x, y, chunk->z, 77, 5, std::max(6, y - 4));
    if (get_voxel_safe(chunk, x, h, z) == BlockId::STONE) {
        set_voxel_safe(chunk, x, h, z, block);
    }
}

void WorldGenerator::lavapool(Chunk *chunk, int x, int y, int z) const {
    int h = y - 1;
    if (get_voxel_safe(chunk, x, h, z) == BlockId::STONE) {
        set_voxel_safe(chunk, x, h, z, BlockId::LAVA);
    }
}

void WorldGenerator::generate_terrain(Chunk *chunk) const {
    constexpr float seed = 2.0f;
    int chunk_world_x = chunk->x * Chunk::WIDTH;
    int chunk_world_y = chunk->y * Chunk::HEIGHT;
    int chunk_world_z = chunk->z * Chunk::DEPTH;
    constexpr float base_scale = 1.3f;

    for (int x = 0; x < Chunk::WIDTH; x++) {
        for (int z = 0; z < Chunk::DEPTH; z++) {
            auto wx = static_cast<float>(chunk_world_x + x);
            auto wz = static_cast<float>(chunk_world_z + z);

            float cs0 = combined_compute(8, 1, 8, 2, seed, wx * base_scale, wz * base_scale);
            float cs1 = combined_compute(8, 3, 8, 4, seed, wx * base_scale, wz * base_scale);

            auto hl = static_cast<int>((cs0 / 6.0f) - 4.0f);
            auto hh = static_cast<int>((cs1 / 5.0f) + 6.0f);

            float t = octave_compute(6, 0, seed, wx, wz);
            float r = octave_compute(6, 1, seed, wx / 4.0f, wz / 4.0f) / 32.0f;

            int hr = (t > 0) ? hl : std::max(hh, hl);
            int h = hr + WATER_LEVEL;

            Biome biome = (h < WATER_LEVEL) ? Biome::OCEAN : ((t < 0.08f && h < WATER_LEVEL + 2) ? Biome::BEACH : Biome::PLAINS);
            auto d = static_cast<int>(r * 1.4f + 5.0f);

            uint8_t top_block = BlockId::GRASS;
            if (biome == Biome::OCEAN) {
                top_block = (r > 0.8f) ? BlockId::GRAVEL : ((r > 0.3f) ? BlockId::SAND : ((r > 0.15f && t < 0.08f) ? BlockId::CLAY : BlockId::DIRT));
            } else if (biome == Biome::BEACH) {
                top_block = BlockId::SAND;
            } else if (biome == Biome::PLAINS) {
                top_block = (t > 4.0f && r > 0.78f) ? BlockId::GRAVEL : BlockId::GRASS;
            }

            for (int y = 0; y < Chunk::HEIGHT; y++) {
                int world_y = chunk_world_y + y;
                int index = (y * Chunk::DEPTH + z) * Chunk::WIDTH + x;

                if (world_y < h) {
                    if (world_y == (h - 1)) {
                        chunk->voxels[index].id = top_block;
                    } else if (world_y > (h - d)) {
                        chunk->voxels[index].id = (top_block == BlockId::GRASS) ? BlockId::DIRT : top_block;
                    } else {
                        chunk->voxels[index].id = BlockId::STONE;
                    }
                } else if (world_y < WATER_LEVEL) {
                    chunk->voxels[index].id = BlockId::WATER;
                } else {
                    chunk->voxels[index].id = BlockId::AIR;
                }
            }
        }
    }
}

void WorldGenerator::carve_caves(Chunk *chunk) const {
    int chunk_world_x = chunk->x * Chunk::WIDTH;
    int chunk_world_y = chunk->y * Chunk::HEIGHT;
    int chunk_world_z = chunk->z * Chunk::DEPTH;
    constexpr float scale = 0.02f;
    constexpr float threshold = 0.65f;

    for (int x = 0; x < Chunk::WIDTH; x++) {
        int world_x = chunk_world_x + x;
        for (int z = 0; z < Chunk::DEPTH; z++) {
            int world_z = chunk_world_z + z;
            for (int y = 0; y < Chunk::HEIGHT; y++) {
                int world_y = chunk_world_y + y;

                if (world_y <= 4 || world_y >= 256) continue;

                glm::vec3 pos = glm::vec3(world_x, world_y, world_z) * scale;
                float noise_val = glm::simplex(pos);

                if (noise_val > threshold) {
                    int index = (y * Chunk::DEPTH + z) * Chunk::WIDTH + x;
                    chunk->voxels[index].id = BlockId::AIR;
                }
            }
        }
    }
}

void WorldGenerator::decorate(Chunk *chunk) const {
    int chunk_world_x = chunk->x * Chunk::WIDTH;
    int chunk_world_y = chunk->y * Chunk::HEIGHT;
    int chunk_world_z = chunk->z * Chunk::DEPTH;
    constexpr float seed = 2.0f;
    constexpr float base_scale = 1.3f;

    for (int x = 0; x < Chunk::WIDTH; x++) {
        for (int z = 0; z < Chunk::DEPTH; z++) {
            auto wx = static_cast<float>(chunk_world_x + x);
            auto wz = static_cast<float>(chunk_world_z + z);

            float cs0 = combined_compute(8, 1, 8, 2, seed, wx * base_scale, wz * base_scale);
            float cs1 = combined_compute(8, 3, 8, 4, seed, wx * base_scale, wz * base_scale);

            auto hl = static_cast<int>((cs0 / 6.0f) - 4.0f);
            auto hh = static_cast<int>((cs1 / 5.0f) + 6.0f);
            float t = octave_compute(6, 0, seed, wx, wz);

            int hr = (t > 0) ? hl : std::max(hh, hl);
            int h = hr + WATER_LEVEL;

            int local_y = (h - 1) - chunk_world_y;

            bool is_plains = !(h < WATER_LEVEL) && !(t < 0.08f && h < WATER_LEVEL + 2);

            if (local_y >= 0 && local_y < Chunk::HEIGHT) {
                if (rand_chance_local(chunk_world_x + x, h, chunk_world_z + z, 101, 0.02f)) {
                    orevein(chunk, x, local_y, z, BlockId::COAL);
                }
                if (rand_chance_local(chunk_world_x + x, h, chunk_world_z + z, 202, 0.02f)) {
                    orevein(chunk, x, local_y, z, BlockId::COPPER);
                }
                if (h <= (WATER_LEVEL + 3) && t < 0.1f && rand_chance_local(chunk_world_x + x, h, chunk_world_z + z, 303, 0.001f)) {
                    lavapool(chunk, x, local_y, z);
                }
                if (is_plains && rand_chance_local(chunk_world_x + x, h, chunk_world_z + z, 404, 0.005f)) {
                    tree(chunk, x, local_y, z);
                }
                if (is_plains && rand_chance_local(chunk_world_x + x, h, chunk_world_z + z, 505, 0.0085f)) {
                    flowers(chunk, x, local_y, z);
                }
            }
        }
    }
}