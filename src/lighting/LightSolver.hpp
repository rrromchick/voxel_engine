#pragma once

#include <queue>

struct LightEntry {
    int x, y, z;
    unsigned char light;
};

struct LightSolver {
    explicit LightSolver(int channel)
        : channel(channel) {}

    ~LightSolver() = default;

    LightSolver(const LightSolver &other) = delete;
    LightSolver(LightSolver &&other) = default;
    LightSolver &operator=(const LightSolver &other) = delete;
    LightSolver &operator=(LightSolver &&other) = default;

    void add(int x, int y, int z);
    void add(int x, int y, int z, int emission);
    void remove(int x, int y, int z);

    void solve();

private:
    std::queue<LightEntry> add_queue;
    std::queue<LightEntry> rem_queue;
    int channel;
};