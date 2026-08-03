#include "WorldFiles.hpp"
#include "File.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>

static uint32_t bytes_to_uint32(std::span<const uint8_t> src) {
    uint32_t value;
    std::memcpy(&value, src.data(), sizeof(uint32_t));
    if constexpr (std::endian::native == std::endian::little) {
        return std::byteswap(value);
    }
    return value;
}

static void uint32_to_bytes(uint32_t value, std::span<uint8_t> dest) {
    if constexpr (std::endian::native == std::endian::little) {
        value = std::byteswap(value);
    }
    std::memcpy(dest.data(), &value, sizeof(uint32_t));
}

WorldFiles::WorldFiles(std::string_view dir, std::size_t capacity)
    : directory(dir), main_buffer_capacity(capacity) {
    main_buffer = std::make_unique<uint8_t[]>(capacity);
}

std::string WorldFiles::get_region_file(int rx, int rz) const {
    return directory + std::to_string(rx) + "_" + std::to_string(rz) + ".bin";
}

static bool calculate_chunk_index(int x, int y, int z, int &region_x, int &region_z, std::size_t &out_idx) {
    if (y < 0 || y >= static_cast<int>(REGION_HEIGHT)) {
        return false;
    }

    region_x = x >> REGION_SIZE_BIT;
    region_z = z >> REGION_SIZE_BIT;

    int local_x = x - (region_x << REGION_SIZE_BIT);
    int local_z = z - (region_z << REGION_SIZE_BIT);

    out_idx = (static_cast<std::size_t>(y) * REGION_SIZE + local_z) * REGION_SIZE + local_x;
    return true;
}

void WorldFiles::put(std::span<const uint8_t> chunk_data, int x, int y, int z) {
    assert(chunk_data.size() >= Chunk::VOLUME);

    int region_x, region_z;
    std::size_t chunk_idx;
    if (!calculate_chunk_index(x, y, z, region_x, region_z, chunk_idx)) {
        return;
    }

    auto &region_ptr = regions[{ region_x, region_z }];
    if (!region_ptr) {
        region_ptr = std::make_unique<RegionMap>();
    }

    auto &region = *region_ptr;
    auto &chunk = region[chunk_idx];

    if (!chunk) {
        chunk = std::make_unique<uint8_t[]>(Chunk::VOLUME);
    }

    std::copy_n(chunk_data.begin(), Chunk::VOLUME, chunk.get());
}

bool WorldFiles::get_chunk(int x, int y, int z, std::span<uint8_t> out) {
    assert(out.size() >= Chunk::VOLUME);

    int region_x, region_z;
    std::size_t chunk_idx;
    if (!calculate_chunk_index(x, y, z, region_x, region_z, chunk_idx)) {
        return false;
    }

    auto it = regions.find({ region_x, region_z });
    if (it == regions.end() || !it->second) {
        return read_chunk(x, y, z, out);
    }

    const auto &chunk_ptr = (*it->second)[chunk_idx];
    if (!chunk_ptr) {
        return read_chunk(x, y, z, out);
    }

    std::copy_n(chunk_ptr.get(), Chunk::VOLUME, out.begin());
    return true;
}

bool WorldFiles::read_chunk(int x, int y, int z, std::span<uint8_t> out) {
    assert(out.size() >= Chunk::VOLUME);

    int region_x, region_z;
    std::size_t chunk_index;
    if (!calculate_chunk_index(x, y, z, region_x, region_z, chunk_index)) {
        return false;
    }

    std::string filename = get_region_file(region_x, region_z);
    std::ifstream input(filename, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }

    uint32_t raw_offset = 0;
    input.seekg(chunk_index * 4);
    input.read(reinterpret_cast<char*>(&raw_offset), 4);
    if (input.gcount() != 4) return false;

    uint32_t offset = bytes_to_uint32(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(&raw_offset), 4));
    if (offset == 0) return false;

    input.seekg(offset);
    input.read(reinterpret_cast<char*>(&raw_offset), 4);
    if (input.gcount() != 4) return false;

    std::size_t compressed_size = bytes_to_uint32(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(&raw_offset), 4));

    if (compressed_size == 0 || compressed_size > Chunk::VOLUME * 2) {
        return false;
    }

    auto compressed_buffer = std::make_unique<uint8_t[]>(compressed_size);
    input.read(reinterpret_cast<char*>(compressed_buffer.get()), compressed_size);
    if (static_cast<std::size_t>(input.gcount()) != compressed_size) return false;

    std::span<const uint8_t> buffer_span{
        reinterpret_cast<const uint8_t*>(compressed_buffer.get()), compressed_size };
    File::decompress_rle(buffer_span, out);
    return true;
}

void WorldFiles::write() {
    for (auto &[coords, region_ptr] : regions) {
        if (!region_ptr) continue;

        std::span<uint8_t> buffer_span { main_buffer.get(), main_buffer_capacity };
        unsigned int size = write_region(buffer_span, coords.x, coords.z, *region_ptr);
        if (size > 0) {
            File::write_binary_file(get_region_file(coords.x, coords.z),
                buffer_span.first(size));
        }
    }
}

unsigned int WorldFiles::write_region(
    std::span<uint8_t> out, int rx, int rz, RegionMap &region) {
    unsigned int offset = REGION_VOL * 4;
    if (offset > out.size()) return 0;

    std::fill_n(out.begin(), offset, 0);

    std::vector<uint8_t> compressed(Chunk::VOLUME * 2);

    for (std::size_t i = 0; i < REGION_VOL; i++) {
        auto &chunk_ptr = region[i];

        if (!chunk_ptr) {
            auto temp_chunk = std::make_unique<uint8_t[]>(Chunk::VOLUME);

            auto local_x = i % REGION_SIZE;
            auto local_z = (i / REGION_SIZE) % REGION_SIZE;
            auto local_y = static_cast<int>(i / (REGION_SIZE * REGION_SIZE));

            int chunk_x = local_x + (rx << REGION_SIZE_BIT);
            int chunk_z = local_z + (rz << REGION_SIZE_BIT);
            int chunk_y = local_y;

            if (read_chunk(chunk_x, chunk_y, chunk_z, std::span<uint8_t>(
                    temp_chunk.get(), Chunk::VOLUME))) {
                chunk_ptr = std::move(temp_chunk);
            }
        }

        if (!chunk_ptr) {
            uint32_to_bytes(0, out.subspan(i * 4, 4));
        } else {
            auto compressed_size = static_cast<unsigned int>(File::compress_rle(
                std::span<const uint8_t>(chunk_ptr.get(), Chunk::VOLUME), compressed));

            if (offset + 4 + compressed_size > out.size()) {
                return 0; // Buffer capacity safety check
            }

            uint32_to_bytes(offset, out.subspan(i * 4, 4));
            uint32_to_bytes(compressed_size, out.subspan(offset, 4));
            offset += 4;

            std::copy_n(compressed.begin(), compressed_size, out.begin() + offset);
            offset += compressed_size;
        }
    }
    return offset;
}