#include "Chunks.hpp"
#include "Chunk.hpp"
#include "Lightmap.hpp"
#include "WorldGenerator.hpp"
#include "WorldFiles.hpp"
#include "Lighting.hpp"
#include "VoxelRenderer.hpp"
#include "Mesh.hpp"
#include "Global.hpp"

Chunks::Chunks(int w, int h, int d, int ox, int oy, int oz) 
    : w(w), h(h), d(d), ox(ox), oy(oy), oz(oz) {
    volume = w * h * d;
    chunks.resize(volume);
    chunks_second.resize(volume);

    meshes.resize(volume);
    meshes_second.resize(volume);

    for (std::size_t i = 0; i < volume; i++) {
        chunks[i] = nullptr;
        meshes[i] = nullptr;
    }
}

bool Chunks::is_obstacle(int x, int y, int z) {
    auto *vox = get(x, y, z);
    if (vox == nullptr) {
        return true;
    }
    return global.blocks[vox->id].get()->obstacle;
}

bool Chunks::build_meshes(VoxelRenderer *renderer) {
    int near_x = 0;
    int near_y = 0;
    int near_z = 0;
    int min_distance = 1000000000;
    for (unsigned int y = 0; y < h; y++) {
        for (unsigned int z = 1; z < d - 1; z++) {
            for (unsigned int x = 1; x < w - 1; x++) {
                int index = (y * d + z) * w + x;
                auto *chunk = chunks[index].get();
                if (chunk == nullptr) {
                    continue;
                }

                auto *mesh = meshes[index].get();
                if (mesh != nullptr && !chunk->modified) {
                    continue;
                }

                int lx = x - w / 2;
                int ly = y - h / 2;
                int lz = z - d / 2;
                auto distance = (lx * lx + ly * ly + lz * lz);
                if (distance < min_distance) {
                    min_distance = distance;
                    near_x = x;
                    near_y = y;
                    near_z = z;
                }
            }
        }
    }

    int index = (near_y * d + near_z) * w + near_x;

    std::vector<Chunk*> closes(27);
    auto *chunk = chunks[index].get();
    if (chunk == nullptr) {
        return false;
    }

    auto *mesh = meshes[index].get();
    if (mesh == nullptr || chunk->modified) {
        if (chunk->is_empty()) {
            meshes[index].reset();
            return false;
        }

        chunk->modified = false;
        for (std::size_t i = 0; i < closes.size(); i++) {
            closes[i] = nullptr;
        }

        for (std::size_t j = 0; j < volume; j++) {
            auto *other = chunks[j].get();
            if (other == nullptr) {
                continue;
            }

            int ox = other->x - chunk->x;
            int oy = other->y - chunk->y;
            int oz = other->z - chunk->z;

            if (std::abs(ox) > 1 || std::abs(oy) > 1 || std::abs(oz) > 1) {
                continue;
            }

            ox += 1;
            oy += 1;
            oz += 1;
            closes[(oy * 3 + oz) * 3 + ox] = other;
        }

        meshes[index] = renderer->render(chunk, closes);
        return true;
    }
    return false;
}

