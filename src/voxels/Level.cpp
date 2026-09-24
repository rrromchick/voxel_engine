#include "Level.hpp"
#include "Global.hpp"
#include "WorldGenerator.hpp"
#include "WorldFiles.hpp"
#include "Lighting.hpp"
#include "Lightmap.hpp"
#include "VoxelRenderer.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

Chunk *Level::get_chunk(int cx, int cy, int cz) const {
    auto it = chunks.find(ChunkPos{cx, cy, cz});
    if (it != chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

Chunk *Level::get_chunk_by_voxel(int world_x, int world_y, int world_z) const {
    auto cx = static_cast<int>(std::floor(static_cast<float>(world_x) / Chunk::WIDTH));
    auto cy = static_cast<int>(std::floor(static_cast<float>(world_y) / Chunk::HEIGHT));
    auto cz = static_cast<int>(std::floor(static_cast<float>(world_z) / Chunk::DEPTH));

    return get_chunk(cx, cy, cz);
}

voxel *Level::get_voxel(int world_x, int world_y, int world_z) const {
    auto *chunk = get_chunk_by_voxel(world_x, world_y, world_z);
    if (!chunk) return nullptr;

    int lx = ((world_x % Chunk::WIDTH) + Chunk::WIDTH) % Chunk::WIDTH;
    int ly = ((world_y % Chunk::HEIGHT) + Chunk::HEIGHT) % Chunk::HEIGHT;
    int lz = ((world_z % Chunk::DEPTH) + Chunk::DEPTH) % Chunk::DEPTH;

    int index = (ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx;
    return &chunk->voxels[index];
}

uint8_t Level::get_light(int world_x, int world_y, int world_z, int channel) const {
    auto *chunk = get_chunk_by_voxel(world_x, world_y, world_z);
    if (!chunk || !chunk->lightmap) return 0;

    int lx = (world_x % Chunk::WIDTH + Chunk::WIDTH) % Chunk::WIDTH;
    int ly = (world_y % Chunk::HEIGHT + Chunk::HEIGHT) % Chunk::HEIGHT;
    int lz = (world_z % Chunk::DEPTH + Chunk::DEPTH) % Chunk::DEPTH;

    return chunk->lightmap->get(lx, ly, lz, channel);
}

void Level::set(int world_x, int world_y, int world_z, uint8_t block_id) {
    set_voxel(world_x, world_y, world_z, block_id);
}

void Level::ensure_loaded_around(int cx, int cy, int cz, int radius, WorldFiles *world_files) {
    for (int y = cy - radius; y <= cy + radius; ++y) {
        for (int z = cz - radius; z <= cz + radius; ++z) {
            for (int x = cx - radius; x <= cx + radius; ++x) {
                ChunkPos pos{x, y, z};
                if (chunks.find(pos) == chunks.end()) {

                    auto chunk = std::make_unique<Chunk>(x, y, z);
                    std::span<uint8_t> voxel_span{ reinterpret_cast<uint8_t*>(chunk->voxels.get()), Chunk::VOLUME };

                    if (world_files && world_files->get_chunk(x, y, z, voxel_span)) {
                        chunk->decorated = true;
                    } else {
                        global.generator->generate_terrain(chunk.get());
                        global.generator->carve_caves(chunk.get());
                        chunk->decorated = false;
                    }

                    if (global.lighting) {
                        global.lighting->on_chunk_loaded(x, y, z);
                    }

                    chunks[pos] = std::move(chunk);
                }
            }
        }
    }
}

// Avoid calling write() inside frame ticks every time a chunk unloads!
// Instead, schedule saving at fixed intervals or upon game exit.
void Level::unload_distant_chunks(const std::vector<glm::vec3> &player_positions, int max_chunk_distance) {
    for (auto it = chunks.begin(); it != chunks.end(); ) {
        bool keep = false;
        for (const auto &pos : player_positions) {
            int pcx = static_cast<int>(std::floor(pos.x / Chunk::WIDTH));
            int pcy = static_cast<int>(std::floor(pos.y / Chunk::HEIGHT));
            int pcz = static_cast<int>(std::floor(pos.z / Chunk::DEPTH));

            if (std::abs(it->first.x - pcx) <= max_chunk_distance &&
                std::abs(it->first.y - pcy) <= max_chunk_distance &&
                std::abs(it->first.z - pcz) <= max_chunk_distance) {
                keep = true;
                break;
            }
        }

        if (!keep) {
            if (it->second->modified && global.world_files) {
                std::span<const uint8_t> voxel_span{ 
                    reinterpret_cast<const uint8_t*>(it->second->voxels.get()), 
                    Chunk::VOLUME 
                };
                global.world_files->put(voxel_span, it->first.x, it->first.y, it->first.z);
            }
            meshes.erase(it->first);
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }
}

uint8_t Level::get_voxel_id(int world_x, int world_y, int world_z) const {
    auto *v = get_voxel(world_x, world_y, world_z);
    return v ? v->id : 0;
}

bool Level::set_voxel(int world_x, int world_y, int world_z, uint8_t block_id) {
    auto cx = static_cast<int>(std::floor(static_cast<float>(world_x) / Chunk::WIDTH));
    auto cy = static_cast<int>(std::floor(static_cast<float>(world_y) / Chunk::HEIGHT));
    auto cz = static_cast<int>(std::floor(static_cast<float>(world_z) / Chunk::DEPTH));

    auto *chunk = get_chunk(cx, cy, cz);
    if (!chunk) return false;

    int lx = (world_x % Chunk::WIDTH + Chunk::WIDTH) % Chunk::WIDTH;
    int ly = (world_y % Chunk::HEIGHT + Chunk::HEIGHT) % Chunk::HEIGHT;
    int lz = (world_z % Chunk::DEPTH + Chunk::DEPTH) % Chunk::DEPTH;

    int index = (ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx;
    chunk->voxels[index].id = block_id;
    chunk->modified = true;

    // Restore boundary modification for neighbor mesh rebuilds
    Chunk *neighbor = nullptr;
    if (lx == 0 && (neighbor = get_chunk(cx - 1, cy, cz))) neighbor->modified = true;
    if (ly == 0 && (neighbor = get_chunk(cx, cy - 1, cz))) neighbor->modified = true;
    if (lz == 0 && (neighbor = get_chunk(cx, cy, cz - 1))) neighbor->modified = true;
    if (lx == Chunk::WIDTH - 1 && (neighbor = get_chunk(cx + 1, cy, cz))) neighbor->modified = true;
    if (ly == Chunk::HEIGHT - 1 && (neighbor = get_chunk(cx, cy + 1, cz))) neighbor->modified = true;
    if (lz == Chunk::DEPTH - 1 && (neighbor = get_chunk(cx, cy, cz + 1))) neighbor->modified = true;

    return true;
}

bool Level::decorate_visible(const std::vector<glm::vec3> &player_positions) {
    ChunkPos target_pos{};
    float min_distance = std::numeric_limits<float>::max();
    bool found = false;

    // 1. Find undecorated chunk that has ALL 27 neighbor chunks available
    for (const auto &[pos, chunk] : chunks) {
        if (!chunk || chunk->decorated) continue;

        // Ensure all 27 neighbors exist before considering this candidate
        bool has_all_neighbors = true;
        for (int oy = -1; oy <= 1 && has_all_neighbors; oy++) {
            for (int oz = -1; oz <= 1 && has_all_neighbors; oz++) {
                for (int ox = -1; ox <= 1 && has_all_neighbors; ox++) {
                    if (!get_chunk(pos.x + ox, pos.y + oy, pos.z + oz)) {
                        has_all_neighbors = false;
                    }
                }
            }
        }

        if (!has_all_neighbors) continue;

        for (const auto &p : player_positions) {
            float dx = (pos.x * Chunk::WIDTH) - p.x;
            float dy = (pos.y * Chunk::HEIGHT) - p.y;
            float dz = (pos.z * Chunk::DEPTH) - p.z;
            float dist = dx*dx + dy*dy + dz*dz;

            if (dist < min_distance) {
                min_distance = dist;
                target_pos = pos;
                found = true;
            }
        }
    }

    if (!found) return false;

    auto *chunk = get_chunk(target_pos.x, target_pos.y, target_pos.z);
    std::vector<Chunk*> closes(27, nullptr);

    for (int oy_off = -1; oy_off <= 1; oy_off++) {
        for (int oz_off = -1; oz_off <= 1; oz_off++) {
            for (int ox_off = -1; ox_off <= 1; ox_off++) {
                closes[((oy_off + 1) * 3 + (oz_off + 1)) * 3 + (ox_off + 1)] = 
                    get_chunk(target_pos.x + ox_off, target_pos.y + oy_off, target_pos.z + oz_off);
            }
        }
    }

    // 2. Decorate terrain
    global.generator->decorate(chunk, closes);
    chunk->decorated = true;

    // 3. Flag affected neighbors as modified & update light maps if needed
    for (auto *neighbor : closes) {
        if (neighbor) {
            neighbor->modified = true;
            if (global.lighting) {
                global.lighting->on_chunk_loaded(neighbor->x, neighbor->y, neighbor->z);
            }
        }
    }

    return true;
}

bool Level::build_meshes(VoxelRenderer *renderer, const std::vector<glm::vec3> &player_positions) {
    ChunkPos target_pos{};
    float min_distance = std::numeric_limits<float>::max();
    bool found = false;

    for (const auto &[pos, chunk] : chunks) {
        if (!chunk) continue;

        auto mesh_it = meshes.find(pos);
        if (mesh_it != meshes.end() && !chunk->modified) continue;

        for (auto &p : player_positions) {
            float dx = (pos.x * Chunk::WIDTH) - p.x;
            float dy = (pos.y * Chunk::HEIGHT) - p.y;
            float dz = (pos.z * Chunk::DEPTH) - p.z;
            float dist = dx*dx + dy*dy + dz*dz;

            if (dist < min_distance) {
                min_distance = dist;
                target_pos = pos;
                found = true;
            }
        }
    }

    if (!found) return false;

    auto *chunk = get_chunk(target_pos.x, target_pos.y, target_pos.z);
    if (!chunk) return false;

    if (chunk->is_empty()) {
        meshes.erase(target_pos);
        chunk->modified = false;
        return false;
    }

    chunk->modified = false;
    std::vector<Chunk*> closes(27, nullptr);

    for (int oy_off = -1; oy_off <= 1; oy_off++) {
        for (int oz_off = -1; oz_off <= 1; oz_off++) {
            for (int ox_off = -1; ox_off <= 1; ox_off++) {
                closes[((oy_off + 1) * 3 + (oz_off + 1)) * 3 + (ox_off + 1)] = 
                    get_chunk(target_pos.x + ox_off, target_pos.y + oy_off, target_pos.z + oz_off);
            }
        }
    }

    meshes[target_pos] = renderer->render(chunk, closes);
    return true;
}

bool Level::is_obstacle(int world_x, int world_y, int world_z) const {
    auto *chunk = get_chunk_by_voxel(world_x, world_y, world_z);
    
    // Treat unloaded chunks as solid obstacles to stop entities from clipping/falling through void
    if (!chunk) {
        return true; 
    }

    auto *vox = get_voxel(world_x, world_y, world_z);
    if (!vox) return false;

    return vox->id != BlockId::AIR && vox->id != BlockId::WATER && vox->id != BlockId::LAVA;
}

bool Level::is_solid(int world_x, int world_y, int world_z) const {
    auto *chunk = get_chunk_by_voxel(world_x, world_y, world_z);
    if (!chunk) return false;

    return get_voxel_id(world_x, world_y, world_z) != BlockId::AIR;
}

std::optional<RaycastHit> Level::raycast(
    const glm::vec3 &origin, const glm::vec3 &dir, float max_distance) const {
    glm::vec3 normalized_dir = glm::normalize(dir);

    auto x = static_cast<int>(std::floor(origin.x));
    auto y = static_cast<int>(std::floor(origin.y));
    auto z = static_cast<int>(std::floor(origin.z));

    int step_x = (normalized_dir.x > 0) ? 1 : -1;
    int step_y = (normalized_dir.y > 0) ? 1 : -1;
    int step_z = (normalized_dir.z > 0) ? 1 : -1;

    float t_max_x = (step_x > 0) ? (std::floor(origin.x) + 1.0f - origin.x) 
        / normalized_dir.x : (origin.x - std::floor(origin.x)) / -normalized_dir.x;
    float t_max_y = (step_y > 0) ? (std::floor(origin.y) + 1.0f - origin.y)
        / normalized_dir.y : (origin.y - std::floor(origin.y)) / -normalized_dir.y;
    float t_max_z = (step_z > 0) ? (std::floor(origin.z) + 1.0f - origin.z)
        / normalized_dir.z : (origin.z - std::floor(origin.z)) / -normalized_dir.z;

    float t_delta_x = std::abs(1.0f / normalized_dir.x);
    float t_delta_y = std::abs(1.0f / normalized_dir.y);
    float t_delta_z = std::abs(1.0f / normalized_dir.z);

    glm::ivec3 normal(0);
    float distance = 0.0f;

    while (distance <= max_distance) {
        uint8_t id = get_voxel_id(x, y, z);
        if (id != 0) {
            return RaycastHit { glm::ivec3(x, y, z), normal, id };
        }

        if (t_max_x < t_max_y) {
            if (t_max_x < t_max_z) {
                x += step_x;
                distance = t_max_x;
                t_max_x += t_delta_x;
                normal = glm::ivec3(-step_x, 0, 0);
            } else {
                z += step_z;
                distance = t_max_z;
                t_max_z += t_delta_z;
                normal = glm::ivec3(0, 0, -step_z);
            }
        } else {
            if (t_max_y < t_max_z) {
                y += step_y;
                distance = t_max_y;
                t_max_y += t_delta_y;
                normal = glm::ivec3(0, -step_y, 0);
            } else {
                z += step_z;
                distance = t_max_z;
                t_max_z += t_delta_z;
                normal = glm::ivec3(0, 0, -step_z);
            }
        }
    }

    return std::nullopt;
}

bool Level::intersects_aabb(const glm::vec3 &min, const glm::vec3 &max) const {
    auto min_x = static_cast<int>(std::floor(min.x));
    auto max_x = static_cast<int>(std::floor(max.x));
    auto min_y = static_cast<int>(std::floor(min.y));
    auto max_y = static_cast<int>(std::floor(max.y));
    auto min_z = static_cast<int>(std::floor(min.z));
    auto max_z = static_cast<int>(std::floor(max.z));

    for (int y = min_y; y <= max_y; y++) {
        for (int z = min_z; z <= max_z; z++) {
            for (int x = min_x; x <= max_x; x++) {
                if (is_obstacle(x, y, z)) {
                    return true;
                }
            }
        }
    }
    return false;
}

// #include "Level.hpp"
// #include "Global.hpp"
// #include "WorldGenerator.hpp"
// #include "WorldFiles.hpp"
// #include "Lighting.hpp"
// #include "Lightmap.hpp"
// #include "VoxelRenderer.hpp"
// #include <algorithm>
// #include <cmath>
// #include <limits>

// Chunk *Level::get_chunk(int cx, int cy, int cz) const {
//     auto it = chunks.find(ChunkPos{cx, cy, cz});
//     if (it != chunks.end()) {
//         return it->second.get();
//     }
//     return nullptr;
// }

// Chunk *Level::get_chunk_by_voxel(int world_x, int world_y, int world_z) const {
//     auto cx = static_cast<int>(std::floor(static_cast<float>(world_x) / Chunk::WIDTH));
//     auto cy = static_cast<int>(std::floor(static_cast<float>(world_y) / Chunk::HEIGHT));
//     auto cz = static_cast<int>(std::floor(static_cast<float>(world_z) / Chunk::DEPTH));

//     return get_chunk(cx, cy, cz);
// }

// voxel *Level::get_voxel(int world_x, int world_y, int world_z) const {
//     auto *chunk = get_chunk_by_voxel(world_x, world_y, world_z);
//     if (!chunk) return nullptr;

//     int lx = ((world_x % Chunk::WIDTH) + Chunk::WIDTH) % Chunk::WIDTH;
//     int ly = ((world_y % Chunk::HEIGHT) + Chunk::HEIGHT) % Chunk::HEIGHT;
//     int lz = ((world_z % Chunk::DEPTH) + Chunk::DEPTH) % Chunk::DEPTH;

//     int index = (ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx;
//     return &chunk->voxels[index];
// }

// uint8_t Level::get_light(int world_x, int world_y, int world_z, int channel) const {
//     auto *chunk = get_chunk_by_voxel(world_x, world_y, world_z);
//     if (!chunk || !chunk->lightmap) return 0;

//     int lx = (world_x % Chunk::WIDTH + Chunk::WIDTH) % Chunk::WIDTH;
//     int ly = (world_y % Chunk::HEIGHT + Chunk::HEIGHT) % Chunk::HEIGHT;
//     int lz = (world_z % Chunk::DEPTH + Chunk::DEPTH) % Chunk::DEPTH;

//     return chunk->lightmap->get(lx, ly, lz, channel);
// }

// void Level::set(int world_x, int world_y, int world_z, uint8_t block_id) {
//     set_voxel(world_x, world_y, world_z, block_id);
// }

// // Inside Level_4.cpp
// void Level::ensure_loaded_around(int cx, int cy, int cz, int radius, WorldFiles *world_files) {
//     // Limit load rate: process max 2-4 chunks per frame to prevent disk I/O saturation
//     int loaded_this_frame = 0;
//     const int max_loads_per_frame = 2;

//     for (int y = cy - radius; y <= cy + radius; ++y) {
//         for (int z = cz - radius; z <= cz + radius; ++z) {
//             for (int x = cx - radius; x <= cx + radius; ++x) {
//                 ChunkPos pos{x, y, z};
//                 if (chunks.find(pos) == chunks.end()) {
//                     if (loaded_this_frame >= max_loads_per_frame) return;

//                     auto chunk = std::make_unique<Chunk>(x, y, z);
//                     std::span<uint8_t> voxel_span{ reinterpret_cast<uint8_t*>(chunk->voxels.get()), Chunk::VOLUME };

//                     if (world_files && world_files->get_chunk(x, y, z, voxel_span)) {
//                         chunk->decorated = true;
//                     } else {
//                         global.generator->generate_terrain(chunk.get());
//                         global.generator->carve_caves(chunk.get());
//                         chunk->decorated = false;
//                     }

//                     if (global.lighting) {
//                         global.lighting->on_chunk_loaded(x, y, z);
//                     }

//                     chunks[pos] = std::move(chunk);
//                     loaded_this_frame++;
//                 }
//             }
//         }
//     }
// }

// // Avoid calling write() inside frame ticks every time a chunk unloads!
// // Instead, schedule saving at fixed intervals or upon game exit.
// void Level::unload_distant_chunks(const std::vector<glm::vec3> &player_positions, int max_chunk_distance) {
//     for (auto it = chunks.begin(); it != chunks.end(); ) {
//         bool keep = false;
//         for (const auto &pos : player_positions) {
//             int pcx = static_cast<int>(std::floor(pos.x / Chunk::WIDTH));
//             int pcy = static_cast<int>(std::floor(pos.y / Chunk::HEIGHT));
//             int pcz = static_cast<int>(std::floor(pos.z / Chunk::DEPTH));

//             if (std::abs(it->first.x - pcx) <= max_chunk_distance &&
//                 std::abs(it->first.y - pcy) <= max_chunk_distance &&
//                 std::abs(it->first.z - pcz) <= max_chunk_distance) {
//                 keep = true;
//                 break;
//             }
//         }

//         if (!keep) {
//             if (it->second->modified && global.world_files) {
//                 std::span<const uint8_t> voxel_span{ 
//                     reinterpret_cast<const uint8_t*>(it->second->voxels.get()), 
//                     Chunk::VOLUME 
//                 };
//                 global.world_files->put(voxel_span, it->first.x, it->first.y, it->first.z);
//             }
//             meshes.erase(it->first);
//             it = chunks.erase(it);
//         } else {
//             ++it;
//         }
//     }
// }

// uint8_t Level::get_voxel_id(int world_x, int world_y, int world_z) const {
//     auto *v = get_voxel(world_x, world_y, world_z);
//     return v ? v->id : 0;
// }

// bool Level::set_voxel(int world_x, int world_y, int world_z, uint8_t block_id) {
//     auto cx = static_cast<int>(std::floor(static_cast<float>(world_x) / Chunk::WIDTH));
//     auto cy = static_cast<int>(std::floor(static_cast<float>(world_y) / Chunk::HEIGHT));
//     auto cz = static_cast<int>(std::floor(static_cast<float>(world_z) / Chunk::DEPTH));

//     auto *chunk = get_chunk(cx, cy, cz);
//     if (!chunk) return false;

//     int lx = (world_x % Chunk::WIDTH + Chunk::WIDTH) % Chunk::WIDTH;
//     int ly = (world_y % Chunk::HEIGHT + Chunk::HEIGHT) % Chunk::HEIGHT;
//     int lz = (world_z % Chunk::DEPTH + Chunk::DEPTH) % Chunk::DEPTH;

//     int index = (ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx;
//     chunk->voxels[index].id = block_id;
//     chunk->modified = true;

//     // Restore boundary modification for neighbor mesh rebuilds
//     Chunk *neighbor = nullptr;
//     if (lx == 0 && (neighbor = get_chunk(cx - 1, cy, cz))) neighbor->modified = true;
//     if (ly == 0 && (neighbor = get_chunk(cx, cy - 1, cz))) neighbor->modified = true;
//     if (lz == 0 && (neighbor = get_chunk(cx, cy, cz - 1))) neighbor->modified = true;
//     if (lx == Chunk::WIDTH - 1 && (neighbor = get_chunk(cx + 1, cy, cz))) neighbor->modified = true;
//     if (ly == Chunk::HEIGHT - 1 && (neighbor = get_chunk(cx, cy + 1, cz))) neighbor->modified = true;
//     if (lz == Chunk::DEPTH - 1 && (neighbor = get_chunk(cx, cy, cz + 1))) neighbor->modified = true;

//     return true;
// }

// bool Level::decorate_visible(const std::vector<glm::vec3> &player_positions) {
//     ChunkPos target_pos{};
//     float min_distance = std::numeric_limits<float>::max();
//     bool found = false;

//     // 1. Find undecorated chunk that has ALL 27 neighbor chunks available
//     for (const auto &[pos, chunk] : chunks) {
//         if (!chunk || chunk->decorated) continue;

//         // Ensure all 27 neighbors exist before considering this candidate
//         bool has_all_neighbors = true;
//         for (int oy = -1; oy <= 1 && has_all_neighbors; oy++) {
//             for (int oz = -1; oz <= 1 && has_all_neighbors; oz++) {
//                 for (int ox = -1; ox <= 1 && has_all_neighbors; ox++) {
//                     if (!get_chunk(pos.x + ox, pos.y + oy, pos.z + oz)) {
//                         has_all_neighbors = false;
//                     }
//                 }
//             }
//         }

//         if (!has_all_neighbors) continue;

//         for (const auto &p : player_positions) {
//             float dx = (pos.x * Chunk::WIDTH) - p.x;
//             float dy = (pos.y * Chunk::HEIGHT) - p.y;
//             float dz = (pos.z * Chunk::DEPTH) - p.z;
//             float dist = dx*dx + dy*dy + dz*dz;

//             if (dist < min_distance) {
//                 min_distance = dist;
//                 target_pos = pos;
//                 found = true;
//             }
//         }
//     }

//     if (!found) return false;

//     auto *chunk = get_chunk(target_pos.x, target_pos.y, target_pos.z);
//     std::vector<Chunk*> closes(27, nullptr);

//     for (int oy_off = -1; oy_off <= 1; oy_off++) {
//         for (int oz_off = -1; oz_off <= 1; oz_off++) {
//             for (int ox_off = -1; ox_off <= 1; ox_off++) {
//                 closes[((oy_off + 1) * 3 + (oz_off + 1)) * 3 + (ox_off + 1)] = 
//                     get_chunk(target_pos.x + ox_off, target_pos.y + oy_off, target_pos.z + oz_off);
//             }
//         }
//     }

//     // 2. Decorate terrain
//     global.generator->decorate(chunk, closes);
//     chunk->decorated = true;

//     // 3. Flag affected neighbors as modified & update light maps if needed
//     for (auto *neighbor : closes) {
//         if (neighbor) {
//             neighbor->modified = true;
//             if (global.lighting) {
//                 global.lighting->on_chunk_loaded(neighbor->x, neighbor->y, neighbor->z);
//             }
//         }
//     }

//     return true;
// }

// bool Level::build_meshes(VoxelRenderer *renderer, const std::vector<glm::vec3> &player_positions) {
//     ChunkPos target_pos{};
//     float min_distance = std::numeric_limits<float>::max();
//     bool found = false;

//     for (const auto &[pos, chunk] : chunks) {
//         if (!chunk) continue;

//         auto mesh_it = meshes.find(pos);
//         if (mesh_it != meshes.end() && !chunk->modified) continue;

//         for (auto &p : player_positions) {
//             float dx = (pos.x * Chunk::WIDTH) - p.x;
//             float dy = (pos.y * Chunk::HEIGHT) - p.y;
//             float dz = (pos.z * Chunk::DEPTH) - p.z;
//             float dist = dx*dx + dy*dy + dz*dz;

//             if (dist < min_distance) {
//                 min_distance = dist;
//                 target_pos = pos;
//                 found = true;
//             }
//         }
//     }

//     if (!found) return false;

//     auto *chunk = get_chunk(target_pos.x, target_pos.y, target_pos.z);
//     if (!chunk) return false;

//     if (chunk->is_empty()) {
//         meshes.erase(target_pos);
//         chunk->modified = false;
//         return false;
//     }

//     chunk->modified = false;
//     std::vector<Chunk*> closes(27, nullptr);

//     for (int oy_off = -1; oy_off <= 1; oy_off++) {
//         for (int oz_off = -1; oz_off <= 1; oz_off++) {
//             for (int ox_off = -1; ox_off <= 1; ox_off++) {
//                 closes[((oy_off + 1) * 3 + (oz_off + 1)) * 3 + (ox_off + 1)] = 
//                     get_chunk(target_pos.x + ox_off, target_pos.y + oy_off, target_pos.z + oz_off);
//             }
//         }
//     }

//     meshes[target_pos] = renderer->render(chunk, closes);
//     return true;
// }

// bool Level::is_obstacle(int world_x, int world_y, int world_z) const {
//     auto *chunk = get_chunk_by_voxel(world_x, world_y, world_z);
//     if (!chunk) return false;

//     auto *vox = get_voxel(world_x, world_y, world_z);
//     if (!vox) return false;

//     return vox->id != BlockId::AIR && vox->id != BlockId::WATER && vox->id != BlockId::LAVA;
// }

// bool Level::is_solid(int world_x, int world_y, int world_z) const {
//     auto *chunk = get_chunk_by_voxel(world_x, world_y, world_z);
//     if (!chunk) return false;

//     return get_voxel_id(world_x, world_y, world_z) != BlockId::AIR;
// }

// std::optional<RaycastHit> Level::raycast(
//     const glm::vec3 &origin, const glm::vec3 &dir, float max_distance) const {
//     glm::vec3 normalized_dir = glm::normalize(dir);

//     auto x = static_cast<int>(std::floor(origin.x));
//     auto y = static_cast<int>(std::floor(origin.y));
//     auto z = static_cast<int>(std::floor(origin.z));

//     int step_x = (normalized_dir.x > 0) ? 1 : -1;
//     int step_y = (normalized_dir.y > 0) ? 1 : -1;
//     int step_z = (normalized_dir.z > 0) ? 1 : -1;

//     float t_max_x = (step_x > 0) ? (std::floor(origin.x) + 1.0f - origin.x) 
//         / normalized_dir.x : (origin.x - std::floor(origin.x)) / -normalized_dir.x;
//     float t_max_y = (step_y > 0) ? (std::floor(origin.y) + 1.0f - origin.y)
//         / normalized_dir.y : (origin.y - std::floor(origin.y)) / -normalized_dir.y;
//     float t_max_z = (step_z > 0) ? (std::floor(origin.z) + 1.0f - origin.z)
//         / normalized_dir.z : (origin.z - std::floor(origin.z)) / -normalized_dir.z;

//     float t_delta_x = std::abs(1.0f / normalized_dir.x);
//     float t_delta_y = std::abs(1.0f / normalized_dir.y);
//     float t_delta_z = std::abs(1.0f / normalized_dir.z);

//     glm::ivec3 normal(0);
//     float distance = 0.0f;

//     while (distance <= max_distance) {
//         uint8_t id = get_voxel_id(x, y, z);
//         if (id != 0) {
//             return RaycastHit { glm::ivec3(x, y, z), normal, id };
//         }

//         if (t_max_x < t_max_y) {
//             if (t_max_x < t_max_z) {
//                 x += step_x;
//                 distance = t_max_x;
//                 t_max_x += t_delta_x;
//                 normal = glm::ivec3(-step_x, 0, 0);
//             } else {
//                 z += step_z;
//                 distance = t_max_z;
//                 t_max_z += t_delta_z;
//                 normal = glm::ivec3(0, 0, -step_z);
//             }
//         } else {
//             if (t_max_y < t_max_z) {
//                 y += step_y;
//                 distance = t_max_y;
//                 t_max_y += t_delta_y;
//                 normal = glm::ivec3(0, -step_y, 0);
//             } else {
//                 z += step_z;
//                 distance = t_max_z;
//                 t_max_z += t_delta_z;
//                 normal = glm::ivec3(0, 0, -step_z);
//             }
//         }
//     }

//     return std::nullopt;
// }

// bool Level::intersects_aabb(const glm::vec3 &min, const glm::vec3 &max) const {
//     auto min_x = static_cast<int>(std::floor(min.x));
//     auto max_x = static_cast<int>(std::floor(max.x));
//     auto min_y = static_cast<int>(std::floor(min.y));
//     auto max_y = static_cast<int>(std::floor(max.y));
//     auto min_z = static_cast<int>(std::floor(min.z));
//     auto max_z = static_cast<int>(std::floor(max.z));

//     for (int y = min_y; y <= max_y; y++) {
//         for (int z = min_z; z <= max_z; z++) {
//             for (int x = min_x; x <= max_x; x++) {
//                 if (is_obstacle(x, y, z)) {
//                     return true;
//                 }
//             }
//         }
//     }
//     return false;
// }