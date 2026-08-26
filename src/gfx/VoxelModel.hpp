#pragma once

#include <vector>
#include <string>
#include <memory>
#include <array>
#include <glm/glm.hpp>
#include <glm/common.hpp>

#include "Mesh.hpp"

struct VoxelElement {
    uint8_t x, y, z;
    uint8_t color_index;
};

struct VoxelAABB {
    glm::vec3 min_bounds { FLT_MAX };
    glm::vec3 max_bounds { -FLT_MAX };

    void fit(const glm::vec3 &point) {
        min_bounds = glm::min(min_bounds, point);
        max_bounds = glm::max(max_bounds, point); 
    }

    glm::vec3 get_size() const {
        return max_bounds - min_bounds;
    }

    glm::vec3 get_center() const {
        return (min_bounds + max_bounds) * 0.5f;
    }

    bool is_valid() const {
        return min_bounds.x <= max_bounds.x &&
            min_bounds.y <= max_bounds.y &&
            min_bounds.z <= max_bounds.z;
    }
};

struct VoxelModel {
    VoxelModel() = default;
    ~VoxelModel() = default;

    VoxelModel(const VoxelModel &other) = delete;
    VoxelModel(VoxelModel &&other) = default;
    VoxelModel &operator=(const VoxelModel &other) = delete;
    VoxelModel &operator=(VoxelModel &&other) = default;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 0;

    VoxelAABB tight_aabb;

    std::vector<VoxelElement> voxels;
    std::array<glm::vec4, 256> palette {};
    std::unique_ptr<Mesh> mesh;

    bool load_from_vox(const std::string &filepath);
    void build_mesh();
    void draw() const;
};