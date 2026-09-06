#include "LightSolver.hpp"
#include "Lightmap.hpp"
#include "Level.hpp"
#include "Chunk.hpp"
#include "Global.hpp"
#include "Block.hpp"

void LightSolver::add(int x, int y, int z, int emission) {
    if (emission <= 1 || !global.ecs || !global.ecs->level) return;

    auto *chunk = global.ecs->level->get_chunk_by_voxel(x, y, z);    
    if (!chunk || !chunk->lightmap) return;

    LightEntry entry { x, y, z, static_cast<unsigned char>(emission) };
    add_queue.push(entry);

    chunk->modified = true;
    chunk->lightmap->set(
        entry.x - chunk->x * Chunk::WIDTH, 
        entry.y - chunk->y * Chunk::HEIGHT, 
        entry.z - chunk->z * Chunk::DEPTH, 
        channel, 
        entry.light
    );
}

void LightSolver::add(int x, int y, int z) {
    if (!global.ecs || !global.ecs->level) return;
    int light = global.ecs->level->get_light(x, y, z, channel);
    if (light > 1) {
        add(x, y, z, light);
    }
}

void LightSolver::remove(int x, int y, int z) {
    if (!global.ecs || !global.ecs->level) return;
    auto *chunk = global.ecs->level->get_chunk_by_voxel(x, y, z);
    if (!chunk || !chunk->lightmap) return;

    int light = chunk->lightmap->get(
        x - chunk->x * Chunk::WIDTH, 
        y - chunk->y * Chunk::HEIGHT, 
        z - chunk->z * Chunk::DEPTH, 
        channel
    );
    if (light == 0) return;

    LightEntry entry { x, y, z, static_cast<unsigned char>(light) };
    rem_queue.push(entry);

    chunk->lightmap->set(
        entry.x - chunk->x * Chunk::WIDTH, 
        entry.y - chunk->y * Chunk::HEIGHT,
        entry.z - chunk->z * Chunk::DEPTH, 
        channel, 
        0
    );
}

void LightSolver::solve() {
    if (!global.ecs || !global.ecs->level) return;

    constexpr std::array<int, 18> coords = {
        0, 0, 1,
        0, 0, -1, 
        0, 1, 0,
        0, -1, 0,
        1, 0, 0,
        -1, 0, 0
    };

    auto *level = global.ecs->level.get();

    while (!rem_queue.empty()) {
        auto entry = rem_queue.front();
        rem_queue.pop();

        for (std::size_t i = 0; i < 6; i++) {
            int x = entry.x + coords[i * 3 + 0];
            int y = entry.y + coords[i * 3 + 1];
            int z = entry.z + coords[i * 3 + 2];
            auto *chunk = level->get_chunk_by_voxel(x, y, z);
            if (chunk && chunk->lightmap) {
                auto light = level->get_light(x, y, z, channel);
                if (light != 0 && light == entry.light - 1) {
                    LightEntry nentry { x, y, z, light };
                    rem_queue.push(nentry);
                    chunk->lightmap->set(
                        x - chunk->x * Chunk::WIDTH,
                        y - chunk->y * Chunk::HEIGHT, 
                        z - chunk->z * Chunk::DEPTH, 
                        channel, 
                        0
                    );
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

        if (entry.light <= 1) continue;

        for (std::size_t i = 0; i < 6; i++) {
            int x = entry.x + coords[i * 3 + 0];
            int y = entry.y + coords[i * 3 + 1];
            int z = entry.z + coords[i * 3 + 2];

            auto *chunk = level->get_chunk_by_voxel(x, y, z);
            if (chunk && chunk->lightmap) {
                auto *v = level->get_voxel(x, y, z);
                if (!v) continue; // Prevent null dereference

                if (v->id >= global.blocks.size() || !global.blocks[v->id]) continue;
                auto *block = global.blocks[v->id].get();

                auto light = level->get_light(x, y, z, channel);
                if (block && block->light_passing && light < entry.light - 1) {
                    chunk->lightmap->set(
                        x - chunk->x * Chunk::WIDTH,
                        y - chunk->y * Chunk::HEIGHT, 
                        z - chunk->z * Chunk::DEPTH,
                        channel, 
                        entry.light - 1
                    );
                    chunk->modified = true;

                    LightEntry nentry { x, y, z, static_cast<unsigned char>(entry.light - 1) };
                    add_queue.push(nentry);
                }
            }
        }
    }
}