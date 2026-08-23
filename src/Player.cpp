#include "Player.hpp"
#include "Global.hpp"

Player Player::create(const glm::vec3 &spawn_pos) {
    auto opt_obj = global.ecs->create();
    if (!opt_obj) {
        return Player(); 
    }

    Player player;
    player.p = opt_obj->p;
    player.id = opt_obj->id;

    auto &trans = player.add<TransformComponent>();
    trans.position = spawn_pos;

    player.add<HitboxComponent>();
    player.add<PlayerInputComponent>();

    return player;
}

TransformComponent &Player::transform() const {
    return get<TransformComponent>();
}

HitboxComponent &Player::hitbox() const {
    return get<HitboxComponent>();
}

PlayerInputComponent &Player::input() const {
    return get<PlayerInputComponent>();
}

void Player::move(const glm::vec2 &dir, float speed) {
    if (glm::length(dir) > 0.0001f) {
        glm::vec2 norm_dir = glm::normalize(dir);
        hitbox().velocity.x = norm_dir.x * speed;
        hitbox().velocity.z = norm_dir.y * speed;
    } else {
        hitbox().velocity.x = 0.0f;
        hitbox().velocity.z = 0.0f;
    }
}

void Player::jump(float jump_force) {
    auto &hb = hitbox();
    if (hb.grounded) {
        hb.velocity.y = jump_force;
        hb.grounded = false;
    }
}

void Player::set_shifting(bool is_shifting) {
    input().shifting = is_shifting;
}

void Player::set_swimming_up(bool is_swimming) {
    input().is_swimming_up = is_swimming;
}