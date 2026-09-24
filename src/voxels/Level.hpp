#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <limits>
#include <cmath>
#include <optional>
#include <unordered_map>
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

// Key structure for hashing chunk coordinates
struct ChunkPos {
    int x, y, z;

    bool operator==(const ChunkPos &other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// Custom hash for ChunkPos using prime mixing
struct ChunkPosHash {
    std::size_t operator()(const ChunkPos &pos) const {
        std::size_t h1 = std::hash<int>{}(pos.x);
        std::size_t h2 = std::hash<int>{}(pos.y);
        std::size_t h3 = std::hash<int>{}(pos.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

struct Level {
    // Spatial storage for active chunks and meshes
    std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash> chunks;
    std::unordered_map<ChunkPos, std::unique_ptr<Mesh>, ChunkPosHash> meshes;

    Level() = default;
    ~Level() = default;

    Level(const Level &other) = delete;
    Level(Level &&other) = default;
    Level &operator=(const Level &other) = delete;
    Level &operator=(Level &&other) = default;

    Chunk *get_chunk(int cx, int cy, int cz) const;
    Chunk *get_chunk_by_voxel(int world_x, int world_y, int world_z) const;

    voxel *get_voxel(int world_x, int world_y, int world_z) const;
    uint8_t get_voxel_id(int world_x, int world_y, int world_z) const;
    uint8_t get_light(int world_x, int world_y, int world_z, int channel) const;

    bool set_voxel(int world_x, int world_y, int world_z, uint8_t block_id);
    void set(int world_x, int world_y, int world_z, uint8_t block_id);

    bool load_visible(WorldFiles *world_files, const std::vector<glm::vec3> &player_positions);
    bool decorate_visible(const std::vector<glm::vec3> &player_positions);
    bool build_meshes(VoxelRenderer *renderer, const std::vector<glm::vec3> &player_positions);

    bool is_solid(int world_x, int world_y, int world_z) const;
    bool is_obstacle(int world_x, int world_y, int world_z) const;
    bool intersects_aabb(const glm::vec3 &min, const glm::vec3 &max) const;

    std::optional<RaycastHit> raycast(const glm::vec3 &origin, const glm::vec3 &dir, float max_distance) const;

    void ensure_loaded_around(int cx, int cy, int cz, int radius, WorldFiles *world_files);
    void unload_distant_chunks(const std::vector<glm::vec3> &player_positions, int max_chunk_distance);
};

// #pragma once

// #include <vector>
// #include <memory>
// #include <cstdint>
// #include <limits>
// #include <cmath>
// #include <optional>
// #include <glm/vec3.hpp>

// #include "Chunk.hpp"
// #include "Mesh.hpp"

// struct VoxelRenderer;
// struct WorldFiles;

// struct RaycastHit {
//     glm::ivec3 voxel_pos;
//     glm::ivec3 normal;
//     uint8_t block_id;
// };

// struct Level {
//     int w, h, d;
//     int ox, oy, oz;
//     int center_cx, center_cy, center_cz;
//     std::size_t volume;
    
//     std::vector<std::unique_ptr<Chunk>> chunks;
//     std::vector<std::unique_ptr<Mesh>> meshes;

//     explicit Level(int w, int h, int d, int ox, int oy, int oz);
//     ~Level() = default;

//     Level(const Level &other) = delete;
//     Level(Level &&other) = default;
//     Level &operator=(const Level &other) = delete;
//     Level &operator=(Level &&other) = default;

//     void set_center(int cx, int cy, int cz);
//     void translate(int dx, int dy, int dz);
//     Chunk *get_chunk(int cx, int cy, int cz) const;
//     Chunk *get_chunk_by_voxel(int world_x, int world_y, int world_z) const;

//     voxel *get_voxel(int world_x, int world_y, int world_z) const;
//     uint8_t get_voxel_id(int world_x, int world_y, int world_z) const;
//     uint8_t get_light(int world_x, int world_y, int world_z, int channel) const;

//     bool set_voxel(int world_x, int world_y, int world_z, uint8_t block_id);
//     void set(int world_x, int world_y, int world_z, uint8_t block_id);

//     bool load_visible(WorldFiles *world_files);
//     bool decorate_visible();
//     bool build_meshes(VoxelRenderer *renderer);

//     bool is_solid(int world_x, int world_y, int world_z) const;
//     bool is_obstacle(int world_x, int world_y, int world_z) const;
//     bool intersects_aabb(const glm::vec3 &min, const glm::vec3 &max) const;
    
//     std::optional<RaycastHit> raycast(const glm::vec3 &origin, const glm::vec3 &dir, float max_distance) const;

//     void update(const glm::vec3 &player_pos, WorldFiles *world_files, VoxelRenderer *renderer);
//     void ensure_loaded_around(int cx, int cy, int cz, WorldFiles *world_files);

//     std::size_t get_chunk_index(int cx, int cy, int cz) const;
    
// private:
//     std::vector<std::unique_ptr<Chunk>> chunks_second;
//     std::vector<std::unique_ptr<Mesh>> meshes_second;
// };