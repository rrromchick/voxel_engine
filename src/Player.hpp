#pragma once

#include "ECS.hpp"
#include "Components.hpp"
#include <glm/glm.hpp>

struct Player : public ECS::Object {
    using ECS::Object::Object;

    static Player create(const glm::vec3 &spawn_pos);

    TransformComponent &transform() const;
    HitboxComponent &hitbox() const;
    PlayerInputComponent &input() const;

    void move(const glm::vec2 &dir, float speed);
    void jump(float jump_force = 8.0f);
    void set_shifting(bool is_shifting);
    void set_swimming_up(bool is_swimming);
};