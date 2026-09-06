#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <optional>
#include <glm/glm.hpp>

enum class PacketType : uint8_t {
    Unknown = 0,
    PlayerState = 1,
    BlockModify = 2,
    EntitySpawn = 3,
    EntityDestroy = 4,
    EntityStateUpdate = 5
};

struct Packet {
    Packet() = default;

    explicit Packet(PacketType type) {
        write_type(type);
    }

    Packet(const Packet &other) = delete;
    
    Packet(Packet &&other) noexcept
        : buffer(std::move(other.buffer)),
          read_offset(other.read_offset),
          has_read_error(other.has_read_error) {
        other.read_offset = 0;
        other.has_read_error = false;
    }

    Packet &operator=(const Packet &other) = delete;

    Packet &operator=(Packet &&other) noexcept {
        if (this != &other) {
            buffer = std::move(other.buffer);
            read_offset = other.read_offset;
            has_read_error = other.has_read_error;
            other.read_offset = 0;
            other.has_read_error = false;
        }
        return *this;
    }

    PacketType get_type() const {
        if (buffer.empty()) {
            return PacketType::Unknown;
        }
        return static_cast<PacketType>(buffer[0]);
    }

    void write_type(PacketType type) {
        write<uint8_t>(static_cast<uint8_t>(type));
    }

    template <typename T>
    void write(const T &value) {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        auto *bytes = reinterpret_cast<const uint8_t*>(&value);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(T));
    }

    template <typename T>
    T read() {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        T value {};
        if (read_offset + sizeof(T) <= buffer.size()) {
            std::memcpy(&value, &buffer[read_offset], sizeof(T));
            read_offset += sizeof(T);
        } else {
            has_read_error = true;
        }
        return value;
    }

    void write_vec3(const glm::vec3 &vec) {
        write<float>(vec.x);
        write<float>(vec.y);
        write<float>(vec.z);
    }

    glm::vec3 read_vec3() {
        auto x = read<float>();
        auto y = read<float>();
        auto z = read<float>();
        return glm::vec3(x, y, z);
    }

    void write_string(const std::string &str) {
        write<uint16_t>(static_cast<uint16_t>(str.size()));
        buffer.insert(buffer.end(), str.begin(), str.end());
    }

    std::string read_string() {
        auto length = read<uint16_t>();
        if (has_read_error || read_offset + length > buffer.size()) {
            has_read_error = true;
            return "";
        }
        std::string str(reinterpret_cast<const char*>(&buffer[read_offset]), length);
        read_offset += length;
        return str;
    }
    
    const uint8_t *data() const {
        return buffer.data();
    }

    const char *as_char_ptr() const {
        return reinterpret_cast<const char*>(buffer.data());
    }

    char *data_ptr() {
        return reinterpret_cast<char*>(buffer.data());
    }

    std::size_t size() const {
        return buffer.size();
    }

    void assign(const uint8_t *data, std::size_t length) {
        buffer.assign(data, data + length);
        reset_read();
    }

    void reset_read() {
        read_offset = (buffer.size() >= sizeof(uint8_t)) ? sizeof(uint8_t) : 0;
        has_read_error = false;
    }

    bool bad() const { return has_read_error; }

private:
    std::vector<uint8_t> buffer;
    std::size_t read_offset = 0;
    bool has_read_error = false;
};