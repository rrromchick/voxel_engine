#include "VoxelRenderer.hpp"
#include "Mesh.hpp"
#include "Chunk.hpp"
#include "Block.hpp"
#include "Lightmap.hpp"
#include "Global.hpp"

#include <array>
#include <cmath>

namespace gfx {
    constexpr std::size_t VERTEX_SIZE = 3 + 2 + 4;
    constexpr std::array<int, 4> CHUNK_ATTRS = { 3, 2, 4, 0 };
    constexpr int ATLAS_TILES_PER_ROW = 16;
    constexpr float UV_SIZE = 1.0f / static_cast<float>(ATLAS_TILES_PER_ROW);
    constexpr float UV_EPSILON = 0.0005f;
    constexpr float MAX_LIGHT_LEVEL = 15.0f;

    struct FaceDirection {
        int dx, dy, dz;
        float factor;
        int texture_face_idx;
    };

    constexpr std::array<FaceDirection, 6> FACES = {{
        {  0,  1,  0, 1.00f, 3 },
        {  0, -1,  0, 0.75f, 2 },
        {  1,  0,  0, 0.95f, 1 },
        { -1,  0,  0, 0.85f, 0 },
        {  0,  0,  1, 0.90f, 5 },
        {  0,  0, -1, 0.80f, 4 }
    }};

    struct VertexDef {
        float x, y, z;
        float u_mod, v_mod;
        int dx1, dy1, dz1;
        int dx2, dy2, dz2;
    };

    constexpr std::array<std::array<VertexDef, 6>, 6> FACE_VERTICES = {{
        {{
            { -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1, 0, 0,  0, 0, -1 },
            {  0.5f, 0.5f, -0.5f, 1.0f, 0.0f,  1, 0, 0,  0, 0, -1 },
            {  0.5f, 0.5f,  0.5f, 1.0f, 1.0f,  1, 0, 0,  0, 0,  1 },
            { -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1, 0, 0,  0, 0, -1 },
            {  0.5f, 0.5f,  0.5f, 1.0f, 1.0f,  1, 0, 0,  0, 0,  1 },
            { -0.5f, 0.5f,  0.5f, 0.0f, 1.0f, -1, 0, 0,  0, 0,  1 }
        }},
        {{
            { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, -1, 0, 0,  0, 0,  1 },
            {  0.5f, -0.5f,  0.5f, 1.0f, 0.0f,  1, 0, 0,  0, 0,  1 },
            {  0.5f, -0.5f, -0.5f, 1.0f, 1.0f,  1, 0, 0,  0, 0, -1 },
            { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, -1, 0, 0,  0, 0,  1 },
            {  0.5f, -0.5f, -0.5f, 1.0f, 1.0f,  1, 0, 0,  0, 0, -1 },
            { -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -1, 0, 0,  0, 0, -1 }
        }},
        {{
            { 0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 0, -1, 0,  0, 0,  1 },
            { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0, -1, 0,  0, 0, -1 },
            { 0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0,  1, 0,  0, 0, -1 },
            { 0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 0, -1, 0,  0, 0,  1 },
            { 0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0,  1, 0,  0, 0, -1 },
            { 0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0,  1, 0,  0, 0,  1 }
        }},
        {{
            { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0, -1, 0,  0, 0, -1 },
            { -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0, -1, 0,  0, 0,  1 },
            { -0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0,  1, 0,  0, 0,  1 },
            { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0, -1, 0,  0, 0, -1 },
            { -0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0,  1, 0,  0, 0,  1 },
            { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0,  1, 0,  0, 0, -1 }
        }},
        {{
            { -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, -1, 0, 0,  0, -1, 0 },
            {  0.5f, -0.5f, 0.5f, 1.0f, 0.0f,  1, 0, 0,  0, -1, 0 },
            {  0.5f,  0.5f, 0.5f, 1.0f, 1.0f,  1, 0, 0,  0,  1, 0 },
            { -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, -1, 0, 0,  0, -1, 0 },
            {  0.5f,  0.5f, 0.5f, 1.0f, 1.0f,  1, 0, 0,  0,  1, 0 },
            { -0.5f,  0.5f, 0.5f, 0.0f, 1.0f, -1, 0, 0,  0,  1, 0 }
        }},
        {{
            {  0.5f, -0.5f, -0.5f, 0.0f, 0.0f,  1, 0, 0,  0, -1, 0 },
            { -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, -1, 0, 0,  0, -1, 0 },
            { -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, -1, 0, 0,  0,  1, 0 },
            {  0.5f, -0.5f, -0.5f, 0.0f, 0.0f,  1, 0, 0,  0, -1, 0 },
            { -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, -1, 0, 0,  0,  1, 0 },
            {  0.5f,  0.5f, -0.5f, 0.0f, 1.0f,  1, 0, 0,  0,  1, 0 }
        }}
    }};

