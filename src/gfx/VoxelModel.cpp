#define OGT_VOX_IMPLEMENTATION
#include "ogt_vox.h"

#include "VoxelModel.hpp"
#include <fstream>
#include <iostream>

bool VoxelModel::load_from_vox(const std::string &filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return false;
    }

    const ogt_vox_scene *scene = ogt_vox_read_scene(buffer.data(), static_cast<uint32_t>(size));
    if (!scene || scene->num_models == 0) return false;

    const ogt_vox_model *model = scene->models[0];
    tight_aabb = VoxelAABB {};

    for (uint32_t z = 0; z < model->size_z; z++) {
        for (uint32_t y = 0; y < model->size_y; y++) {
            for (uint32_t x = 0; x < model->size_x; x++) {
                uint32_t idx = x + (y * model->size_x) + (z * model->size_x * model->size_y);
                if (model->voxel_data[idx] != 0) {
                    glm::vec3 vox_min(x - 0.5f, z - 0.5f, y - 0.5f);
                    glm::vec3 vox_max(x + 0.5f, z + 0.5f, y + 0.5f);

                    tight_aabb.fit(vox_min);
                    tight_aabb.fit(vox_max);
                }
            }
        }
    }

    if (!tight_aabb.is_valid()) {
        tight_aabb.min_bounds = glm::vec3(0.0f);
        tight_aabb.max_bounds = glm::vec3(model->size_x, model->size_y, model->size_z);
    }

    width = model->size_x;
    height = model->size_z;
    depth = model->size_y;

    for (int i = 0; i < 256; i++) {
        ogt_vox_rgba c = scene->palette.color[i];
        palette[i] = glm::vec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
    }

    voxels.clear();
    for (uint32_t z = 0; z < model->size_z; z++) {
        for (uint32_t y = 0; y < model->size_y; y++) {
            for (uint32_t x = 0; x < model->size_x; x++) {
                uint32_t idx = x + (y * model->size_x) + (z * model->size_x * model->size_y);
                uint8_t color_idx = model->voxel_data[idx];

                if (color_idx != 0) {
                    voxels.push_back({ static_cast<uint8_t>(x), static_cast<uint8_t>(z), static_cast<uint8_t>(y), color_idx });
                }
            }
        }
    }

    ogt_vox_destroy_scene(scene);

    build_mesh();
    return true;
}

void VoxelModel::build_mesh() {
    std::vector<float> vertex_buffer;
    vertex_buffer.reserve(voxels.size() * 6 * 6 * 9);

    std::vector<uint8_t> grid(width * height * depth, 0);
    auto get_grid = [&](int x, int y, int z) -> uint8_t {
        if (x < 0 || x >= (int) width || y < 0 || y >= (int) height || z < 0 || z >= (int) depth) {
            return 0;
        }
        return grid[(y * depth + z) * width + x];
    };

    for (const auto &v : voxels) {
        grid[(v.y * depth + v.z) * width + v.x] = v.color_index;
    }

    struct Face {
        int dx, dy, dz;
        float factor;
    };

    constexpr std::array<Face, 6> faces = {{
        { 0, 1, 0, 1.00f },
        { 0, -1, 0, 0.75f },
        { 1, 0, 0, 0.95f },
        { -1, 0, 0, 0.85f },
        { 0, 0, 1, 0.90f },
        { 0, 0, -1, 0.80f }
    }};

    constexpr std::array<std::array<glm::vec3, 6>, 6> face_quads = {{
        {{ {-0.5f, 0.5f, -0.5f}, { 0.5f, 0.5f, -0.5f}, { 0.5f, 0.5f,  0.5f}, {-0.5f, 0.5f, -0.5f}, { 0.5f, 0.5f,  0.5f}, {-0.5f, 0.5f,  0.5f} }},
        {{ {-0.5f,-0.5f,  0.5f}, { 0.5f,-0.5f,  0.5f}, { 0.5f,-0.5f, -0.5f}, {-0.5f,-0.5f,  0.5f}, { 0.5f,-0.5f, -0.5f}, {-0.5f,-0.5f, -0.5f} }},
        {{ { 0.5f,-0.5f,  0.5f}, { 0.5f,-0.5f, -0.5f}, { 0.5f, 0.5f, -0.5f}, { 0.5f,-0.5f,  0.5f}, { 0.5f, 0.5f, -0.5f}, { 0.5f, 0.5f,  0.5f} }},
        {{ {-0.5f,-0.5f, -0.5f}, {-0.5f,-0.5f,  0.5f}, {-0.5f, 0.5f,  0.5f}, {-0.5f,-0.5f, -0.5f}, {-0.5f, 0.5f,  0.5f}, {-0.5f, 0.5f, -0.5f} }},
        {{ {-0.5f,-0.5f,  0.5f}, { 0.5f,-0.5f,  0.5f}, { 0.5f, 0.5f,  0.5f}, {-0.5f,-0.5f,  0.5f}, { 0.5f, 0.5f,  0.5f}, {-0.5f, 0.5f,  0.5f} }},
        {{ { 0.5f,-0.5f, -0.5f}, {-0.5f,-0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, { 0.5f,-0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, { 0.5f, 0.5f, -0.5f} }}
    }};

    for (const auto &v : voxels) {
        glm::vec4 col = palette[v.color_index];

        for (std::size_t f = 0; f < 6; f++) {
            const auto &face = faces[f];
            if (get_grid(v.x + face.dx, v.y + face.dy, v.z + face.dz) != 0) {
                continue;
            }

            float r = col.r * face.factor;
            float g = col.g * face.factor;
            float b = col.b * face.factor;
            float a = 1.0f;

            for (std::size_t vi = 0; vi < 6; vi++) {
                glm::vec3 pos = glm::vec3(v.x, v.y, v.z) + face_quads[f][vi];

                vertex_buffer.insert(vertex_buffer.end(), {
                    pos.x, pos.y, pos.z, 0.0f, 0.0f, r, g, b, a 
                });
            }
        }
    }

    constexpr std::array<int, 4> MODEL_ATTRS = { 3, 2, 4, 0 };
    mesh = std::make_unique<Mesh>(vertex_buffer, MODEL_ATTRS);
}

void VoxelModel::draw() const {
    if (mesh) {
        mesh->draw(GL_TRIANGLES);
    }
}