#include "Chunk.hpp"
#include "Lightmap.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

Chunk::Chunk(int xpos, int ypos, int zpos) : x(xpos), y(ypos), z(zpos) {
	voxels = std::make_unique<voxel[]>(Chunk::VOLUME);
    lightmap = std::make_unique<Lightmap>();

    for (std::size_t i = 0; i < Chunk::VOLUME; i++) {
        voxels[i].id = 1;
    }
}

Chunk::~Chunk() = default;

bool Chunk::is_empty() const {
    int id = -1;
    for (std::size_t i = 0; i < Chunk::VOLUME; i++) {
        if (id != -1) {
            return false;
        } else {
            id = voxels[i].id;
        }
    }
    return true;
}