voxel *Chunks::get(int x, int y, int z) {
    x -= ox * Chunk::WIDTH;
    y -= oy * Chunk::HEIGHT;
    z -= oz * Chunk::DEPTH;

    int cx = x / Chunk::WIDTH;
    int cy = y / Chunk::HEIGHT;
    int cz = z / Chunk::DEPTH;

    if (x < 0) cx--;
    if (y < 0) cy--;
    if (z < 0) cz--;

    if (cx < 0 || cy < 0 || cz < 0 || cx >= w || cy >= h || cz >= d) {
        return nullptr;
    }

    auto *chunk = chunks[(cy * d + cz) * w + cx].get();
    if (chunk == nullptr) return nullptr;
    int lx = x - cx * Chunk::WIDTH;
    int ly = y - cy * Chunk::HEIGHT;
    int lz = z - cz * Chunk::DEPTH;
    return &chunk->voxels[(ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx];
}

Chunk *Chunks::get_chunk(int x, int y, int z) {
    x -= ox;
    y -= oy;
    z -= oz;

    if (x < 0 || y < 0 || z < 0 || x >= w || y >= h || z >= d) {
        return nullptr;
    }
    return chunks[(y * d + z) * w + x].get();
}

Chunk *Chunks::get_chunk_by_voxel(int x, int y, int z) {
    x -= ox * Chunk::WIDTH;
    y -= oy * Chunk::HEIGHT;
    z -= oz * Chunk::DEPTH;

    int cx = x / Chunk::WIDTH;
    int cy = y / Chunk::HEIGHT;
    int cz = z / Chunk::DEPTH;

    if (x < 0) cx--;
    if (y < 0) cy--;
    if (z < 0) cz--;

    if (cx < 0 || cy < 0 || cz < 0 || cx >= w || cy >= h || cz >= d) {
        return nullptr;
    }
    return chunks[(cy * d + cz) * w + cx].get();
}

unsigned char Chunks::get_light(int x, int y, int z, int channel) {
    x -= ox * Chunk::WIDTH;
    y -= oy * Chunk::HEIGHT;
    z -= oz * Chunk::DEPTH;

    int cx = x / Chunk::WIDTH;
    int cy = y / Chunk::HEIGHT;
    int cz = z / Chunk::DEPTH;

    if (x < 0) cx--;
    if (y < 0) cy--;
    if (z < 0) cz--;

    if (cx < 0 || cy < 0 || cz < 0 || cx >= w || cy >= h || cz >= d) {
        return 0;
    }

    auto *chunk = chunks[(cy * d + cz) * w + cx].get();
    if (chunk == nullptr) return 0;
    int lx = x - cx * Chunk::WIDTH;
    int ly = y - cy * Chunk::HEIGHT;
    int lz = z - cz * Chunk::DEPTH; 
    return chunk->lightmap->get(lx, ly, lz, channel);
}

void Chunks::set(int x, int y, int z, int id) {
    x -= ox * Chunk::WIDTH;
    y -= oy * Chunk::HEIGHT;
    z -= oz * Chunk::DEPTH;

    int cx = x / Chunk::WIDTH;
    int cy = y / Chunk::HEIGHT;
    int cz = z / Chunk::DEPTH;

    if (x < 0) cx--;
    if (y < 0) cy--;
    if (z < 0) cz--;

    if (cx < 0 || cy < 0 || cz < 0 || cx >= w || cy >= h || cz >= d) {
        return;
    }

    auto *chunk = chunks[(cy * d + cz) * w + cx].get();
    int lx = x - cx * Chunk::WIDTH;
    int ly = y - cy * Chunk::HEIGHT;
    int lz = z - cz * Chunk::DEPTH;
    chunk->voxels[(ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx].id = id;
    chunk->modified = true;

    if (lx == 0 && (chunk = get_chunk(cx - 1, cy, cz))) chunk->modified = true;
    if (ly == 0 && (chunk = get_chunk(cx, cy - 1, cz))) chunk->modified = true;
    if (lz == 0 && (chunk = get_chunk(cx, cy, cz - 1))) chunk->modified = true;

    if (lx == Chunk::WIDTH - 1 && (chunk = get_chunk(cx + 1, cy, cz))) chunk->modified = true;
    if (ly == Chunk::HEIGHT - 1 && (chunk = get_chunk(cx, cy + 1, cz))) chunk->modified = true;
    if (lz == Chunk::DEPTH - 1 && (chunk = get_chunk(cx, cy, cz + 1))) chunk->modified = true;
}

voxel *Chunks::ray_cast(
    vec3 a, vec3 dir, float max_dist, vec3 &end, vec3 &norm, vec3 &iend) {
    float px = a.x;
    float py = a.y;
    float pz = a.z;

    float dx = dir.x;
    float dy = dir.y;
    float dz = dir.z;

    float t = 0.0f;
    int ix = glm::floor(px);
    int iy = glm::floor(py);
    int iz = glm::floor(pz);

    float stepx = (dx > 0.0f) ? 1.0f : -1.0f;
    float stepy = (dy > 0.0f) ? 1.0f : -1.0f;
    float stepz = (dz > 0.0f) ? 1.0f : -1.0f;

    constexpr auto inf = std::numeric_limits<float>::infinity();

    float tx_delta = (dx == 0.0f) ? inf : std::abs(1.0f / dx);
    float ty_delta = (dy == 0.0f) ? inf : std::abs(1.0f / dy);
    float tz_delta = (dz == 0.0f) ? inf : std::abs(1.0f / dz);

    float xdist = (stepx > 0) ? (ix + 1 - px) : (px - ix);
    float ydist = (stepy > 0) ? (iy + 1 - py) : (py - iy);
    float zdist = (stepz > 0) ? (iz + 1 - pz) : (pz - iz);

    auto tx_max = (tx_delta < inf) ? tx_delta * xdist : inf;
    auto ty_max = (ty_delta < inf) ? ty_delta * ydist : inf;
    auto tz_max = (tz_delta < inf) ? tz_delta * zdist : inf;

    int stepped_index = -1;

    while (t <= max_dist) {
        auto *voxel = get(ix, iy, iz);
        if (voxel == nullptr || voxel->id) {
            end.x = px + t * dx;
            end.y = py + t * dy;
            end.z = pz + t * dz;

            iend.x = ix;
            iend.y = iy;
            iend.z = iz;

            norm.x = norm.y = norm.z = 0.0f;
            if (stepped_index == 0) norm.x = -stepx;
            if (stepped_index == 1) norm.y = -stepy;
            if (stepped_index == 2) norm.z = -stepz;
            return voxel;
        }
        
        if (tx_max < ty_max) {
            if (tx_max < tz_max) {
                ix += stepx;
                t = tx_max;
                tx_max += tx_delta;
                stepped_index = 0;
            } else {
                iz += stepz;
                t = tz_max;
                tz_max += tz_delta;
                stepped_index = 2;
            }
        } else {
            if (ty_max < tz_max) {
                iy += stepy;
                t = ty_max;
                ty_max += ty_delta;
                stepped_index = 1;
            } else {
                iz += stepz;
                t = tz_max;
                tz_max += tz_delta;
                stepped_index = 2;
            }
        }
    }
    
    iend.x = ix;
    iend.y = iy;
    iend.z = iz;

    end.x = px + t * dx;
    end.y = py + t * dy;
    end.z = pz + t * dz;
    norm.x = norm.y = norm.z = 0.0f;
    return nullptr;
}

void Chunks::set_center(int x, int y, int z) {
    int cx = x / Chunk::WIDTH;
    int cy = y / Chunk::HEIGHT;
    int cz = z / Chunk::DEPTH;

    cx -= ox;
    cy -= oy;
    cz -= oz;

    if (x < 0) cx--;
    if (y < 0) cy--;
    if (z < 0) cz--;

    cx -= w / 2;
    cy -= h / 2;
    cz -= d / 2;
    if (cx != 0 || cy != 0 || cz != 0) {
        translate(cx, cy, cz);
    }
}

bool Chunks::load_visible(WorldFiles *world_files) {
    int near_x = 0;
    int near_y = 0;
    int near_z = 0;
    int min_distance = 1000000000;
    for (unsigned int y = 0; y < h; y++) {
        for (unsigned int z = 1; z < d - 1; z++) {
            for (unsigned int x = 1; x < w - 1; x++) {
                int index = (y * d + z) * w + x;
                auto *chunk = chunks[index].get();
                if (chunk != nullptr) {
                    continue;
                }

                int lx = x - w / 2;
                int ly = y - h / 2;
                int lz = z - d / 2;
                auto distance = (lx * lx + ly * ly + lz * lz);
                if (distance < min_distance) {
                    min_distance = distance;
                    near_x = x;
                    near_y = y;
                    near_z = z;
                }
            }
        }
    }

    int index = (near_y * d + near_z) * w + near_x;
    if (chunks[index].get() != nullptr) {
        return false;
    }

    chunks[index] = std::make_unique<Chunk>(near_x + ox, near_y + oy, near_z + oz);
    auto *chunk = chunks[index].get();
    std::span<uint8_t> voxel_span { 
        reinterpret_cast<uint8_t*>(chunk->voxels.get()), Chunk::VOLUME };
    if (!world_files->get_chunk(chunk->x, chunk->z, voxel_span)) {
        global.generator->generate(chunk->voxels, chunk->x, chunk->y, chunk->z);
    }

    global.lighting->on_chunk_loaded(ox + near_x, oy + near_y, oz + near_z);
    return true;
}

void Chunks::translate(int dx, int dy, int dz) {
    for (unsigned int i = 0; i < volume; i++) {
        chunks_second[i].reset();
        meshes_second[i].reset();
    }

    for (unsigned int y = 0; y < h; y++) {
        for (unsigned int z = 0; z < d; z++) {
            for (unsigned int x = 0; x < w; x++) {
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