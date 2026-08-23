#pragma once

#include "ECS.hpp"
#include <glm/glm.hpp>

struct TransformComponent : public ECS::Component<TransformComponent> {
    glm::vec3 position{0.0f};
};

struct HitboxComponent : public ECS::Component<HitboxComponent> {
    glm::vec3 halfsize{0.3f, 0.9f, 0.3f};
    glm::vec3 velocity{0.0f};
    bool grounded{false};
};

struct PlayerInputComponent : public ECS::Component<PlayerInputComponent> {
    bool shifting{false};
    bool is_swimming_up{false};
};

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