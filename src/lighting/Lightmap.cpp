#include "Lightmap.hpp"
#include <cstring>

Lightmap::Lightmap() {
    map = std::make_unique<unsigned short[]>(Chunk::VOLUME);
    std::memset(map.get(), 0, Chunk::VOLUME * sizeof(unsigned short));
}