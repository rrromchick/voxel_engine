#include "WorldFiles.hpp"
#include "File.hpp"

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

void WorldFiles::put(std::span<const uint8_t> chunk_data, int x, int y) {
    assert(chunk_data.size() >= Chunk::VOLUME);

    int region_x = x >> REGION_SIZE_BIT;
    int region_y = y >> REGION_SIZE_BIT;

    int local_x = x - (region_x << REGION_SIZE_BIT);
    int local_y = y - (region_y << REGION_SIZE_BIT);

    auto &region_ptr = regions[{ region_x, region_y }];
    if (!region_ptr) {
        region_ptr = std::make_unique<RegionMap>();
    }

    auto &region = *region_ptr;

    std::size_t chunk_idx = local_y * REGION_SIZE + local_x;
    auto &chunk = region[chunk_idx];

    if (!chunk) {
        chunk = std::make_unique<uint8_t[]>(Chunk::VOLUME);
    }

    std::copy_n(chunk_data.begin(), Chunk::VOLUME, chunk.get());
}

std::string WorldFiles::get_region_file(int x, int y) const {
    return directory + std::to_string(x) + "_" + std::to_string(y) + ".bin";
}

bool WorldFiles::get_chunk(int x, int y, std::span<uint8_t> out) {
    assert(out.size() >= Chunk::VOLUME);

    int region_x = x >> REGION_SIZE_BIT;
    int region_y = y >> REGION_SIZE_BIT;

    int local_x = x - (region_x << REGION_SIZE_BIT);
    int local_y = y - (region_y << REGION_SIZE_BIT);
    std::size_t chunk_index = local_y * REGION_SIZE + local_x;
    assert(chunk_index < REGION_VOL);

    auto it = regions.find({ region_x, region_y });
    if (it == regions.end() || !it->second) {
        return read_chunk(x, y, out);
    }

    const auto &chunk_ptr = (*it->second)[chunk_index];
    if (!chunk_ptr) {
        return read_chunk(x, y, out);
    }

    std::copy_n(chunk_ptr.get(), Chunk::VOLUME, out.begin());
    return true;
}

bool WorldFiles::read_chunk(int x, int y, std::span<uint8_t> out) {
    assert(out.size() >= Chunk::VOLUME);

    int region_x = x >> REGION_SIZE_BIT;
    int region_y = y >> REGION_SIZE_BIT;

    int local_x = x - (region_x << REGION_SIZE_BIT);
    int local_y = y - (region_y << REGION_SIZE_BIT);
    std::size_t chunk_index = local_y * REGION_SIZE + local_x;

    std::string filename = get_region_file(region_x, region_y);
    std::ifstream input(filename, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }

    uint32_t raw_offset = 0;
    input.seekg(chunk_index * 4);
    input.read(reinterpret_cast<char*>(&raw_offset), 4);

    uint32_t offset = bytes_to_uint32(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(&raw_offset), 4));
    if (offset == 0) return false;

    input.seekg(offset);
    input.read(reinterpret_cast<char*>(&raw_offset), 4);
    std::size_t compressed_size = bytes_to_uint32(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(&raw_offset), 4));

    input.read(reinterpret_cast<char*>(main_buffer.get()), compressed_size);
    File::decompress_rle(std::span<const uint8_t>(main_buffer.get(), compressed_size), out);
    return true;
}

void WorldFiles::write() {
    for (auto &[coords, region_ptr] : regions) {
        if (!region_ptr) continue;
        
        std::span<uint8_t> buffer_span { main_buffer.get(), main_buffer_capacity };
        unsigned int size = write_region(buffer_span, coords.x, coords.y, *region_ptr);
        File::write_binary_file(get_region_file(coords.x, coords.y), 
            buffer_span.first(size));
    }
}

unsigned int WorldFiles::write_region(
    std::span<uint8_t> out, int x, int y, RegionMap &region) {
    unsigned int offset = REGION_VOL * 4;
    std::fill_n(out.begin(), offset, 0);

    std::vector<uint8_t> compressed(Chunk::VOLUME * 2);

    for (std::size_t i = 0; i < REGION_VOL; i++) {
        auto &chunk_ptr = region[i];

        if (!chunk_ptr) {
            auto temp_chunk = std::make_unique<uint8_t[]>(Chunk::VOLUME);
            int chunk_x = (i % REGION_SIZE) + x * REGION_SIZE;
            int chunk_y = (i / REGION_SIZE) + y * REGION_SIZE;

            if (read_chunk(chunk_x, chunk_y, std::span<uint8_t>(
                    temp_chunk.get(), Chunk::VOLUME))) {
                chunk_ptr = std::move(temp_chunk);
            }
        }

        if (!chunk_ptr) {
            uint32_to_bytes(0, out.subspan(i * 4, 4));
        } else {
            uint32_to_bytes(offset, out.subspan(i * 4, 4));

            unsigned int compressed_size = File::compress_rle(
                std::span<const uint8_t>(chunk_ptr.get(), Chunk::VOLUME), compressed);

            uint32_to_bytes(compressed_size, out.subspan(offset, 4));
            offset += 4;

            std::copy_n(compressed.begin(), compressed_size, out.begin() + offset);
            offset += compressed_size;
        }
    }
    return offset;
}