#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>

using namespace glm;

struct Mesh;
struct VoxelRenderer;
struct WorldFiles;
struct Chunk;
struct voxel;

struct Chunks {
    std::vector<std::unique_ptr<Chunk>> chunks;
    std::vector<std::unique_ptr<Chunk>> chunks_second;
    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<std::unique_ptr<Mesh>> meshes_second;

    std::size_t volume;
    unsigned int w, h, d;
    int ox, oy, oz;

    explicit Chunks(int w, int h, int d, int ox, int oy, int oz);
    ~Chunks() = default;

    Chunks(const Chunks &other) = delete;
    Chunks(Chunks &&other) = default;
    Chunks &operator=(const Chunks &other) = delete;
    Chunks &operator=(Chunks &&other) = default;

    Chunk *get_chunk(int x, int y, int z);
    Chunk *get_chunk_by_voxel(int x, int y, int z);
    voxel *get(int x, int y, int z);
    void set(int x, int y, int z, int id);

    unsigned char get_light(int x, int y, int z, int channel);

    voxel *ray_cast(vec3 start, vec3 dir, float max_length, vec3 &end, 
        vec3 &norm, vec3 &iend);

    void set_center(int x, int y, int z);
    void translate(int x, int y, int z);

    bool load_visible(WorldFiles *world_files);
    bool build_meshes(VoxelRenderer *voxel_renderer);

    bool is_obstacle(int x, int y, int z);
};