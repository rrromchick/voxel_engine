#pragma once

#include "Chunk.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <utility>
#include <unordered_map>
#include <string>
#include <string_view>
#include <span>
#include <bit>
#include <stdint.h>
#include <assert.h>

constexpr std::size_t REGION_SIZE_BIT = 5;
constexpr std::size_t REGION_SIZE = 1 << REGION_SIZE_BIT;
constexpr std::size_t REGION_VOL = REGION_SIZE * REGION_SIZE;

using ChunkData = std::unique_ptr<uint8_t[]>;
using RegionMap = std::array<ChunkData, REGION_VOL>;

struct RegionCoords {
    int x, y;
    bool operator==(const RegionCoords &other) const = default;
};

template <>
struct std::hash<RegionCoords> {
    std::size_t operator()(const RegionCoords &c) const noexcept {
        std::size_t h1 = std::hash<int>{}(c.x);
        std::size_t h2 = std::hash<int>{}(c.y);
        return h1 ^ (h2 << 1);
    }
};

struct WorldFiles {
    std::unordered_map<RegionCoords, std::unique_ptr<RegionMap>> regions;
    std::string directory;

    std::unique_ptr<uint8_t[]> main_buffer;
    std::size_t main_buffer_capacity;

    WorldFiles(std::string_view dir, std::size_t main_buffer_capacity);
    ~WorldFiles() = default;

    WorldFiles(const WorldFiles &other) = delete;
    WorldFiles(WorldFiles &&other) = default;
    WorldFiles &operator=(const WorldFiles &other) = delete;
    WorldFiles &operator=(WorldFiles &&other) = default;

    void put(std::span<const uint8_t> chunk_data, int x, int y);

    bool read_chunk(int x, int y, std::span<uint8_t> out);
    bool get_chunk(int x, int y, std::span<uint8_t> out);

    unsigned int write_region(std::span<uint8_t> out, int x, int y, RegionMap &region);
    void write();

    [[nodiscard]] std::string get_region_file(int x, int y) const;
};