#include "WorldGenerator.hpp"
#include "Chunk.hpp"
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

WorldGenerator::WorldGenerator() = default;
WorldGenerator::~WorldGenerator() = default;

void WorldGenerator::generate(std::unique_ptr<voxel[]> &voxels, int cx, int cy, int cz) {
    for (int z = 0; z < Chunk::DEPTH; z++) {
        for (int x = 0; x < Chunk::WIDTH; x++) {
            int real_x = x + cx * Chunk::WIDTH;
            int real_z = z + cz * Chunk::DEPTH;
            float height = glm::perlin(glm::vec3(real_x * 0.0125f, real_z * 0.0125f, 0.0f));
            height += glm::perlin(glm::vec3(real_x * 0.025f, real_z * 0.025f, 0.0f)) * 0.5f;
            height *= 0.1f;
            height += 0.05f;
            for (int y = 0; y < Chunk::HEIGHT; y++) {
                int real_y = y + cy * Chunk::HEIGHT;
                auto noise = height;
                int id = noise / std::fmax(0.01f, real_y * 0.1f + 0.1f) > 0.1f;
                if (real_y <= 2) {
                    id = 2;
                }

                if (id == 0 && real_y == 14 && height <= 0.01f) {
                    id = 1;
                }
                voxels[(y * Chunk::DEPTH + z) * Chunk::WIDTH + x].id = id;
            }
        }
    }
}