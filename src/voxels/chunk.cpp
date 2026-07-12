#include "chunk.hpp"

Chunk::Chunk() {
	voxels = std::make_unique<voxel[]>(Chunk::VOLUME);
	for (int y = 0; y < Chunk::HEIGHT; y++) {
		for (int z = 0; z < Chunk::DEPTH; z++) {
			for (int x = 0; x < Chunk::WIDTH; x++) {
				int id = y <= (sin(x * 0.3f) * 0.5f + 0.5f) * 10;
				if (y <= 2) {
					id = 2;
				}

				voxels[(y * Chunk::DEPTH + z) * Chunk::WIDTH + x].id = id;
			}
		}
	}

	//for (int i = 0; i < Chunk::VOLUME; i++) {
	//	voxels[i].id = 1;
	//}
}