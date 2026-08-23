#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <limits>
#include <cmath>
#include <optional>
#include <glm/vec3.hpp>

#include "Chunk.hpp"
#include "Mesh.hpp"

struct VoxelRenderer;
struct WorldFiles;

struct RaycastHit {
    glm::ivec3 voxel_pos;
    glm::ivec3 normal;
    uint8_t block_id;
};

struct Level {
    int w, h, d;
    int ox, oy, oz;
    std::size_t volume;
    
    std::vector<std::unique_ptr<Chunk>> chunks;
    std::vector<std::unique_ptr<Mesh>> meshes;

    explicit Level(int w, int h, int d, int ox, int oy, int oz);
    ~Level() = default;

    Level(const Level &other) = delete;
    Level(Level &&other) = default;
    Level &operator=(const Level &other) = delete;
    Level &operator=(Level &&other) = default;

    void set_center(int cx, int cy, int cz);
    void translate(int dx, int dy, int dz);
    Chunk *get_chunk(int cx, int cy, int cz) const;
    Chunk *get_chunk_by_voxel(int world_x, int world_y, int world_z) const;

    voxel *get_voxel(int world_x, int world_y, int world_z) const;
    uint8_t get_voxel_id(int world_x, int world_y, int world_z) const;
    uint8_t get_light(int world_x, int world_y, int world_z, int channel) const;

    bool set_voxel(int world_x, int world_y, int world_z, uint8_t block_id);
    void set(int world_x, int world_y, int world_z, uint8_t block_id);

    bool load_visible(WorldFiles *world_files);
    bool decorate_visible();
    bool build_meshes(VoxelRenderer *renderer);

    bool is_solid(int world_x, int world_y, int world_z) const;
    bool is_obstacle(int world_x, int world_y, int world_z) const;
    bool intersects_aabb(const glm::vec3 &min, const glm::vec3 &max) const;

    // voxel *ray_cast(glm::vec3 origin, glm::vec3 dir, float max_dist,
    //     glm::vec3 &end, glm::vec3 &norm, glm::vec3 &iend) const;
    
    std::optional<RaycastHit> raycast(const glm::vec3 &origin, const glm::vec3 &dir, float max_distance) const;

    void update(const glm::vec3 &player_pos, WorldFiles *world_files, VoxelRenderer *renderer);

private:
    std::vector<std::unique_ptr<Chunk>> chunks_second;
    std::vector<std::unique_ptr<Mesh>> meshes_second;
};