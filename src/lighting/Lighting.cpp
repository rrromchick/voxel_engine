#include "Lighting.hpp"
#include "LightSolver.hpp"
#include "Lightmap.hpp"
#include "Level.hpp"
#include "Chunk.hpp"
#include "Block.hpp"
#include "Global.hpp"

Lighting::Lighting() {
    solver_r = std::make_unique<LightSolver>(0);
    solver_g = std::make_unique<LightSolver>(1);
    solver_b = std::make_unique<LightSolver>(2);
    solver_s = std::make_unique<LightSolver>(3);
}

Lighting::~Lighting() = default;

void Lighting::clear() {
    auto *level = global.ecs->level.get();
    if (!level) return;

    for (unsigned int y = 0; y < level->h; y++) {
        for (unsigned int z = 0; z < level->d; z++) {
            for (unsigned int x = 0; x < level->w; x++) {
                auto *chunk = level->get_chunk(x, y, z);
                if (!chunk || !chunk->lightmap) continue;
                for (int i = 0; i < Chunk::VOLUME; i++) {
                    chunk->lightmap->map[i] = 0;
                }
            }
        }
    }
}

void Lighting::on_chunk_loaded(int cx, int cy, int cz) {
    if (!global.ecs || !global.ecs->level) return;

    auto *chunk = global.ecs->level->get_chunk(cx, cy, cz);
    auto *chunk_upper = global.ecs->level->get_chunk(cx, cy + 1, cz);
    auto *chunk_lower = global.ecs->level->get_chunk(cx, cy - 1, cz);
    if (!chunk) return;

    if (chunk_lower && chunk_lower->lightmap) {
        for (int z = 0; z < Chunk::DEPTH; z++) {
            for (int x = 0; x < Chunk::WIDTH; x++) {
                int gx = x + cx * Chunk::WIDTH;
                int gz = z + cz * Chunk::DEPTH;

                int light = chunk->lightmap ? chunk->lightmap->get_s(x, 0, z) : 0;
                int ncy = cy - 1;
                if (light < 15) {
                    auto *current = chunk_lower;
                    if (current->lightmap && current->lightmap->get_s(x, 15, z) == 0) {
                        continue;
                    }
                    for (int y = 15;; y--) {
                        if (y < 0) {
                            ncy--;
                            y += Chunk::HEIGHT;
                        }
                        if (!current || ncy != current->y) {
                            current = global.ecs->level->get_chunk(cx, ncy, cz);
                        }
                        if (!current || !current->lightmap) {
                            break;
                        }

                        auto *vox = &(current->voxels[(y * Chunk::DEPTH + z) * Chunk::WIDTH + x]);
                        auto *block = global.blocks[vox->id].get();
                        if (!block || !block->light_passing) {
                            break;
                        }

                        current->modified = true;
                        solver_s->remove(gx, y + ncy * Chunk::HEIGHT, gz);
                        current->lightmap->set_s(x, y, z, 0);
                    }
                }
            }
        }
    }

    if (chunk_upper && chunk_upper->lightmap) {
        for (int z = 0; z < Chunk::DEPTH; z++) {
            for (int x = 0; x < Chunk::WIDTH; x++) {
                int gx = x + cx * Chunk::WIDTH;
                int gy = cy * Chunk::HEIGHT;
                int gz = z + cz * Chunk::DEPTH;
                int ncy = cy;

                int light = chunk_upper->lightmap->get_s(x, 0, z);

                auto *current = chunk;
                if (light == 15) {
                    for (int y = Chunk::HEIGHT - 1;; y--) {
                        if (y < 0) {
                            ncy--;
                            y += Chunk::HEIGHT;
                        }
                        if (!current || ncy != current->y) {
                            current = global.ecs->level->get_chunk(cx, ncy, cz);
                        }
                        if (!current || !current->lightmap) {
                            break;
                        }

                        auto *vox = &(current->voxels[(y * Chunk::DEPTH + z) * Chunk::WIDTH + x]);
                        auto *block = global.blocks[vox->id].get();
                        if (!block || !block->light_passing) {
                            break;
                        }
                        current->lightmap->set_s(x, y, z, 15);
                        current->modified = true;
                        solver_s->add(gx, y + ncy * Chunk::HEIGHT, gz);
                    }
                } else if (light) {
                    solver_s->add(gx, gy + Chunk::HEIGHT, gz);
                }
            }
        }
    } else {
        for (int z = 0; z < Chunk::DEPTH; z++) {
            for (int x = 0; x < Chunk::WIDTH; x++) {
                int gx = x + cx * Chunk::WIDTH;
                int gz = z + cz * Chunk::DEPTH;
                int ncy = cy;

                auto *current = chunk;
                for (int y = Chunk::HEIGHT - 1;; y--) {
                    if (y < 0) {
                        ncy--;
                        y += Chunk::HEIGHT;
                    }
                    if (!current || ncy != current->y) {
                        current = global.ecs->level->get_chunk(cx, ncy, cz);
                    }
                    if (!current || !current->lightmap) {
                        break;
                    }
                    
                    auto *vox = &(current->voxels[(y * Chunk::DEPTH + z) * Chunk::WIDTH + x]);
                    auto *block = global.blocks[vox->id].get();
                    if (!block || !block->light_passing) {
                        break;
                    }
                    current->lightmap->set_s(x, y, z, 15);
                    current->modified = true;
                    solver_s->add(gx, y + ncy * Chunk::HEIGHT, gz);
                }
            }
        }
    }

    for (unsigned int y = 0; y < Chunk::HEIGHT; y++) {
        for (unsigned int z = 0; z < Chunk::DEPTH; z++) {
            for (unsigned int x = 0; x < Chunk::WIDTH; x++) {
                auto vox = chunk->voxels[(y * Chunk::DEPTH + z) * Chunk::WIDTH + x];
                auto *block = global.blocks[vox.id].get();
                if (block && (block->emission[0] || block->emission[1] || block->emission[2])) {
                    int gx = x + cx * Chunk::WIDTH;
                    int gy = y + cy * Chunk::HEIGHT;
                    int gz = z + cz * Chunk::DEPTH;
                    solver_r->add(gx, gy, gz, block->emission[0]);
                    solver_g->add(gx, gy, gz, block->emission[1]);
                    solver_b->add(gx, gy, gz, block->emission[2]);
                }
            }
        }
    }

    for (int y = -1; y <= Chunk::HEIGHT; y++) {
        for (int z = -1; z <= Chunk::DEPTH; z++) {
            for (int x = -1; x <= Chunk::WIDTH; x++) {
                if (!(x == -1 || x == Chunk::WIDTH || y == -1 || y == Chunk::HEIGHT
                    || z == -1 || z == Chunk::DEPTH)) {
                    continue;
                }

                int gx = x + cx * Chunk::WIDTH;
                int gy = y + cy * Chunk::HEIGHT;
                int gz = z + cz * Chunk::DEPTH;

                solver_r->add(gx, gy, gz);
                solver_g->add(gx, gy, gz);
                solver_b->add(gx, gy, gz);
                solver_s->add(gx, gy, gz);
            }
        }
    }

    solver_r->solve();
    solver_g->solve();
    solver_b->solve();
    solver_s->solve();

    constexpr std::array<std::array<int, 3>, 6> directions = {{
        { -1, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 },
        { 0, 0, -1 }, { 0, 0, 1 }
    }};

    for (const auto &dir : directions) {
        auto *other = global.ecs->level->get_chunk(cx + dir[0], cy + dir[1], cz + dir[2]);
        if (other) other->modified = true;
    }
}