    struct PlantVertex {
        float x, y, z, u, v;
    };

    constexpr std::array<PlantVertex, 24> PLANT_QUADS = {{
        { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f },
        {  0.5f, -0.5f,  0.5f, 1.0f, 0.0f },
        {  0.5f,  0.5f,  0.5f, 1.0f, 1.0f },
        { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f },
        {  0.5f,  0.5f,  0.5f, 1.0f, 1.0f },
        { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f },

        { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f },
        { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f },
        {  0.5f,  0.5f,  0.5f, 1.0f, 1.0f },
        { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f },
        {  0.5f,  0.5f,  0.5f, 1.0f, 1.0f },
        {  0.5f, -0.5f,  0.5f, 1.0f, 0.0f },

        { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f },
        {  0.5f, -0.5f, -0.5f, 1.0f, 0.0f },
        {  0.5f,  0.5f, -0.5f, 1.0f, 1.0f },
        { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f },
        {  0.5f,  0.5f, -0.5f, 1.0f, 1.0f },
        { -0.5f,  0.5f,  0.5f, 0.0f, 1.0f },

        { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f },
        { -0.5f,  0.5f,  0.5f, 0.0f, 1.0f },
        {  0.5f,  0.5f, -0.5f, 1.0f, 1.0f },
        { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f },
        {  0.5f,  0.5f, -0.5f, 1.0f, 1.0f },
        {  0.5f, -0.5f, -0.5f, 1.0f, 0.0f }
    }};

    struct UvBounds {
        float u_min, u_max, v_min, v_max;
    };

    constexpr UvBounds compute_uv_bounds(unsigned int tex_idx) {
        unsigned int col = tex_idx % ATLAS_TILES_PER_ROW;
        unsigned int row = tex_idx / ATLAS_TILES_PER_ROW;
        return {
            col * UV_SIZE + UV_EPSILON,
            (col + 1) * UV_SIZE - UV_EPSILON,
            1.0f - ((row + 1) * UV_SIZE) + UV_EPSILON,
            1.0f - (row * UV_SIZE) - UV_EPSILON
        };
    }

    constexpr int floor_div(int x, int a) {
        int res = x / a;
        int rem = x % a;
        if (rem != 0 && ((x ^ a) < 0)) {
            --res;
        }
        return res;
    }

    constexpr int wrap_local(int x, int size) {
        int rem = x % size;
        return rem < 0 ? rem + size : rem;
    }

