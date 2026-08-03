#include "WorldGenerator.hpp"
#include "Chunk.hpp"
#include "Block.hpp"
#include <glm/gtc/noise.hpp>
#include <algorithm>
#include <cmath>

WorldGenerator::WorldGenerator(float seed) : seed(seed) {}

float WorldGenerator::octave_noise(float x, float z, int octaves, float persistence, float scale) const {
    float total = 0.0f;
    float frequency = scale;
    float amplitude = 1.0f;
    float max_value = 0.0f;

    for (int i = 0; i < octaves; i++) {
        glm::vec2 sample_pos = glm::vec2(x, z) * frequency + glm::vec2(seed + i * 123.45f);
        float n = (glm::perlin(sample_pos) + 1.0f) * 0.5f;
        
        total += n * amplitude;
        max_value += amplitude;

        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / max_value;
}

float WorldGenerator::noise3d(float x, float y, float z, float scale) const {
    glm::vec3 sample_pos = glm::vec3(x, y, z) * scale + glm::vec3(seed * 0.5f);
    return (glm::perlin(sample_pos) + 1.0f) * 0.5f;
}

float WorldGenerator::hash2d(int x, int z) const {
    int n = x * 374761393 + z * 668265263;
    n = (n ^ (n >> 13)) * 1274126177;
    return static_cast<float>(n & 0x7FFFFFFF) / static_cast<float>(0x7FFFFFFF);
}

void WorldGenerator::set_voxel_safe(Chunk* chunk, int x, int y, int z, uint8_t block_id) const {
    if (x < 0 || x >= Chunk::WIDTH || y < 0 || y >= Chunk::HEIGHT || z < 0 || z >= Chunk::DEPTH) {
        return;
    }
    int index = (y * Chunk::DEPTH + z) * Chunk::WIDTH + x;
    chunk->voxels[index].id = block_id;
}

void WorldGenerator::place_tree(Chunk* chunk, int local_x, int local_surface_y, int local_z) const {
    int trunk_height = 5;

    int leaves_bottom = local_surface_y + trunk_height - 2;
    int leaves_top = local_surface_y + trunk_height + 1;

    for (int ly = leaves_bottom; ly <= leaves_top; ly++) {
        int radius = (ly >= leaves_top - 1) ? 1 : 2;
        
        for (int lx = -radius; lx <= radius; lx++) {
            for (int lz = -radius; lz <= radius; lz++) {
                if (std::abs(lx) == radius && std::abs(lz) == radius && radius > 1) {
                    if (hash2d(local_x + lx, local_z + lz) > 0.4f) continue;
                }

                if (lx == 0 && lz == 0 && ly < local_surface_y + trunk_height) continue;
                set_voxel_safe(chunk, local_x + lx, ly, local_z + lz, BlockId::LEAVES);
            }
        }
    }

    for (int i = 1; i <= trunk_height; i++) {
        set_voxel_safe(chunk, local_x, local_surface_y + i, local_z, BlockId::LOG);
    }
}

void WorldGenerator::generate(Chunk* chunk) const {
    constexpr int SEA_LEVEL   = 36;
    constexpr int BASE_HEIGHT = 48;

    int chunk_world_x = chunk->x * Chunk::WIDTH;
    int chunk_world_y = chunk->y * Chunk::HEIGHT;
    int chunk_world_z = chunk->z * Chunk::DEPTH;

    for (int z = 0; z < Chunk::DEPTH; z++) {
        for (int x = 0; x < Chunk::WIDTH; x++) {
            float world_x = static_cast<float>(chunk_world_x + x);
            float world_z = static_cast<float>(chunk_world_z + z);

            float continental = octave_noise(world_x, world_z, 2, 0.5f, 0.0012f);
            float base_detail = octave_noise(world_x, world_z, 4, 0.5f, 0.008f);

            float mountain_raw = octave_noise(world_x, world_z, 5, 0.5f, 0.004f);
            float mountain_height = std::pow(mountain_raw, 2.0f) * 40.0f;

            int final_height = BASE_HEIGHT;

            if (continental < 0.22f) {
                float ocean_depth = (0.22f - continental) / 0.22f; 
                final_height = BASE_HEIGHT - static_cast<int>(ocean_depth * 20.0f + base_detail * 4.0f);
            } else if (continental < 0.55f) {
                float blend = (continental - 0.22f) / 0.33f;
                final_height = BASE_HEIGHT + static_cast<int>(base_detail * 10.0f + blend * 4.0f);
            } else {
                float blend = (continental - 0.55f) / 0.45f;
                final_height = BASE_HEIGHT + static_cast<int>(base_detail * 8.0f + mountain_height * blend);
            }

            for (int y = 0; y < Chunk::HEIGHT; y++) {
                int world_y = chunk_world_y + y;
                int index = (y * Chunk::DEPTH + z) * Chunk::WIDTH + x;

                if (world_y > final_height) {
                    chunk->voxels[index].id = (world_y <= SEA_LEVEL) ? BlockId::WATER : BlockId::AIR;
                } else if (world_y == final_height) {
                    if (world_y <= SEA_LEVEL + 2) {
                        chunk->voxels[index].id = BlockId::SAND;
                    } else if (world_y > 75) {
                        chunk->voxels[index].id = BlockId::STONE;
                    } else {
                        chunk->voxels[index].id = BlockId::GRASS;
                    }
                } else if (world_y > final_height - 4) {
                    if (world_y <= SEA_LEVEL + 2) {
                        chunk->voxels[index].id = BlockId::SAND;
                    } else if (world_y > 75) {
                        chunk->voxels[index].id = BlockId::STONE;
                    } else {
                        chunk->voxels[index].id = BlockId::DIRT;
                    }
                } else {
                    chunk->voxels[index].id = BlockId::STONE;
                }
            }
        }
    }

    for (int z = 0; z < Chunk::DEPTH; z++) {
        for (int x = 0; x < Chunk::WIDTH; x++) {
            float world_x = static_cast<float>(chunk_world_x + x);
            float world_z = static_cast<float>(chunk_world_z + z);

            for (int y = 0; y < Chunk::HEIGHT; y++) {
                int world_y = chunk_world_y + y;
                int index = (y * Chunk::DEPTH + z) * Chunk::WIDTH + x;

                if (chunk->voxels[index].id == BlockId::STONE) {
                    float coal_noise = noise3d(world_x, static_cast<float>(world_y), world_z, 0.08f);
                    float copper_noise = noise3d(world_x + 500.0f, static_cast<float>(world_y), world_z + 500.0f, 0.09f);

                    if (coal_noise > 0.72f && world_y < 60) {
                        chunk->voxels[index].id = BlockId::COAL;
                    } else if (copper_noise > 0.74f && world_y < 40) {
                        chunk->voxels[index].id = BlockId::COPPER;
                    }
                }
            }
        }
    }

    for (int z = 0; z < Chunk::DEPTH; z++) {
        for (int x = 0; x < Chunk::WIDTH; x++) {
            int world_x = chunk_world_x + x;
            int world_z = chunk_world_z + z;

            for (int y = Chunk::HEIGHT - 1; y >= 0; y--) {
                int world_y = chunk_world_y + y;
                int index = (y * Chunk::DEPTH + z) * Chunk::WIDTH + x;

                if (chunk->voxels[index].id == BlockId::GRASS && world_y > SEA_LEVEL) {
                    float rnd = hash2d(world_x, world_z);

                    if (rnd > 0.985f && x >= 2 && x <= Chunk::WIDTH - 3 && z >= 2 && z <= Chunk::DEPTH - 3) {
                        place_tree(chunk, x, y, z);
                    } else if (rnd < 0.04f && y + 1 < Chunk::HEIGHT) {
                        int flower_above_index = ((y + 1) * Chunk::DEPTH + z) * Chunk::WIDTH + x;
                        if (chunk->voxels[flower_above_index].id == BlockId::AIR) {
                            chunk->voxels[flower_above_index].id = (rnd < 0.02f) ? BlockId::ROSE : BlockId::BUTTERCUP;
                        }
                    }
                    break;
                }
            }
        }
    }
}