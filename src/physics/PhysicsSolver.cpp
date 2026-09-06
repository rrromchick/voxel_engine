#include "PhysicsSolver.hpp"
#include "Global.hpp"
#include "Level.hpp"
#include "Components.hpp"
#include <algorithm>
#include <cmath>

constexpr auto E = 0.001f;
constexpr float STEP_HEIGHT = 0.5f;

PhysicsSolver::PhysicsSolver(glm::vec3 gravity) : gravity(gravity) {}

FluidType PhysicsSolver::check_fluid(Level *level, const glm::vec3 &pos, const glm::vec3 &half) {
    if (!level) return FluidType::NONE;

    auto min_x = static_cast<int>(std::floor(pos.x - half.x + E));
    auto max_x = static_cast<int>(std::floor(pos.x + half.x - E));
    auto min_y = static_cast<int>(std::floor(pos.y - half.y + E));
    auto max_y = static_cast<int>(std::floor(pos.y + half.y - E));
    auto min_z = static_cast<int>(std::floor(pos.z - half.z + E));
    auto max_z = static_cast<int>(std::floor(pos.z + half.z - E));

    bool in_water = false;
    bool in_lava = false;

    for (int x = min_x; x <= max_x; x++) {
        for (int y = min_y; y <= max_y; y++) {
            for (int z = min_z; z <= max_z; z++) {
                auto *vox = level->get_voxel(x, y, z);
                if (vox) {
                    if (vox->id == BlockId::WATER) in_water = true;
                    if (vox->id == BlockId::LAVA) in_lava = true;
                }
            }
        }
    }

    if (in_lava) return FluidType::LAVA;
    if (in_water) return FluidType::WATER;
    return FluidType::NONE;
}

bool PhysicsSolver::is_block_inside(int x, int y, int z, const glm::vec3 &pos, const glm::vec3 &half) {
    auto min_x = static_cast<int>(std::floor(pos.x - half.x + E));
    auto max_x = static_cast<int>(std::floor(pos.x + half.x - E));
    auto min_y = static_cast<int>(std::floor(pos.y - half.y + E));
    auto max_y = static_cast<int>(std::floor(pos.y + half.y - E));
    auto min_z = static_cast<int>(std::floor(pos.z - half.z + E));
    auto max_z = static_cast<int>(std::floor(pos.z + half.z - E));

    return x >= min_x && x <= max_x &&
           y >= min_y && y <= max_y &&
           z >= min_z && z <= max_z;
}

void PhysicsSolver::step(float delta, unsigned int substeps) {
    if (substeps == 0 || !global.ecs || !global.ecs->level) return;

    float dt = delta / static_cast<float>(substeps);
    auto &ecs = *global.ecs;
    auto *level = ecs.level.get();

    for (unsigned int sub = 0; sub < substeps; sub++) {
        for (std::size_t obj_id = 0; obj_id < ecs.size; ++obj_id) {
            auto obj = typename ECS::Object{ &ecs, static_cast<EntityId>(obj_id) };

            if (!obj.p) continue;

            if (obj.template has<TransformComponent>() && obj.template has<HitboxComponent>()) {
                auto &trans = obj.template get<TransformComponent>();
                auto &hitbox = obj.template get<HitboxComponent>();
                auto *input = obj.template opt<PlayerInputComponent>();

                step_entity(trans, hitbox, input, level, dt);
            }
        }
    }
}

