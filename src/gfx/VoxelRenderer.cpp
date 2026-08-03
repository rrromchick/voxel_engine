#include "VoxelRenderer.hpp"
#include "Mesh.hpp"
#include "Chunk.hpp"
#include "Block.hpp"
#include "Lightmap.hpp"
#include "Global.hpp"

constexpr std::size_t VERTEX_SIZE = 3 + 2 + 4;
constexpr std::array<int, 4> CHUNK_ATTRS = { 3, 2, 4, 0 };
constexpr float UV_SIZE = 1.0f / 16.0f;

constexpr float UV_EPSILON = 0.0005f; 

VoxelRenderer::VoxelRenderer(std::size_t capacity) : capacity(capacity) {
    buffer.reserve(capacity * VERTEX_SIZE * 6);
}

std::unique_ptr<Mesh> VoxelRenderer::render(
    Chunk *chunk, const std::vector<Chunk *> &chunks) {    
    buffer.clear();

    auto cdiv = [](int x, int a) -> int {
        int res = x / a;
        int rem = x % a;
        if (rem != 0 && ((x ^ a) < 0)) {
            res--;
        }
        return res;
    };

    auto local = [](int x, int size) -> int {
        int rem = x % size;
        return rem < 0 ? rem + size : rem;
    };

    auto get_chunk = [&](int x, int y, int z) -> Chunk * {
        int cx = cdiv(x, Chunk::WIDTH) + 1;
        int cy = cdiv(y, Chunk::HEIGHT) + 1;
        int cz = cdiv(z, Chunk::DEPTH) + 1;

        if (cx < 0 || cx >= 3 || cy < 0 || cy >= 3 || cz < 0 || cz >= 3) {
            return nullptr;
        }

        return chunks[(cy * 3 + cz) * 3 + cx];
    };

    auto get_voxel = [&](int x, int y, int z) -> const voxel & {
        auto *target_chunk = get_chunk(x, y, z);
        if (!target_chunk) {
            static voxel dummy {};
            return dummy;
        }

        int lx = local(x, Chunk::WIDTH);
        int ly = local(y, Chunk::HEIGHT);
        int lz = local(z, Chunk::DEPTH);

        return target_chunk->voxels[(ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx];
    };

    auto is_blocked = [&](int x, int y, int z, unsigned char group) -> bool {
        auto *target_chunk = get_chunk(x, y, z);
        if (!target_chunk) return true;

        const auto &vox = get_voxel(x, y, z);
        if (vox.id == 0) return false;

        auto *block = global.blocks[vox.id].get();
        return block && (block->draw_group == group);
    };

    auto get_light = [&](int x, int y, int z, int channel) -> float {
        auto *target_chunk = get_chunk(x, y, z);
        if (!target_chunk || !target_chunk->lightmap) {
            return 0.0f;
        }
        int lx = local(x, Chunk::WIDTH);
        int ly = local(y, Chunk::HEIGHT);
        int lz = local(z, Chunk::DEPTH);
        return target_chunk->lightmap->get(lx, ly, lz, channel);
    };

    auto compute_vertex_light =
        [&](int x, int y, int z, int channel,
            int dx1, int dy1, int dz1,
            int dx2, int dy2, int dz2,
            float face_factor) -> float {
        float center_light = get_light(x, y, z, channel);
        float side1_light  = get_light(x + dx1, y + dy1, z + dz1, channel);
        float side2_light  = get_light(x + dx2, y + dy2, z + dz2, channel);
        float corner_light = get_light(x + dx1 + dx2, y + dy1 + dy2, z + dz1 + dz2, channel);

        float light_avg = (center_light + side1_light + side2_light + corner_light) / 4.0f / 15.0f;
        return light_avg * face_factor;
    };

    struct FaceDirection {
        int dx, dy, dz;
        float factor;
        int texture_face_idx;
    };

    constexpr std::array<FaceDirection, 6> faces = {{
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

    constexpr std::array<std::array<VertexDef, 6>, 6> face_vertices = {{
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

    for (int y = 0; y < Chunk::HEIGHT; y++) {
        for (int z = 0; z < Chunk::DEPTH; z++) {
            for (int x = 0; x < Chunk::WIDTH; x++) {
                const auto &vox = get_voxel(x, y, z);
                unsigned int id = vox.id;

                if (id == 0) continue;

                auto *block = global.blocks[id].get();
                if (!block) continue;

                if (block->model == BlockModel::PLANT) {
                    unsigned int tex_idx = block->texture_faces[0];
                    unsigned int col = tex_idx % 16;
                    unsigned int row = tex_idx / 16;

                    float u_min = col * UV_SIZE + UV_EPSILON;
                    float u_max = (col + 1) * UV_SIZE - UV_EPSILON;
                    float v_min = 1.0f - ((row + 1) * UV_SIZE) + UV_EPSILON;
                    float v_max = 1.0f - (row * UV_SIZE) - UV_EPSILON;

                    float lr = get_light(x, y, z, 0) / 15.0f;
                    float lg = get_light(x, y, z, 1) / 15.0f;
                    float lb = get_light(x, y, z, 2) / 15.0f;
                    float ls = get_light(x, y, z, 3) / 15.0f;

                    struct PlantVertex { float x, y, z, u, v; };

                    constexpr std::array<PlantVertex, 24> plant_quads = {{
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

                    for (const auto &pv : plant_quads) {
                        float final_u = u_min + pv.u * (u_max - u_min);
                        float final_v = v_min + pv.v * (v_max - v_min);

                        buffer.insert(buffer.end(), {
                            x + pv.x, y + pv.y, z + pv.z,
                            final_u, final_v, lr, lg, lb, ls
                        });
                    }

                    continue;
                }

                unsigned char group = block->draw_group;

                for (std::size_t f = 0; f < 6; f++) {
                    const auto &face = faces[f];

                    int target_x = x + face.dx;
                    int target_y = y + face.dy;
                    int target_z = z + face.dz;

                    if (!is_blocked(target_x, target_y, target_z, group)) {
                        unsigned int tex_idx = block->texture_faces[face.texture_face_idx];
                        
                        unsigned int col = tex_idx % 16;
                        unsigned int row = tex_idx / 16;

                        float u_min = col * UV_SIZE + UV_EPSILON;
                        float u_max = (col + 1) * UV_SIZE - UV_EPSILON;

                        float v_min = 1.0f - ((row + 1) * UV_SIZE) + UV_EPSILON;
                        float v_max = 1.0f - (row * UV_SIZE) - UV_EPSILON;

                        for (std::size_t v_idx = 0; v_idx < 6; v_idx++) {
                            const auto &v_data = face_vertices[f][v_idx];

                            float final_u = u_min + v_data.u_mod * (u_max - u_min);
                            float final_v = v_min + v_data.v_mod * (v_max - v_min);

                            float lr = compute_vertex_light(
                                target_x, target_y, target_z, 0,
                                v_data.dx1, v_data.dy1, v_data.dz1,
                                v_data.dx2, v_data.dy2, v_data.dz2,
                                face.factor);

                            float lg = compute_vertex_light(
                                target_x, target_y, target_z, 1,
                                v_data.dx1, v_data.dy1, v_data.dz1,
                                v_data.dx2, v_data.dy2, v_data.dz2,
                                face.factor);

                            float lb = compute_vertex_light(
                                target_x, target_y, target_z, 2,
                                v_data.dx1, v_data.dy1, v_data.dz1,
                                v_data.dx2, v_data.dy2, v_data.dz2,
                                face.factor);

                            float ls = compute_vertex_light(
                                target_x, target_y, target_z, 3,
                                v_data.dx1, v_data.dy1, v_data.dz1,
                                v_data.dx2, v_data.dy2, v_data.dz2,
                                face.factor);

                            buffer.insert(buffer.end(), {
                                x + v_data.x, y + v_data.y, z + v_data.z,
                                final_u, final_v, lr, lg, lb, ls });
                        }
                    }
                }
            }
        }
    }

    return std::make_unique<Mesh>(buffer, CHUNK_ATTRS);
}