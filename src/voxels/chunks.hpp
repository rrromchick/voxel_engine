#pragma once

#include "std.hpp"
#include "typedefs.hpp"
#include <glm/glm.hpp>

using namespace glm;

struct Chunk;
struct voxel;

struct Chunks {
    std::vector<std::unique_ptr<Chunk>> chunks;
    usize volume;
    unsigned int w, h, d;

    explicit Chunks(int w, int h, int d);
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

    void write(unsigned char *dest);
    void read(unsigned char *source);
};