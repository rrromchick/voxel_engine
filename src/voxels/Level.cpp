#include "Level.hpp"
#include "Global.hpp"
#include "WorldGenerator.hpp"
#include "WorldFiles.hpp"
#include "Lighting.hpp"
#include "Lightmap.hpp"
#include "VoxelRenderer.hpp"
#include <algorithm>

Level::Level(int w, int h, int d, int ox, int oy, int oz)
    : w(w), h(h), d(d), ox(ox), oy(oy), oz(oz) {
    volume = static_cast<std::size_t>(w * h * d);
    chunks.resize(volume);
    chunks_second.resize(volume);
    meshes.resize(volume);
    meshes_second.resize(volume);
}

Chunk *Level::get_chunk_by_voxel(int world_x, int world_y, int world_z) const {
    auto cx = static_cast<int>(std::floor(static_cast<float>(world_x) / Chunk::WIDTH));
    auto cy = static_cast<int>(std::floor(static_cast<float>(world_y) / Chunk::HEIGHT));
    auto cz = static_cast<int>(std::floor(static_cast<float>(world_z) / Chunk::DEPTH));

    return get_chunk(cx, cy, cz);
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

void Level::translate(int dx, int dy, int dz) {
    for (std::size_t i = 0; i < volume; i++) {
        chunks_second[i].reset();
        meshes_second[i].reset();
    }

    for (int y = 0; y < h; y++) {
        for (int z = 0; z < d; z++) {
            for (int x = 0; x < w; x++) {
                std::size_t old_idx = (y * d + z) * w + x;
                int nx = x - dx;
                int ny = y - dy;
                int nz = z - dz;

                if (nx < 0 || ny < 0 || nz < 0 || nx >= w || ny >= h || nz >= d) {
                    chunks[old_idx].reset();
                    meshes[old_idx].reset();
                    continue;
                }

                std::size_t new_idx = (ny * d + nz) * w + nx;
                meshes_second[new_idx] = std::move(meshes[old_idx]);
                chunks_second[new_idx] = std::move(chunks[old_idx]);
            }
        }
    }

    std::swap(chunks, chunks_second);
    std::swap(meshes, meshes_second);

    ox += dx;
    oy += dy;
    oz += dz;
}

void Level::set_center(int cx, int cy, int cz) {
    int dx = (cx - w / 2) - ox;
    int dy = (cy - h / 2) - oy;
    int dz = (cz - d / 2) - oz;

    if (dx != 0 || dy != 0 || dz != 0) {
        translate(dx, dy, dz);
    }
}

Chunk *Level::get_chunk(int cx, int cy, int cz) const {
    int lx = cx - ox;
    int ly = cy - oy;
    int lz = cz - oz;

    if (lx < 0 || lx >= w || ly < 0 || ly >= h || lz < 0 || lz >= d) {
        return nullptr;
    }

    int index = (ly * d + lz) * w + lx;
    return chunks[index].get();
}

voxel *Level::get_voxel(int world_x, int world_y, int world_z) const {
    auto cx = static_cast<int>(std::floor(static_cast<float>(world_x) / Chunk::WIDTH));
    auto cy = static_cast<int>(std::floor(static_cast<float>(world_y) / Chunk::HEIGHT));
    auto cz = static_cast<int>(std::floor(static_cast<float>(world_z) / Chunk::DEPTH));

    auto *chunk = get_chunk(cx, cy, cz);
    if (!chunk) return nullptr;

    int lx = (world_x % Chunk::WIDTH + Chunk::WIDTH) % Chunk::WIDTH;
    int ly = (world_y % Chunk::HEIGHT + Chunk::HEIGHT) % Chunk::HEIGHT;
    int lz = (world_z % Chunk::DEPTH + Chunk::DEPTH) % Chunk::DEPTH;

    int index = (ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx;
    return &chunk->voxels[index];
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

    Chunk *neighbor = nullptr;
    if (lx == 0 && (neighbor = get_chunk(cx - 1, cy, cz))) {
        neighbor->modified = true;
    }
    if (ly == 0 && (neighbor = get_chunk(cx, cy - 1, cz))) {
        neighbor->modified = true;
    }
    if (lz == 0 && (neighbor = get_chunk(cx, cy, cz - 1))) {
        neighbor->modified = true;
    }
    if (lx == Chunk::WIDTH - 1 && (neighbor = get_chunk(cx + 1, cy, cz))) {
        neighbor->modified = true;
    }
    if (ly == Chunk::HEIGHT - 1 && (neighbor = get_chunk(cx, cy + 1, cz))) {
        neighbor->modified = true;
    }
    if (lz == Chunk::DEPTH - 1 && (neighbor = get_chunk(cx, cy, cz + 1))) {
        neighbor->modified = true;
    }

    return true;
}

bool Level::load_visible(WorldFiles *world_files) {
    int near_x = 0, near_y = 0, near_z = 0;
    int min_distance = std::numeric_limits<int>::max();
    bool found = false;

    for (int y = 0; y < h; y++) {
        for (int z = 0; z < d; z++) {
            for (int x = 0; x < w; x++) {
                int index = (y * d + z) * w + x;
                if (chunks[index] != nullptr) continue;

                int lx = x - w / 2;
                int ly = y - h / 2;
                int lz = z - d / 2;
                int distance = lx * lx + ly * ly + lz * lz;

                if (distance < min_distance) {
                    min_distance = distance;
                    near_x = x;
                    near_y = y;
                    near_z = z;
                    found = true;
                }
            }
        }
    }

    if (!found) return false;

    int index = (near_y * d + near_z) * w + near_x;
    if (chunks[index] != nullptr) return false;

    chunks[index] = std::make_unique<Chunk>(near_x + ox, near_y + oy, near_z + oz);
    auto *chunk = chunks[index].get();
    std::span<uint8_t> voxel_span { reinterpret_cast<uint8_t*>(chunk->voxels.get()), Chunk::VOLUME };

    if (!world_files->get_chunk(chunk->x, chunk->y, chunk->z, voxel_span)) {
        global.generator->generate_terrain(chunk);
        global.generator->carve_caves(chunk);
        chunk->decorated = false;
    } else {
        chunk->decorated = true;
    }

    global.lighting->on_chunk_loaded(ox + near_x, oy + near_y, oz + near_z);
    return true;
}

bool Level::decorate_visible() {
    int near_x = 0, near_y = 0, near_z = 0;
    int min_distance = std::numeric_limits<int>::max();
    bool found = false;

    for (int y = 1; y < h - 1; y++) {
        for (int z = 1; z < d - 1; z++) {
            for (int x = 1; x < w - 1; x++) {
                int index = (y * d + z) * w + x;
                auto *chunk = chunks[index].get();

                if (!chunk || chunk->decorated) continue;

                int lx = x - w / 2;
                int ly = y - h / 2;
                int lz = z - d / 2;
                int distance = lx * lx + ly * ly + lz * lz;

                if (distance < min_distance) {
                    min_distance = distance;
                    near_x = x;
                    near_y = y;
                    near_z = z;
                    found = true;
                }
            }
        }
    }

    if (!found) return false;

    int index = (near_y * d + near_z) * w + near_x;
    auto *chunk = chunks[index].get();

    std::vector<Chunk*> closes(27, nullptr);
    for (int oy = -1; oy <= 1; oy++) {
        for (int oz = -1; oz <= 1; oz++) {
            for (int ox_off = -1; ox_off <= 1; ox_off++) {
                int nx = near_x + ox_off;
                int ny = near_y + oy;
                int nz = near_z + oz;

                if (nx < 0 || nx >= w || ny < 0 || ny >= h || nz < 0 || nz >= d) {
                    continue; 
                }

                auto *neighbor = chunks[(ny * d + nz) * w + nx].get();
                if (!neighbor) return false;

                closes[((oy + 1) * 3 + (oz + 1)) * 3 + (ox_off + 1)] = neighbor;
            }
        }
    }

    global.generator->decorate(chunk, closes);
    chunk->decorated = true;

    for (auto *neighbor : closes) {
        if (neighbor) neighbor->modified = true;
    }

    return true;
}

bool Level::build_meshes(VoxelRenderer *renderer) {
    int near_x = 0, near_y = 0, near_z = 0;
    auto min_distance = std::numeric_limits<int>::max();
    bool found = false;

    for (int y = 0; y < h; y++) {
        for (int z = 0; z < d; z++) {
            for (int x = 0; x < w; x++) {
                int index = (y * d + z) * w + x;
                auto *chunk = chunks[index].get();
                if (chunk == nullptr) continue;

                auto *mesh = meshes[index].get();
                if (mesh != nullptr && !chunk->modified) continue;

                int lx = x - w / 2;
                int ly = y - h / 2;
                int lz = z - d / 2;
                int distance = (lx * lx + ly * ly + lz * lz);
                if (distance < min_distance) {
                    min_distance = distance;
                    near_x = x;
                    near_y = y;
                    near_z = z;
                    found = true;
                }
            }
        }
    }

    if (!found) return false;

    int index = (near_y * d + near_z) * w + near_x;
    auto *chunk = chunks[index].get();
    if (chunk == nullptr) return false;

    auto *mesh = meshes[index].get();
    if (mesh == nullptr || chunk->modified) {
        if (chunk->is_empty()) {
            meshes[index].reset();
            return false;
        }

        chunk->modified = false;
        std::vector<Chunk*> closes(27, nullptr);

        for (int oy_off = -1; oy_off <= 1; oy_off++) {
            for (int oz_off = -1; oz_off <= 1; oz_off++) {
                for (int ox_off = -1; ox_off <= 1; ox_off++) {
                    int nx = near_x + ox_off;
                    int ny = near_y + oy_off;
                    int nz = near_z + oz_off;

                    if (nx < 0 || nx >= w || ny < 0 || ny >= h || nz < 0 || nz >= d) {
                        continue;
                    }

                    auto *neighbor = chunks[(ny * d + nz) * w + nx].get();
                    closes[((oy_off + 1) * 3 + (oz_off + 1)) * 3 + (ox_off + 1)] = neighbor;
                }
            }
        }

        meshes[index] = renderer->render(chunk, closes);
        return true;
    }
    return false;
}

bool Level::is_obstacle(int world_x, int world_y, int world_z) const {
    int max_world_y = h * Chunk::HEIGHT;

    if (world_y < 0) return true;
    if (world_y >= max_world_y) return false;

    auto *vox = get_voxel(world_x, world_y, world_z);
    if (!vox) return false;

    return vox->id != BlockId::AIR && vox->id != BlockId::WATER && vox->id != BlockId::LAVA;
}

bool Level::is_solid(int world_x, int world_y, int world_z) const {
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
                if (is_solid(x, y, z)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void Level::update(const glm::vec3 &player_pos, WorldFiles *world_files, VoxelRenderer *renderer) {
    auto p_chunk_x = static_cast<int>(std::floor(player_pos.x / Chunk::WIDTH));
    auto p_chunk_y = static_cast<int>(std::floor(player_pos.y / Chunk::HEIGHT));
    auto p_chunk_z = static_cast<int>(std::floor(player_pos.z / Chunk::DEPTH));

    set_center(p_chunk_x, p_chunk_y, p_chunk_z);
    load_visible(world_files);
    decorate_visible();
    build_meshes(renderer);
}