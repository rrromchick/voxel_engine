#pragma once

#include <array>

struct Block {
    unsigned int id;

    std::array<int, 6> texture_faces;
    std::array<unsigned char, 3> emission;

    unsigned char draw_group = 0;
    bool light_passing = false;

    explicit Block(unsigned int id, int texture)
        : id(id) {
        for (std::size_t i = 0; i < texture_faces.size(); i++) {
            texture_faces[i] = texture;
        }

        for (std::size_t i = 0; i < emission.size(); i++) {
            emission[i] = 0;
        }
    }

    ~Block() = default;

    Block(const Block &other) = delete;
    Block(Block &&other) = default;
    Block &operator=(const Block &other) = delete;
    Block &operator=(Block &&other) = default;
};