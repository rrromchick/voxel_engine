#include "voxel_renderer.hpp"
#include "mesh.hpp"
#include "voxels/chunk.hpp"

constexpr usize VERTEX_SIZE = 3 + 2 + 1;
constexpr std::array<int, 4> CHUNK_ATTRS = { 3, 2, 1, 0 };
constexpr float UV_SIZE = 1.0f / 16.0f;

VoxelRenderer::VoxelRenderer(usize capacity) : capacity(capacity) {
    buffer.reserve(capacity * VERTEX_SIZE * 6);
}

std::unique_ptr<Mesh> VoxelRenderer::render(
    Chunk *chunk, const std::vector<Chunk*> &chunks) {
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

    auto is_chunk = [&](int x, int y, int z) -> bool {
        return get_chunk(x, y, z) != nullptr;
    };

    auto get_voxel = [&](int x, int y, int z) -> voxel & {
        auto *chunk = get_chunk(x, y, z);
        if (!chunk) {
            static voxel dummy{};
            return dummy;
        }

        int lx = local(x, Chunk::WIDTH);
        int ly = local(y, Chunk::HEIGHT);
        int lz = local(z, Chunk::DEPTH);

        return chunk->voxels[(ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx];
    };

    auto is_blocked = [&](int x, int y, int z) -> bool {
        auto *chunk = get_chunk(x, y, z);
        if (!chunk) return true;
        
        int lx = local(x, Chunk::WIDTH);
        int ly = local(y, Chunk::HEIGHT);
        int lz = local(z, Chunk::DEPTH);

        return chunk->voxels[(ly * Chunk::DEPTH + lz) * Chunk::WIDTH + lx].id != 0;
    };

	auto push_vertex = [&](float x, float y, float z, float u, float v, float l) {
		buffer.insert(buffer.end(), { x, y, z, u, v, l });
	};

	struct FaceDirection {
		int dx, dy, dz;
		float lighting;
	};

	constexpr std::array<FaceDirection, 6> faces = {{
		{ 0, 1, 0, 1.00f },
		{ 0, -1, 0, 0.75f }, 
		{ 1, 0, 0, 0.95f }, 
		{ -1, 0, 0, 0.85f }, 
		{ 0, 0, 1, 0.90f }, 
		{ 0, 0, -1, 0.80f }  
	}};

	struct FaceVertex {
		float x, y, z;
		float u_mod, v_mod;
	};

	constexpr std::array<std::array<FaceVertex, 6>, 6> face_vertices = {{
		// Top (+Y)
		{{ { -0.5f, 0.5f, -0.5f, 1.0f, 0.0f }, { -0.5f, 0.5f, 0.5f, 1.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.0f, 1.0f },
		{ -0.5f, 0.5f, -0.5f, 1.0f, 0.0f }, { 0.5f, 0.5f, 0.5f, 0.0f, 1.0f }, { 0.5f, 0.5f, -0.5f, 0.0f, 0.0f } }},
		// Bottom (-Y)
		{{ { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f }, { 0.5f, -0.5f, 0.5f, 1.0f, 1.0f }, { -0.5f, -0.5f, 0.5f, 0.0f, 1.0f },
		{ -0.5f, -0.5f, -0.5f, 0.0f, 0.0f }, { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f }, { 0.5f, -0.5f, 0.5f, 1.0f, 1.0f } }},
		// Right (+X)
		{{ { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f }, { 0.5f, 0.5f, -0.5f, 1.0f, 1.0f }, { 0.5f, 0.5f, 0.5f, 0.0f, 1.0f },
		{ 0.5f, -0.5f, -0.5f, 1.0f, 0.0f }, { 0.5f, 0.5f, 0.5f, 0.0f, 1.0f }, { 0.5f, -0.5f, 0.5f, 0.0f, 0.0f } }},
		// Left (-X)
		{{ { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f }, { -0.5f, 0.5f, 0.5f, 1.0f, 1.0f }, { -0.5f, 0.5f, -0.5f, 0.0f, 1.0f },
		{ -0.5f, -0.5f, -0.5f, 0.0f, 0.0f }, { -0.5f, -0.5f, 0.5f, 1.0f, 0.0f }, { -0.5f, 0.5f, 0.5f, 1.0f, 1.0f } }},
		// Front (+Z)
		{{ { -0.5f, -0.5f, 0.5f, 0.0f, 0.0f }, { 0.5f, 0.5f, 0.5f, 1.0f, 1.0f }, { -0.5f, 0.5f, 0.5f, 0.0f, 1.0f },
		{ -0.5f, -0.5f, 0.5f, 0.0f, 0.0f }, { 0.5f, -0.5f, 0.5f, 1.0f, 0.0f }, { 0.5f, 0.5f, 0.5f, 1.0f, 1.0f } }},
		// Back (-Z)
		{{ { -0.5f, -0.5f, -0.5f, 1.0f, 0.0f }, { -0.5f, 0.5f, -0.5f, 1.0f, 1.0f }, { 0.5f, 0.5f, -0.5f, 0.0f, 1.0f },
		{ -0.5f, -0.5f, -0.5f, 1.0f, 0.0f }, { 0.5f, 0.5f, -0.5f, 0.0f, 1.0f }, { 0.5f, -0.5f, -0.5f, 0.0f, 0.0f } }}
	}};

	for (int y = 0; y < Chunk::HEIGHT; y++) {
		for (int z = 0; z < Chunk::DEPTH; z++) {
			for (int x = 0; x < Chunk::WIDTH; x++) {
				const auto &vox = get_voxel(x, y, z);
				unsigned int id = vox.id;

				if (id == 0) continue;

				float u_base = (id % 16) * UV_SIZE;
				float v_base = 1.0f - ((1 + id / 16) * UV_SIZE);

				for (usize f = 0; f < 6; f++) {
					const auto &face = faces[f];

					if (!is_blocked(x + face.dx, y + face.dy, z + face.dz)) {
						for (usize v_idx = 0; v_idx < 6; v_idx++) {
							const auto &vertex_data = face_vertices[f][v_idx];

							float final_u = u_base + (vertex_data.u_mod * UV_SIZE);
							float final_v = v_base + (vertex_data.v_mod * UV_SIZE);
							
							push_vertex(
								x + vertex_data.x,
								y + vertex_data.y,
								z + vertex_data.z,
								final_u,
								final_v,
								face.lighting);
						}
					}
				}
			}
		}
	}

	return std::make_unique<Mesh>(buffer, CHUNK_ATTRS);
}