#include "Lightmap.hpp"

Lightmap::Lightmap() {
    map = std::make_unique<unsigned short[]>(Chunk::VOLUME);
    for (usize i = 0; i < Chunk::VOLUME; i++) {
        map[i] = 0x0000;
    }
}