#include "line_batch.hpp"
#include "mesh.hpp"

#include <glad/glad.h>

constexpr auto LB_VERTEX_SIZE = 3 + 4;

LineBatch::LineBatch(usize capacity) : capacity(capacity), index(0) {
    constexpr std::array<int, 3> attrs = { 3, 4, 0 };
    buffer = std::make_unique<float[]>(capacity * LB_VERTEX_SIZE * 2);
    mesh = std::make_unique<Mesh>(std::span<const float> { buffer.get(), capacity * LB_VERTEX_SIZE * 2 }, attrs);
}

void LineBatch::line(float x1, float y1, float z1, float x2, float y2, float z2,
    float r, float g, float b, float a) {
    if (index + 2 * LB_VERTEX_SIZE >= capacity * LB_VERTEX_SIZE * 2) {
        return;
    }

    auto *first_vert = reinterpret_cast<Vertex*>(buffer.get() + index);
    *first_vert = { x1, y1, z1, r, g, b, a };
    index += LB_VERTEX_SIZE;

    auto *second_vert = reinterpret_cast<Vertex*>(buffer.get() + index);
    *second_vert = { x1, y1, z1, r, g, b, a };
    index += LB_VERTEX_SIZE;
}

void LineBatch::box(float x, float y, float z, float w, float h, float d,
    float r, float g, float b, float a) {
    w *= 0.5f;
    h *= 0.5f;
    d *= 0.5f;

    line(x-w, y-h, z-d, x+w, y-h, z-d, r, g, b, a);
    line(x-w, y+h, z-d, x+w, y+h, z-d, r, g, b, a);
    line(x-w, y-h, z+d, x+w, y-h, z+d, r, g, b, a);
    line(x-w, y+h, z+d, x+w, y+h, z+d, r, g, b, a);

    line(x-w, y-h, z-d, x-w, y+h, z-d, r, g, b, a);
    line(x+w, y-h, z-d, x+w, y+h, z-d, r, g, b, a);
    line(x-w, y-h, z+d, x-w, y+h, z+d, r, g, b, a);
    line(x+w, y-h, z+d, x+w, y+h, z+d, r, g, b, a);

    line(x-w, y-h, z-d, x-w, y-h, z+d, r, g, b, a);
    line(x+w, y-h, z-d, x+w, y-h, z+d, r, g, b, a);
    line(x-w, y+h, z-d, x-w, y+h, z+d, r, g, b, a);
    line(x+w, y+h, z-d, x+w, y+h, z+d, r, g, b, a);
}

void LineBatch::render() {
    if (index == 0) {
        return;
    }

    std::span<const float> buf { buffer.get(), index };
    mesh->reload(buf);
    mesh->draw(GL_LINES);
    index = 0;
}