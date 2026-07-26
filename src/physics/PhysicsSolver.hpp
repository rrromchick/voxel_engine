#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Global.hpp"

using namespace glm;

struct Hitbox;

struct PhysicsSolver {
    explicit PhysicsSolver(vec3 gravity);
    ~PhysicsSolver() = default;

    PhysicsSolver(const PhysicsSolver &other) = delete;
    PhysicsSolver(PhysicsSolver &&other) = default;
    PhysicsSolver &operator=(const PhysicsSolver &other) = delete;
    PhysicsSolver &operator=(PhysicsSolver &&other) = default;

    void step(Hitbox *hitbox, float delta, unsigned int substeps, bool shifting);
    bool is_block_inside(int x, int y, int z, Hitbox *hitbox);

private:
    vec3 gravity;
};