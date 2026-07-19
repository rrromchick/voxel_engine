#include "chunk.hpp"

Chunk::Chunk(int xpos, int ypos, int zpos) : x(xpos), y(ypos), z(zpos) {
	voxels = std::make_unique<voxel[]>(Chunk::VOLUME);
    for (int y = 0; y < Chunk::HEIGHT; y++) {
        for (int z = 0; z < Chunk::DEPTH; z++) {
            for (int x = 0; x < Chunk::WIDTH; x++) {
                int real_x = x + this->x * Chunk::WIDTH;
                int real_y = y + this->y * Chunk::HEIGHT;
                int real_z = z + this->z * Chunk::DEPTH;
                
                int id = real_y <= (sin(real_x * 0.1f) * 0.5f + 0.5f) * 10;
                if (real_y <= 2) {
                    id = 2;
                }
                voxels[(y * Chunk::DEPTH + z) * Chunk::WIDTH + x].id = id;
            }
        }
    }
}