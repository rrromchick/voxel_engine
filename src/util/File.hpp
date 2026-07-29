#pragma once

#include <filesystem>
#include <span>
#include <fstream>

namespace fs = std::filesystem;

struct File {
    static bool write_binary_file_part(
        const fs::path &filename, std::span<uint8_t> data, std::size_t offset) {
        std::ofstream output(filename, std::ios::out | std::ios::binary | std::ios::in);
        if (!output.is_open()) {
            return false;
        }
        output.seekp(static_cast<std::streamoff>(offset));
        return static_cast<bool>(output.write(
            reinterpret_cast<const char *>(data.data()), data.size()));
    }

    static bool write_binary_file(const fs::path &filename, std::span<const uint8_t> data) {
        std::ofstream output(filename, std::ios::binary);
        if (!output.is_open()) {
            return false;
        }
        return static_cast<bool>(output.write(
            reinterpret_cast<const char *>(data.data()), data.size()));
    }

    static std::size_t append_binary_file(const fs::path &filename, std::span<const uint8_t> data) {
        std::ofstream output(filename, std::ios::binary | std::ios::app);
        if (!output.is_open()) {
            return 0;
        }

        const auto position = output.tellp();
        if (!output.write(reinterpret_cast<const char *>(data.data()), data.size())) {
            return 0;
        }
        return static_cast<std::size_t>(position);
    }

    static bool read_binary_file(const fs::path &filename, std::span<uint8_t> data, std::size_t offset) {
        std::ifstream input(filename, std::ios::binary);
        if (!input.is_open()) {
            return false;
        }

        if (offset > 0) {
            input.seekg(static_cast<std::streamoff>(offset));
        }

        return static_cast<bool>(input.read(
            reinterpret_cast<char*>(data.data()), data.size()));
    }

    static bool read_binary_file(const fs::path &filename, std::span<uint8_t> data) {
        return read_binary_file(filename, data, 0);
    }

    static std::size_t decompress_rle(std::span<const uint8_t> src, std::span<uint8_t> dst) {
        std::size_t out_offset = 0;

        for (std::size_t i = 0; i + 1 < src.size(); i += 2) {
            auto repeat_count = src[i];
            auto byte_value = src[i + 1];

            auto total_bytes = static_cast<std::size_t>(repeat_count) + 1;
            if (out_offset + total_bytes > dst.size()) {
                break;
            }

            std::fill_n(dst.data() + out_offset, total_bytes, byte_value);
            out_offset += total_bytes;
        }

        return out_offset;
    }

    static std::size_t calc_rle(std::span<const uint8_t> src) {
        if (src.empty()) {
            return 0;
        }

        std::size_t compressed_size = 0;
        uint32_t counter = 1;
        uint8_t current_byte = src[0];

        for (std::size_t i = 1; i < src.size(); i++) {
            uint8_t next_byte = src[i];
            if (next_byte != current_byte || counter == 256) {
                compressed_size += 2;
                current_byte = next_byte;
                counter = 0;
            }
            counter++;
        }

        return compressed_size + 2;
    }

    static std::size_t compress_rle(std::span<const uint8_t> src, std::span<uint8_t> dst) {
        if (src.empty()) {
            return 0;
        }

        std::size_t out_offset = 0;
        uint32_t counter = 1;
        uint8_t current_byte = src[0];

        for (std::size_t i = 1; i < src.size(); i++) {
            uint8_t next_byte = src[i];
            if (next_byte != current_byte || counter == 256) {
                if (out_offset + 2 > dst.size()) return out_offset;

                dst[out_offset++] = static_cast<uint8_t>(counter - 1);
                dst[out_offset++] = current_byte;

                current_byte = next_byte;
                counter = 0;
            }
            counter++;
        }

        if (out_offset + 2 <= dst.size()) {
            dst[out_offset++] = static_cast<uint8_t>(counter - 1);
            dst[out_offset++] = current_byte;
        }

        return out_offset;
    }
};