#pragma once

#include "Chunk.hpp"

struct Lightmap {
    std::unique_ptr<unsigned short[]> map;

    Lightmap();
    ~Lightmap() = default;

    Lightmap(const Lightmap &other) = delete;
    Lightmap(Lightmap &&other) = default;
    Lightmap &operator=(const Lightmap &other) = delete;
    Lightmap &operator=(Lightmap &&other) = default;

    inline unsigned char get(int x, int y, int z, int channel) {
        return (map[y * Chunk::DEPTH * Chunk::WIDTH + z * Chunk::WIDTH + x] 
            >> (channel << 2)) & 0xF;
    }

    inline unsigned char get_r(int x, int y, int z) {
        return map[y * Chunk::DEPTH * Chunk::WIDTH + z * Chunk::WIDTH + x] & 0xF;
    }

    inline unsigned char get_g(int x, int y, int z) {
        return (map[y * Chunk::DEPTH * Chunk::WIDTH + z * Chunk::WIDTH + x] >> 4) & 0xF;
    }

    inline unsigned char get_b(int x, int y, int z) {
        return (map[y * Chunk::DEPTH * Chunk::WIDTH + z * Chunk::WIDTH + x] >> 8) & 0xF;
    }

    inline unsigned char get_s(int x, int y, int z) {
        return (map[y * Chunk::DEPTH * Chunk::WIDTH + z * Chunk::WIDTH + x] >> 12) & 0xF;
    }

    inline void set_r(int x, int y, int z, int value) {
        const auto index = y * Chunk::DEPTH * Chunk::WIDTH + z * Chunk::WIDTH + x;
        map[index] = (map[index] & 0xFFF0) | value;
    }

    inline void set_g(int x, int y, int z, int value) {
        const auto index = y * Chunk::DEPTH * Chunk::WIDTH + z * Chunk::WIDTH + x;
        map[index] = (map[index] & 0xFF0F) | (value << 4);
    }

    inline void set_b(int x, int y, int z, int value) {
        const auto index = y * Chunk::DEPTH * Chunk::WIDTH + z * Chunk::WIDTH + x;
        map[index] = (map[index] & 0xF0FF) | (value << 8);
    }

    inline void set_s(int x, int y, int z, int value) {
        const auto index = y * Chunk::DEPTH * Chunk::WIDTH + z * Chunk::WIDTH + x;
        map[index] = (map[index] & 0x0FFF) | (value << 12);
    }

    inline void set(int x, int y, int z, int channel, int value) {
        const auto index = y * Chunk::DEPTH * Chunk::WIDTH + z * Chunk::WIDTH + x;
        map[index] = (map[index] & (0xFFFF & (~(0xF << (channel * 4))))) |
            (value << (channel << 2));
    }
};