#pragma once

#include <glm/glm.hpp>
#include "Global.hpp"
#include "Player.hpp"

struct Level;

enum class FluidType {
    NONE, WATER, LAVA
};

struct PhysicsSolver {
    explicit PhysicsSolver(glm::vec3 gravity);
    
    void step(float delta, unsigned int substeps);

    static FluidType check_fluid(Level *level, const glm::vec3 &pos, const glm::vec3 &half);
    static bool is_block_inside(int x, int y, int z, const glm::vec3 &pos, const glm::vec3 &half);

private:
    glm::vec3 gravity;

    void step_entity(
        TransformComponent &trans,
        HitboxComponent &hitbox,
        PlayerInputComponent *input,
        Level *level,
        float dt);
};