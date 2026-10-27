#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

enum class AnimationState {
    IDLE,
    WALKING,
    COMBAT
};

struct LimbAngles {
    float left_hand = 0.0f;
    float right_hand = 0.0f;
    float left_foot = 0.0f;
    float right_foot = 0.0f;
};

struct Animator {
private:
    float walk_cycle = 0.0f;
    float attack_timer = 0.0f;
    AnimationState current_state = AnimationState::IDLE;

public:
    void update(const glm::vec3 &velocity, float dt) {
        float horizontal_speed = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
        if (horizontal_speed > 0.05f) {
            current_state = AnimationState::WALKING;
            walk_cycle += 15.0f * dt;
        } else {
            current_state = AnimationState::IDLE;
            walk_cycle = glm::mix(walk_cycle, 0.0f, 0.15f);
        }

        if (attack_timer > 0.0f) {
            attack_timer -= dt;
        }
    }

    void trigger_attack() {
        attack_timer = 0.3f;
    }

    void set_state(AnimationState state) {
        current_state = state;
    }

    LimbAngles get_limb_angles() const {
        LimbAngles angles;
        
        float left_arm_swing  = -std::sin(walk_cycle) * 0.4f;
        float right_arm_swing = -std::sin(walk_cycle + 3.14159f) * 0.4f;
        
        float left_foot_swing  = std::sin(walk_cycle + 3.14159f) * 0.3f;
        float right_foot_swing = std::sin(walk_cycle) * 0.3f;

        if (current_state == AnimationState::WALKING) {
            angles.left_hand  = left_arm_swing;
            angles.right_hand = right_arm_swing;
            angles.left_foot  = left_foot_swing;
            angles.right_foot = right_foot_swing;
        } else {
            angles.left_hand  = 0.0f;
            angles.right_hand = 0.0f;
            angles.left_foot  = 0.0f;
            angles.right_foot = 0.0f;
        }

        if (attack_timer > 0.0f) {
            float progress = attack_timer / 0.3f; 
            float attack_curve = std::sin(progress * 3.14159f) * 1.5f; 
            angles.right_hand += attack_curve;
        }

        return angles;
    }
};