#include "LightSolver.hpp"
#include "Lightmap.hpp"
#include "Chunks.hpp"
#include "Chunk.hpp"
#include "Global.hpp"

void LightSolver::add(int x, int y, int z, int emission) {
    if (emission <= 1) return;

    LightEntry entry { x, y, z, emission };
    add_queue.push(entry);

    auto *chunk = global.chunks->get_chunk_by_voxel(entry.x, entry.y, entry.z);    
    chunk->modified = true;
    chunk->lightmap->set(entry.x - chunk->x * Chunk::WIDTH, entry.y - chunk->y * Chunk::HEIGHT, 
        entry.z - chunk->z * Chunk::DEPTH, channel, entry.light);
}

void LightSolver::add(int x, int y, int z) {
    add(x, y, z, global.chunks->get_light(x, y, z, channel));
}

void LightSolver::remove(int x, int y, int z) {
    auto *chunk = global.chunks->get_chunk_by_voxel(x, y, z);
    if (chunk == nullptr) return;

    int light = chunk->lightmap->get(
        x - chunk->x * Chunk::WIDTH, y - chunk->y * Chunk::HEIGHT, 
        z - chunk->z * Chunk::DEPTH, channel);
    if (light == 0) {
        return;
    }

    LightEntry entry { x, y, z, light };
    rem_queue.push(entry);

    chunk->lightmap->set(entry.x - chunk->x * Chunk::WIDTH, entry.y - chunk->y * Chunk::HEIGHT,
        entry.z - chunk->z * Chunk::DEPTH, channel, 0);
}

void LightSolver::solve() {
    constexpr std::array<int, 18> coords = {
        0, 0, 1,
        0, 0, -1, 
        0, 1, 0,
        0, -1, 0,
        1, 0, 0,
        -1, 0, 0
    };

    auto *chunks = global.chunks.get();

    while (!rem_queue.empty()) {
        auto entry = rem_queue.front();
        rem_queue.pop();

        for (usize i = 0; i < 6; i++) {
            int x = entry.x + coords[i * 3 + 0];
            int y = entry.y + coords[i * 3 + 1];
            int z = entry.z + coords[i * 3 + 2];
            auto *chunk = chunks->get_chunk_by_voxel(x, y, z);
            if (chunk) {
                auto light = chunks->get_light(x, y, z, channel);
                if (light != 0 && light == entry.light - 1) {
                    LightEntry nentry { x, y, z, light };
                    rem_queue.push(nentry);
                    chunk->lightmap->set(x - chunk->x * Chunk::WIDTH,
                        y - chunk->y * Chunk::HEIGHT, z - chunk->z * Chunk::DEPTH, channel, 0);
                    chunk->modified = true;
                } else if (light >= entry.light) {
                    LightEntry nentry { x, y, z, light };
                    add_queue.push(nentry);
                }
            }
        }
    }

    while (!add_queue.empty()) {
        auto entry = add_queue.front();
        add_queue.pop();

        if (entry.light <= 1) {
            continue;
        }

        for (usize i = 0; i < 6; i++) {
            int x = entry.x + coords[i * 3 + 0];
            int y = entry.y + coords[i * 3 + 1];
            int z = entry.z + coords[i * 3 + 2];
            auto *chunk = chunks->get_chunk_by_voxel(x, y, z);
            if (chunk) {
                auto light = chunks->get_light(x, y, z, channel);
                auto *v = chunks->get(x, y, z);

                if (v->id == 0 && light + 2 <= entry.light) {
                    chunk->lightmap->set(x - chunk->x * Chunk::WIDTH,
                        y - chunk->y * Chunk::HEIGHT, z - chunk->z * Chunk::DEPTH, 
                        channel, entry.light - 1);
                    chunk->modified = true;
                    
                    LightEntry nentry { x, y, z, entry.light - 1 };
                    add_queue.push(nentry);
                }
            }
        }
    }
}