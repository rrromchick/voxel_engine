#include "WorldGenerator.hpp"
#include "Chunk.hpp"
#include "Block.hpp"
#include <glm/gtc/noise.hpp>
#include <algorithm>
#include <cmath>

constexpr int WATER_LEVEL = 64;

float WorldGenerator::radial2i(glm::vec2 c, glm::vec2 r, glm::vec2 v) const {
    return glm::length(c - v) / glm::length(r);
}

float WorldGenerator::radial3i(glm::vec3 c, glm::vec3 r, glm::vec3 v) const {
    return glm::length(c - v) / glm::length(r);
}

int WorldGenerator::rand_range(int min, int max) const {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

bool WorldGenerator::rand_chance(float chance) const {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(rng) <= chance;
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

void WorldGenerator::set_voxel_safe(Chunk *chunk, int local_x, int world_y, int local_z, uint8_t block_id) const {
    int chunk_world_y = chunk->y * Chunk::HEIGHT;
    int local_y = world_y - chunk_world_y;

    if (local_x < 0 || local_x >= Chunk::WIDTH ||
        local_y < 0 || local_y >= Chunk::HEIGHT ||
        local_z < 0 || local_z >= Chunk::DEPTH) {
        return;
    }

    int index = (local_y * Chunk::DEPTH + local_z) * Chunk::WIDTH + local_x;
    chunk->voxels[index].id = block_id;
}

uint8_t WorldGenerator::get_voxel_safe(Chunk *chunk, int local_x, int world_y, int local_z) const {
    int chunk_world_y = chunk->y * Chunk::HEIGHT;
    int local_y = world_y - chunk_world_y;

    if (local_x < 0 || local_x >= Chunk::WIDTH ||
        local_y < 0 || local_y >= Chunk::HEIGHT ||
        local_z < 0 || local_z >= Chunk::DEPTH) {
        return BlockId::AIR;
    }

    int index = (local_y * Chunk::DEPTH + local_z) * Chunk::WIDTH + local_x;
    return chunk->voxels[index].id;
}

void WorldGenerator::set_voxel_neighbor(const std::vector<Chunk*> &chunks, Chunk *origin, int x, int world_y, int z, uint8_t block_id) const {
    auto cdiv = [](int val, int size) -> int {
        int res = val / size;
        int rem = val % size;
        if (rem != 0 && ((val ^ size) < 0)) res--;
        return res;
    };

    auto local = [](int val, int size) -> int {
        int rem = val % size;
        return rem < 0 ? rem + size : rem;
    };

    int target_local_y = world_y - (origin->y * Chunk::HEIGHT);

    int cx = cdiv(x, Chunk::WIDTH) + 1;
    int cy = cdiv(target_local_y, Chunk::HEIGHT) + 1;
    int cz = cdiv(z, Chunk::DEPTH) + 1;

    if (cx < 0 || cx >= 3 || cy < 0 || cy >= 3 || cz < 0 || cz >= 3) return;

    auto *target = chunks[(cy * 3 + cz) * 3 + cx];
    if (!target) return;

    int lx = local(x, Chunk::WIDTH);
    int ly = local(target_local_y, Chunk::HEIGHT);
    int lz = local(z, Chunk::DEPTH);

    int index = (ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx;
    target->voxels[index].id = block_id;
}

uint8_t WorldGenerator::get_voxel_neighbor(const std::vector<Chunk*> &chunks, Chunk *origin, int x, int world_y, int z) const {
    auto cdiv = [](int val, int size) -> int {
        int res = val / size;
        int rem = val % size;
        if (rem != 0 && ((val ^ size) < 0)) res--;
        return res;
    };

    auto local = [](int val, int size) -> int {
        int rem = val % size;
        return rem < 0 ? rem + size : rem;
    };

    int target_local_y = world_y - (origin->y * Chunk::HEIGHT);
    
    int cx = cdiv(x, Chunk::WIDTH) + 1;
    int cy = cdiv(target_local_y, Chunk::HEIGHT) + 1;
    int cz = cdiv(z, Chunk::DEPTH) + 1;

    if (cx < 0 || cx >= 3 || cy < 0 || cy >= 3 || cz < 0 || cz >= 3) return BlockId::AIR;

    auto *target = chunks[(cy * 3 + cz) * 3 + cx];
    if (!target) return BlockId::AIR;

    int lx = local(x, Chunk::WIDTH);
    int ly = local(target_local_y, Chunk::HEIGHT);
    int lz = local(z, Chunk::DEPTH);

    int index = (ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx;
    return target->voxels[index].id;
}

void WorldGenerator::tree(const std::vector<Chunk*> &chunks, Chunk *origin, int x, int y, int z) const {
    uint8_t under = get_voxel_neighbor(chunks, origin, x, y - 1, z);
    if (under != BlockId::GRASS && under != BlockId::DIRT) return;

    int h = rand_range(3, 5);
    for (int yy = y; yy <= (y + h); yy++) {
        set_voxel_neighbor(chunks, origin, x, yy, z, BlockId::LOG);
    }

    int lh = rand_range(2, 3);
    for (int xx = (x - 2); xx <= (x + 2); xx++) {
        for (int zz = (z - 2); zz <= (z + 2); zz++) {
            for (int yy = (y + h); yy <= (y + h + 1); yy++) {
                int c = (xx == (x - 2) || xx == (x + 2)) + (zz == (z - 2) || zz == (z + 2));
                bool corner = (c == 2);

                if ((!(xx == x && zz == z) || yy > (y + h) &&
                    !(corner && yy == (y + h + 1) && rand_chance(0.4f)))) {
                    set_voxel_neighbor(chunks, origin, xx, yy, zz, BlockId::LEAVES);
                }
            }
        }
    }

    for (int xx = (x - 1); xx <= (x + 1); xx++) {
        for (int zz = (z - 1); zz <= (z + 1); zz++) {
            for (int yy = (y + h + 2); yy <= (y + h + lh); yy++) {
                int c = (xx == (x - 1) || xx == (x + 1)) + (zz == (z - 1) || zz == (z + 1));
                bool corner = (c == 2);

                if (!(corner && yy == (y + h + lh) && rand_chance(0.8f))) {
                    set_voxel_neighbor(chunks, origin, xx, yy, zz, BlockId::LEAVES);
                }
            }
        }
    }
}

void WorldGenerator::flowers(const std::vector<Chunk*> &chunks, Chunk *origin, int x, int y, int z) const {
    uint8_t flower = rand_chance(0.6f) ? BlockId::ROSE : BlockId::BUTTERCUP;
    int s = rand_range(2, 6);
    int l = rand_range(s - 1, s + 1);
    int h = rand_range(s - 1, s + 1);

    for (int xx = (x - l); xx <= (x + l); xx++) {
        for (int zz = (z - h); zz <= (z + h); zz++) {
            if (get_voxel_neighbor(chunks, origin, xx, y, zz) == BlockId::GRASS && rand_chance(0.5f)) {
                set_voxel_neighbor(chunks, origin, xx, y + 1, zz, flower);    
            }
        }
    }
}

void WorldGenerator::orevein(const std::vector<Chunk*> &chunks, Chunk *origin, int x, int y, int z, uint8_t block) const {
    int h = rand_range(1, std::max(1, y - 4));
    int s = (block == BlockId::COAL) ? rand_range(2, 4) : rand_range(1, 3);
    int l = rand_range(s - 1, s + 1);
    int w = rand_range(s - 1, s + 1);
    int i = rand_range(s - 1, s + 1);

    for (int xx = (x - l); xx <= (x + l); xx++) {
        for (int zz = (z - w); zz <= (z + w); zz++) {
            for (int yy = (h - i); yy <= (h + i); yy++) {
                float d = 1.0f - radial3i(
                    glm::vec3(x, h, z),
                    glm::vec3(l + 1, w + 1, i + 1),
                    glm::vec3(xx, yy, zz));

                if (get_voxel_neighbor(chunks, origin, xx, yy, zz) == BlockId::STONE && rand_chance(0.2f + d * 0.7f)) {
                    set_voxel_neighbor(chunks, origin, xx, yy, zz, block);
                }
            }
        }
    }
}

void WorldGenerator::lavapool(const std::vector<Chunk*> &chunks, Chunk *origin, int x, int y, int z) const {
    int h = y - 1;
    int s = rand_range(1, 5);
    int l = rand_range(s - 1, s + 1);
    int w = rand_range(s - 1, s + 1);

    for (int xx = (x - l); xx <= (x + l); xx++) {
        for (int zz = (z - w); zz <= (z + w); zz++) {
            float d = 1.0f - radial2i(
                glm::vec2(x, z), glm::vec2(l + 1, w + 1), glm::vec2(xx, zz));
            
            if (rand_chance(0.2f + d * 0.95f)) {
                set_voxel_neighbor(chunks, origin, xx, h, zz, BlockId::LAVA);
            }
        }
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

            Biome biome;
            if (h < WATER_LEVEL) {
                biome = Biome::OCEAN;
            } else if (t < 0.08f && h < WATER_LEVEL + 2) {
                biome = Biome::BEACH;
            } else {
                biome = Biome::PLAINS;
            }

            auto d = static_cast<int>(r * 1.4f + 5.0f);

            uint8_t top_block = BlockId::GRASS;
            switch (biome) {
                case Biome::OCEAN:
                    if (r > 0.8f) top_block = BlockId::GRAVEL;
                    else if (r > 0.3f) top_block = BlockId::SAND;
                    else if (r > 0.15f && t < 0.08f) top_block = BlockId::CLAY;
                    else top_block = BlockId::DIRT;
                    break;
                case Biome::BEACH:
                    top_block = BlockId::SAND;
                    break;
                case Biome::PLAINS:
                    top_block = (t > 4.0f && r > 0.78f) ? BlockId::GRAVEL : BlockId::GRASS;
                    break;
                case Biome::MOUNTAIN:
                    if (r > 0.8f) top_block = BlockId::GRAVEL;
                    else if (r > 0.7f) top_block = BlockId::DIRT;
                    else top_block = BlockId::STONE;
                    break;
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
    constexpr float seed = 2.0f;
    int chunk_world_x = chunk->x * Chunk::WIDTH;
    int chunk_world_y = chunk->y * Chunk::HEIGHT;
    int chunk_world_z = chunk->z * Chunk::DEPTH;

    for (int x = 0; x < Chunk::WIDTH; x++) {
        for (int z = 0; z < Chunk::DEPTH; z++) {
            for (int y = 0; y < Chunk::HEIGHT; y++) {
                int world_y = chunk_world_y + y;
                int index = (y * Chunk::DEPTH + z) * Chunk::WIDTH + x;
                
                // Cave carving logic
            }
        }
    }
}

void WorldGenerator::decorate(Chunk *chunk, const std::vector<Chunk*> &chunks) const {
    constexpr float seed = 2.0f;

    uint32_t chunk_hash = (chunk->x * 73856093) ^ (chunk->y * 19349663) ^ (chunk->z * 83492791);
    rng.seed(static_cast<uint32_t>(seed) + chunk_hash);

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

            int hr = (t > 0) ? hl : std::max(hh, hl);
            int h = hr + WATER_LEVEL;

            bool is_plains = !(h < WATER_LEVEL) && !(t < 0.08f && h < WATER_LEVEL + 2);

            if (h >= chunk_world_y && h < chunk_world_y + Chunk::HEIGHT) {
                if (rand_chance(0.02f)) {
                    orevein(chunks, chunk, x, h, z, BlockId::COAL);
                }
                if (rand_chance(0.02f)) {
                    orevein(chunks, chunk, x, h, z, BlockId::COPPER);
                }

                if (h <= (WATER_LEVEL + 3) && t < 0.1f && rand_chance(0.001f)) {
                    lavapool(chunks, chunk, x, h, z);
                }

                if (is_plains && rand_chance(0.005f)) {
                    tree(chunks, chunk, x, h, z);
                }

                if (is_plains && rand_chance(0.0085f)) {
                    flowers(chunks, chunk, x, h, z);
                }
            }
        }
    }
}