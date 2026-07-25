#pragma once

#include <memory>

struct LightSolver;

struct Lighting {
    Lighting();
    ~Lighting();

    Lighting(const Lighting &other) = delete;
    Lighting(Lighting &&other) = default;
    Lighting &operator=(const Lighting &other) = delete;
    Lighting &operator=(Lighting &&other) = default;

    void clear();
    void on_chunk_loaded(int cx, int cy, int cz);
    void on_block_set(int x, int y, int z, int id);

private:
    std::unique_ptr<LightSolver> solver_r;
    std::unique_ptr<LightSolver> solver_g;
    std::unique_ptr<LightSolver> solver_b;
    std::unique_ptr<LightSolver> solver_s;
};