void PhysicsSolver::step_entity(
    TransformComponent &trans,
    HitboxComponent &hitbox,
    PlayerInputComponent *input,
    Level *level,
    float dt
) {
    auto &pos = trans.position;
    auto &half = hitbox.halfsize;
    auto &vel = hitbox.velocity;

    bool shifting = input ? input->shifting : false;
    bool is_swimming_up = input ? input->is_swimming_up : false;

    auto prev_x = pos.x;
    auto prev_z = pos.z;

    auto fluid = check_fluid(level, pos, half);

    auto check_collision = [&](const glm::vec3 &p) -> bool {
        auto min_x = static_cast<int>(std::floor(p.x - half.x + E));
        auto max_x = static_cast<int>(std::floor(p.x + half.x - E));
        auto min_y = static_cast<int>(std::floor(p.y - half.y + E));
        auto max_y = static_cast<int>(std::floor(p.y + half.y - E));
        auto min_z = static_cast<int>(std::floor(p.z - half.z + E));
        auto max_z = static_cast<int>(std::floor(p.z + half.z - E));

        for (int x = min_x; x <= max_x; x++) {
            for (int y = min_y; y <= max_y; y++) {
                for (int z = min_z; z <= max_z; z++) {
                    if (level->is_obstacle(x, y, z)) return true;
                }
            }
        }
        return false;
    };

    if (fluid != FluidType::NONE) {
        float gravity_scale = (fluid == FluidType::WATER) ? 0.3f : 0.1f;
        vel.y += gravity.y * gravity_scale * dt;

        if (is_swimming_up) {
            auto head_x = static_cast<int>(std::floor(pos.x));
            auto head_y = static_cast<int>(std::floor(pos.y + half.y * 2.0f + 0.1f));
            auto head_z = static_cast<int>(std::floor(pos.z));

            auto *block_above = level->get_voxel(head_x, head_y, head_z);
            bool near_surface = (!block_above || block_above->id == BlockId::AIR);

            float max_swim_speed = near_surface ? 3.5f : 2.5f;
            float swim_accel = (fluid == FluidType::WATER) ? 25.0f : 12.0f;

            vel.y = std::min(vel.y + swim_accel * dt, max_swim_speed);
        }

        float h_drag = (fluid == FluidType::WATER) ? 6.0f : 14.0f;
        float v_drag = (fluid == FluidType::WATER) ? 3.0f : 8.0f;

        vel.x *= std::max(0.0f, 1.0f - h_drag * dt);
        vel.z *= std::max(0.0f, 1.0f - h_drag * dt);
        vel.y *= std::max(0.0f, 1.0f - v_drag * dt);

        float max_sink_speed = (fluid == FluidType::WATER) ? -3.0f : -1.0f;
        if (vel.y < max_sink_speed) vel.y = max_sink_speed;
    } else {
        vel.y += gravity.y * dt;
    }

    pos.y += vel.y * dt;
    hitbox.grounded = false;

    glm::vec3 center_pos = pos + glm::vec3(0.0f, half.y, 0.0f);

    if (vel.y <= 0.0f) {
        if (check_collision(center_pos)) {
            int foot_y = static_cast<int>(std::floor(pos.y));
            pos.y = static_cast<float>(foot_y + 1);
            vel.y = 0.0f;
            hitbox.grounded = true;
        }
    } else {
        if (check_collision(center_pos)) {
            int head_y = static_cast<int>(std::floor(pos.y + half.y * 2.0f));
            pos.y = static_cast<float>(head_y) - half.y * 2.0f - E;
            vel.y = 0.0f;
        }
    }

    hitbox.position = pos + glm::vec3(0.0f, half.y, 0.0f);

    float target_x = pos.x + vel.x * dt;
    glm::vec3 test_pos_x = pos;
    test_pos_x.x = target_x;

    if (check_collision(test_pos_x + glm::vec3(0.0f, half.y, 0.0f))) {
        bool stepped = false;
        if (hitbox.grounded && fluid == FluidType::NONE) {
            glm::vec3 step_pos = test_pos_x + glm::vec3(0.0f, half.y + STEP_HEIGHT, 0.0f);

            if (!check_collision(step_pos)) {
                pos.x = target_x;
                pos.y += STEP_HEIGHT;
                stepped = true;
            }
        }

        if (!stepped) {
            if (vel.x < 0.0f) {
                auto x = static_cast<int>(std::floor(target_x - half.x + E));
                pos.x = static_cast<float>(x + 1) + half.x + E;
            } else if (vel.x > 0.0f) {
                auto x = static_cast<int>(std::floor(target_x + half.x - E));
                pos.x = static_cast<float>(x) - half.x - E;
            }
            vel.x = 0.0f;
        }
    } else {
        pos.x = target_x;
    }

    hitbox.position = pos + glm::vec3(0.0f, half.y, 0.0f);

    float target_z = pos.z + vel.z * dt;
    glm::vec3 test_pos_z = pos;
    test_pos_z.z = target_z;

    if (check_collision(test_pos_z + glm::vec3(0.0f, half.y, 0.0f))) {
        bool stepped = false;
        if (hitbox.grounded && fluid == FluidType::NONE) {
            glm::vec3 step_pos = test_pos_z + glm::vec3(0.0f, half.y + STEP_HEIGHT, 0.0f);

            if (!check_collision(step_pos)) {
                pos.z = target_z;
                pos.y += STEP_HEIGHT;
                stepped = true;
            }
        }

        if (!stepped) {
            if (vel.z < 0.0f) {
                auto z = static_cast<int>(std::floor(target_z - half.z + E));
                pos.z = static_cast<float>(z + 1) + half.z + E;
            } else if (vel.z > 0.0f) {
                auto z = static_cast<int>(std::floor(target_z + half.z - E));
                pos.z = static_cast<float>(z) - half.z - E;
            }
            vel.z = 0.0f;
        }
    } else {
        pos.z = target_z;
    }

    hitbox.position = pos + glm::vec3(0.0f, half.y, 0.0f);

    if (shifting && hitbox.grounded && fluid == FluidType::NONE) {
        int check_y = static_cast<int>(std::floor(pos.y - E));

        bool ground_below_z = false;
        for (auto x = static_cast<int>(std::floor(prev_x - half.x + E)); x <= static_cast<int>(std::floor(prev_x + half.x - E)); x++) {
            for (auto z = static_cast<int>(std::floor(pos.z - half.z + E)); z <= static_cast<int>(std::floor(pos.z + half.z - E)); z++) {
                if (level->is_obstacle(x, check_y, z)) {
                    ground_below_z = true;
                    break;
                }
            }
        }
        if (!ground_below_z) {
            pos.z = prev_z;
            vel.z = 0.0f;
        }

        bool ground_below_x = false;
        for (auto x = static_cast<int>(std::floor(pos.x - half.x + E)); x <= static_cast<int>(std::floor(pos.x + half.x - E)); x++) {
            for (auto z = static_cast<int>(std::floor(prev_z - half.z + E)); z <= static_cast<int>(std::floor(prev_z + half.z - E)); z++) {
                if (level->is_obstacle(x, check_y, z)) {
                    ground_below_x = true;
                    break;
                }
            }
        }
        if (!ground_below_x) {
            pos.x = prev_x;
            vel.x = 0.0f;
        }
    }

    hitbox.position = pos + glm::vec3(0.0f, half.y, 0.0f);
}