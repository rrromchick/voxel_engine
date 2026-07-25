#pragma once

#include <memory>
#include <array>
#include <span>

struct Mesh;

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

struct LineBatch {
    explicit LineBatch(std::size_t capacity);
    ~LineBatch() = default;

    LineBatch(const LineBatch &other) = delete;
    LineBatch(LineBatch &&other) = default;
    LineBatch &operator=(const LineBatch &other) = delete;
    LineBatch &operator=(LineBatch &&other) = default;

    void line(float x1, float y1, float z1, float x2, float y2, float z2,
        float r, float g, float b, float a);

    void box(float x, float y, float z, float w, float h, float d,
        float r, float g, float b, float a);

    void render();

private:
    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<float[]> buffer;
    std::size_t index;
    std::size_t capacity;
};