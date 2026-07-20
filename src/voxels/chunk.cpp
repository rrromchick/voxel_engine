#include "chunk.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

Chunk::Chunk(int xpos, int ypos, int zpos) : x(xpos), y(ypos), z(zpos) {
	voxels = std::make_unique<voxel[]>(Chunk::VOLUME);
    
    for (int z = 0; z < Chunk::DEPTH; z++) {
        for (int x = 0; x < Chunk::WIDTH; x++) {
            auto real_x = x + this->x * Chunk::WIDTH;
            auto real_z = z + this->z * Chunk::DEPTH;

            for (int y = 0; y < Chunk::HEIGHT; y++) {
                auto real_y = y + this->y * Chunk::HEIGHT;
                int id = glm::perlin(glm::vec3(real_x * 0.0125f, real_y * 0.0125f, 
                    real_z * 0.0125f)) > 0.1f;

                if (real_y <= 2) {
                    id = 2;
                }
                voxels[(y * Chunk::DEPTH + z) * Chunk::WIDTH + x].id = id;
            }
        }
    }
}