    constexpr int voxel_offset(int lx, int ly, int lz) {
        return (ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx;
    }
}

using namespace gfx;

VoxelRenderer::VoxelRenderer(std::size_t capacity) : capacity(capacity) {
    buffer.reserve(capacity * VERTEX_SIZE * 6);
}

std::unique_ptr<Mesh> VoxelRenderer::render(
    Chunk *chunk, const std::vector<Chunk *> &chunks) {
    buffer.clear();
    if (!chunk) {
        return std::make_unique<Mesh>(buffer, CHUNK_ATTRS);
    }

    auto resolve_neighbor = [&](int x, int y, int z, Chunk *&out_chunk, int &lx, int &ly, int &lz) -> bool {
        if (x >= 0 && x < Chunk::WIDTH &&
            y >= 0 && y < Chunk::HEIGHT &&
            z >= 0 && z < Chunk::DEPTH) {
            out_chunk = chunk;
            lx = x;
            ly = y;
            lz = z;
            return true;
        }

        int cx = floor_div(x, Chunk::WIDTH) + 1;
        int cy = floor_div(y, Chunk::HEIGHT) + 1;
        int cz = floor_div(z, Chunk::DEPTH) + 1;
        if (cx < 0 || cx >= 3 || cy < 0 || cy >= 3 || cz < 0 || cz >= 3) {
            out_chunk = nullptr;
            return false;
        }

        out_chunk = chunks[(cy * 3 + cz) * 3 + cx];
        if (!out_chunk) return false;

        lx = wrap_local(x, Chunk::WIDTH);
        ly = wrap_local(y, Chunk::HEIGHT);
        lz = wrap_local(z, Chunk::DEPTH);
        return true;
    };

    auto is_blocked = [&](int x, int y, int z, unsigned char group) -> bool {
        Chunk *target_chunk = nullptr;
        int lx = 0, ly = 0, lz = 0;
        if (!resolve_neighbor(x, y, z, target_chunk, lx, ly, lz)) {
            return true;
        }

        const uint8_t block_id = target_chunk->voxels[voxel_offset(lx, ly, lz)].id;
        if (block_id == 0) return false;

        const auto *block = global.blocks[block_id].get();
        return block && (block->draw_group == group);
    };

    auto get_light_raw = [&](int x, int y, int z, int channel) -> float {
        Chunk *target_chunk = nullptr;
        int lx = 0, ly = 0, lz = 0;
        if (!resolve_neighbor(x, y, z, target_chunk, lx, ly, lz) || !target_chunk->lightmap) {
            return 0.0f;
        }
        return static_cast<float>(target_chunk->lightmap->get(lx, ly, lz, channel));
    };

    auto compute_vertex_light = [&](
        int x, int y, int z, int channel,
        int dx1, int dy1, int dz1,
        int dx2, int dy2, int dz2,
        float face_factor) -> float {
        float sum =
            get_light_raw(x, y, z, channel) +
            get_light_raw(x + dx1, y + dy1, z + dz1, channel) +
            get_light_raw(x + dx2, y + dy2, z + dz2, channel) +
            get_light_raw(x + dx1 + dx2, y + dy1 + dy2, z + dz1 + dz2, channel);

        return (sum / (4.0f * MAX_LIGHT_LEVEL)) * face_factor;
    };

    auto emit_vertex = [&](
        float vx, float vy, float vz,
        float u, float v,
        float lr, float lg, float lb, float ls) {
        buffer.push_back(vx);
        buffer.push_back(vy);
        buffer.push_back(vz);

        buffer.push_back(u);
        buffer.push_back(v);
        buffer.push_back(lr);
        buffer.push_back(lg);
        buffer.push_back(lb);
        buffer.push_back(ls);
    };

    for (int y = 0; y < Chunk::HEIGHT; ++y) {
        for (int z = 0; z < Chunk::DEPTH; ++z) {
            for (int x = 0; x < Chunk::WIDTH; ++x) {
                const uint8_t id = chunk->voxels[voxel_offset(x, y, z)].id;
                if (id == 0) continue;

                const auto *block = global.blocks[id].get();
                if (!block) continue;

                if (block->model == BlockModel::PLANT) {
                    const auto uv = compute_uv_bounds(block->texture_faces[0]);
                    const float lr = get_light_raw(x, y, z, 0) / MAX_LIGHT_LEVEL;
                    const float lg = get_light_raw(x, y, z, 1) / MAX_LIGHT_LEVEL;
                    const float lb = get_light_raw(x, y, z, 2) / MAX_LIGHT_LEVEL;
                    const float ls = get_light_raw(x, y, z, 3) / MAX_LIGHT_LEVEL;

                    for (const auto &pv : PLANT_QUADS) {
                        emit_vertex(
                            static_cast<float>(x) + pv.x,
                            static_cast<float>(y) + pv.y,
                            static_cast<float>(z) + pv.z,
                            uv.u_min + pv.u * (uv.u_max - uv.u_min),
                            uv.v_min + pv.v * (uv.v_max - uv.v_min),
                            lr, lg, lb, ls
                        );
                    }
                    continue;
                }

                const unsigned char group = block->draw_group;

                for (std::size_t f = 0; f < FACES.size(); ++f) {
                    const auto &face = FACES[f];
                    const int target_x = x + face.dx;
                    const int target_y = y + face.dy;
                    const int target_z = z + face.dz;

                    if (is_blocked(target_x, target_y, target_z, group)) {
                        continue;
                    }

                    const UvBounds uv = compute_uv_bounds(block->texture_faces[face.texture_face_idx]);

                    for (std::size_t v_idx = 0; v_idx < 6; ++v_idx) {
                        const auto &v_data = FACE_VERTICES[f][v_idx];

                        const float lr = compute_vertex_light(
                            target_x, target_y, target_z, 0,
                            v_data.dx1, v_data.dy1, v_data.dz1,
                            v_data.dx2, v_data.dy2, v_data.dz2,
                            face.factor);

                        const float lg = compute_vertex_light(
                            target_x, target_y, target_z, 1,
                            v_data.dx1, v_data.dy1, v_data.dz1,
                            v_data.dx2, v_data.dy2, v_data.dz2,
                            face.factor);

                        const float lb = compute_vertex_light(
                            target_x, target_y, target_z, 2,
                            v_data.dx1, v_data.dy1, v_data.dz1,
                            v_data.dx2, v_data.dy2, v_data.dz2,
                            face.factor);

                        const float ls = compute_vertex_light(
                            target_x, target_y, target_z, 3,
                            v_data.dx1, v_data.dy1, v_data.dz1,
                            v_data.dx2, v_data.dy2, v_data.dz2,
                            face.factor);

                        emit_vertex(
                            static_cast<float>(x) + v_data.x,
                            static_cast<float>(y) + v_data.y,
                            static_cast<float>(z) + v_data.z,
                            uv.u_min + v_data.u_mod * (uv.u_max - uv.u_min),
                            uv.v_min + v_data.v_mod * (uv.v_max - uv.v_min),
                            lr, lg, lb, ls
                        );
                    }
                }
            }
        }
    }

    return std::make_unique<Mesh>(buffer, CHUNK_ATTRS);
}