void Lighting::on_block_set(int x, int y, int z, int id) {
    if (!global.ecs || !global.ecs->level) return;
    auto *level = global.ecs->level.get();

    if (id == 0) {
        solver_r->remove(x, y, z);
        solver_g->remove(x, y, z);
        solver_b->remove(x, y, z);

        solver_r->solve();
        solver_g->solve();
        solver_b->solve();

        if (level->get_light(x, y + 1, z, 3) == 0xF) {
            for (int i = y; i >= 0; i--) {
                auto *vox = level->get_voxel(x, i, z);
                if (!vox || vox->id != 0) {
                    break;
                }
                solver_s->add(x, i, z, 0xF);
            }
        }

        constexpr std::array<std::array<int, 3>, 6> directions = {{
            { -1, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 },
            { 0, 0, -1 }, { 0, 0, 1 }
        }};

        for (const auto &dir : directions) {
            solver_r->add(x + dir[0], y + dir[1], z + dir[2]);
            solver_g->add(x + dir[0], y + dir[1], z + dir[2]);
            solver_b->add(x + dir[0], y + dir[1], z + dir[2]);
            solver_s->add(x + dir[0], y + dir[1], z + dir[2]);
        }

        solver_r->solve();
        solver_g->solve();
        solver_b->solve();
        solver_s->solve();
    } else { // Solid block placed
        solver_r->remove(x, y, z);
        solver_g->remove(x, y, z);
        solver_b->remove(x, y, z);
        solver_s->remove(x, y, z);

        for (int i = y - 1; i >= 0; i--) {
            solver_s->remove(x, i, z);
            auto *below_vox = level->get_voxel(x, i - 1, z);
            if (i == 0 || !below_vox || below_vox->id != 0) {
                break;
            }
        }

        solver_r->solve();
        solver_g->solve();
        solver_b->solve();
        solver_s->solve();

        if (id > 0 && static_cast<std::size_t>(id) < global.blocks.size() && global.blocks[id]) {
            auto *block = global.blocks[id].get();
            if (block->emission[0] || block->emission[1] || block->emission[2]) {
                solver_r->add(x, y, z, block->emission[0]);
                solver_g->add(x, y, z, block->emission[1]);
                solver_b->add(x, y, z, block->emission[2]);
                
                solver_r->solve();
                solver_g->solve();
                solver_b->solve();
            }
        }
    }
}