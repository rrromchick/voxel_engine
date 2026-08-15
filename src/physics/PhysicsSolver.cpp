#include "PhysicsSolver.hpp"
#include "Hitbox.hpp"
#include "Chunks.hpp"
#include "Global.hpp"
#include <algorithm>
#include <cmath>

constexpr auto E = 0.001f;
constexpr float STEP_HEIGHT = 0.5f;

PhysicsSolver::PhysicsSolver(glm::vec3 gravity) : gravity(gravity) {}

FluidType PhysicsSolver::check_fluid(Chunks *chunks, Hitbox *hitbox) {
    auto &pos = hitbox->position;
    auto &half = hitbox->halfsize;

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
                auto *vox = chunks->get(x, y, z);
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

void PhysicsSolver::step(Hitbox *hitbox, float delta, unsigned int substeps, bool shifting, bool is_swimming_up) {
    if (substeps == 0) return;

    float dt = delta / static_cast<float>(substeps);
    auto *chunks = global.chunks.get();

    for (unsigned int i = 0; i < substeps; i++) {
        auto &pos = hitbox->position;
        auto &half = hitbox->halfsize;
        auto &vel = hitbox->velocity;

        auto prev_x = pos.x;
        auto prev_z = pos.z;

        FluidType fluid = check_fluid(chunks, hitbox);

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
                        if (chunks->is_obstacle(x, y, z)) return true;
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
                auto head_y = static_cast<int>(std::floor(pos.y + half.y + 0.1f));
                auto head_z = static_cast<int>(std::floor(pos.z));

                auto *block_above = chunks->get(head_x, head_y, head_z);
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
        hitbox->grounded = false;

        auto min_x = static_cast<int>(std::floor(pos.x - half.x + E));
        auto max_x = static_cast<int>(std::floor(pos.x + half.x - E));
        auto min_z = static_cast<int>(std::floor(pos.z - half.z + E));
        auto max_z = static_cast<int>(std::floor(pos.z + half.z - E));

        if (vel.y <= 0.0f) {
            auto min_y = static_cast<int>(std::floor(pos.y - half.y));
            auto max_y = static_cast<int>(std::floor(pos.y + half.y - E));

            for (int y = max_y; y >= min_y; y--) {
                bool hit = false;
                for (int x = min_x; x <= max_x; x++) {
                    for (int z = min_z; z <= max_z; z++) {
                        if (chunks->is_obstacle(x, y, z)) {
                            pos.y = static_cast<float>(y + 1) + half.y;
                            vel.y = 0.0f;
                            hitbox->grounded = true;

                            if (fluid == FluidType::NONE) {
                                constexpr float friction = 18.0f;
                                vel.x *= std::max(0.0f, 1.0f - dt * friction);
                                vel.z *= std::max(0.0f, 1.0f - dt * friction);
                            }
                            hit = true;
                            break;
                        }
                    }
                    if (hit) break;
                }
                if (hit) break;
            }
        } else {
            auto min_y = static_cast<int>(std::floor(pos.y - half.y + E));
            auto max_y = static_cast<int>(std::floor(pos.y + half.y - E));

            for (int y = min_y; y <= max_y; y++) {
                bool hit = false;
                for (int x = min_x; x <= max_x; x++) {
                    for (int z = min_z; z <= max_z; z++) {
                        if (chunks->is_obstacle(x, y, z)) {
                            pos.y = static_cast<float>(y) - half.y - E;
                            vel.y = 0.0f;
                            hit = true;
                            break;
                        }
                    }
                    if (hit) break;
                }
                if (hit) break;
            }
        }

        vel.x += gravity.x * dt;
        float target_x = pos.x + vel.x * dt;
        glm::vec3 test_pos_x = pos;
        test_pos_x.x = target_x;

        if (check_collision(test_pos_x)) {
            bool stepped = false;
            if (hitbox->grounded && fluid == FluidType::NONE) {
                glm::vec3 step_pos = test_pos_x;
                step_pos.y += STEP_HEIGHT;

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

        vel.z += gravity.z * dt;
        float target_z = pos.z + vel.z * dt;
        glm::vec3 test_pos_z = pos;
        test_pos_z.z = target_z;

        if (check_collision(test_pos_z)) {
            bool stepped = false;
            if (hitbox->grounded && fluid == FluidType::NONE) {
                glm::vec3 step_pos = test_pos_z;
                step_pos.y += STEP_HEIGHT;

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

        if (shifting && hitbox->grounded && fluid == FluidType::NONE) {
            int check_y = static_cast<int>(std::floor(pos.y - half.y - 0.5f));

            bool ground_below_z = false;
            for (auto x = static_cast<int>(std::floor(prev_x - half.x + E)); x <= static_cast<int>(std::floor(prev_x + half.x - E)); x++) {
                for (auto z = static_cast<int>(std::floor(pos.z - half.z + E)); z <= static_cast<int>(std::floor(pos.z + half.z - E)); z++) {
                    if (chunks->is_obstacle(x, check_y, z)) {
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
                    if (chunks->is_obstacle(x, check_y, z)) {
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
    }
}

bool PhysicsSolver::is_block_inside(int x, int y, int z, Hitbox *hitbox) {
    auto &pos = hitbox->position;
    auto &half = hitbox->halfsize;

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