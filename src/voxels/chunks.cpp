#include "chunks.hpp"
#include "chunk.hpp"

Chunks::Chunks(int w, int h, int d) : w(w), h(h), d(d) {
    volume = w * h * d;
    chunks.reserve(volume);

    int index = 0;
    for (int y = 0; y < h; y++) {
        for (int z = 0; z < d; z++) {
            for (int x = 0; x < w; x++, index++) {
                chunks.push_back(std::make_unique<Chunk>(x, y, z));
            }
        }
    }
}

voxel *Chunks::get(int x, int y, int z) {
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
    int lx = x - cx * Chunk::WIDTH;
    int ly = y - cy * Chunk::HEIGHT;
    int lz = z - cz * Chunk::DEPTH;
    return &chunk->voxels[(ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx];
}

Chunk *Chunks::get_chunk(int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0 || x >= w || y >= h || z >= d) {
        return nullptr;
    }
    return chunks[(y * d + z) * w + x].get();
}

void Chunks::set(int x, int y, int z, int id) {
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

    float inf = std::numeric_limits<float>::infinity();

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
                stepped_index = 0;
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

void Chunks::write(unsigned char *dest) {
    usize index = 0;
    for (usize i = 0; i < volume; i++) {
        auto *chunk = chunks[i].get();
        for (usize j = 0; j < Chunk::VOLUME; j++, index++) {
            dest[index] = chunk->voxels[j].id;
        }
    }
}

void Chunks::read(unsigned char *source) {
    usize index = 0;
    for (usize i = 0; i < volume; i++) {
        auto *chunk = chunks[i].get();
        for (usize j = 0; j < Chunk::VOLUME; j++, index++) {
            chunk->voxels[j].id = source[index];
        }
        chunk->modified = true;
    }
}