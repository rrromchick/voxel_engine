#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

struct Hitbox {
    vec3 position;
    vec3 halfsize;
    vec3 velocity;
    bool grounded = false;

    Hitbox(vec3 position, vec3 halfsize) 
        : position(position), halfsize(halfsize), velocity(0.0f, 0.0f, 0.0f) 
    {}
};