#include "PhysicsSolver.hpp"
#include "Hitbox.hpp"
#include "Chunks.hpp"
#include "Global.hpp"
#include <algorithm>
#include <cmath>

constexpr auto E = 0.001f;

PhysicsSolver::PhysicsSolver(glm::vec3 gravity) : gravity(gravity) {}

FluidType PhysicsSolver::check_fluid(Chunks *chunks, Hitbox *hitbox) {
    auto &pos = hitbox->position;
    auto &half = hitbox->halfsize;

    auto min_x = std::floor<int>(pos.x - half.x + E);
    auto max_x = std::floor<int>(pos.x + half.x - E);
    auto min_y = std::floor<int>(pos.y - half.y + E);
    auto max_y = std::floor<int>(pos.y + half.y - E);
    auto min_z = std::floor<int>(pos.z - half.z + E);
    auto max_z = std::floor<int>(pos.z + half.z - E);

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

        if (fluid != FluidType::NONE) {
            float gravity_scale = (fluid == FluidType::WATER) ? 0.2f : 0.05f;
            float drag = (fluid == FluidType::WATER) ? 6.0f : 16.0f;

            vel.y += gravity.y * gravity_scale * dt;

            if (is_swimming_up) {
                auto head_x = std::floor<int>(pos.x);
                auto head_y = std::floor<int>(pos.y + half.y + 0.1f);
                auto head_z = std::floor<int>(pos.z);

                auto *block_above = chunks->get(head_x, head_y, head_z);
                bool near_surface = (!block_above || block_above->id == BlockId::AIR);

                float swim_impulse = near_surface ? 18.0f : ((fluid == FluidType::WATER) ? 12.0f : 6.0f);
                vel.y += swim_impulse * dt;
            }

            auto drag_factor = std::max<float>(0.0f, 1.0f - drag * dt);
            vel.x *= drag_factor;
            vel.y *= drag_factor;
            vel.z *= drag_factor;
            
            float max_sink_speed = (fluid == FluidType::WATER) ? -2.5f : -0.8f;
            if (vel.y < max_sink_speed) {
                vel.y = max_sink_speed;
            }
        } else {
            vel.y += gravity.y * dt;
        }

        pos.y += vel.y * dt;

        hitbox->grounded = false;

        int ground_y = std::floor<int>(pos.y - half.y - E);
        for (int x = std::floor<int>(pos.x - half.x + E); x <= std::floor<int>(pos.x + half.x - E); x++) {
            for (int z = std::floor<int>(pos.z - half.z + E); z <= std::floor<int>(pos.z + half.z - E); z++) {
                if (chunks->is_obstacle(x, ground_y, z)) {
                    hitbox->grounded = true;
                    break;
                }
            }
            if (hitbox->grounded) break;
        }

        if (vel.y <= 0.0f) {
            int y = std::floor<int>(pos.y - half.y);
            for (int x = std::floor<int>(pos.x - half.x + E); x <= std::floor<int>(pos.x + half.x - E); x++) {
                for (int z = std::floor<int>(pos.z - half.z + E); z <= std::floor<int>(pos.z + half.z - E); z++) {
                    if (chunks->is_obstacle(x, y, z)) {
                        pos.y = y + 1.0f + half.y;
                        vel.y = 0.0f;
                        hitbox->grounded = true;

                        constexpr float friction = 18.0f;
                        vel.x *= std::max(0.0f, 1.0f - dt * friction);
                        vel.z *= std::max(0.0f, 1.0f - dt * friction);
                        break;
                    }
                }
                if (hitbox->grounded) break;
            }
        } else if (vel.y > 0.0f) {
            auto y = std::floor<int>(pos.y + half.y - E);
            for (int x = std::floor<int>(pos.x - half.x + E); x <= std::floor<int>(pos.x + half.x - E); x++) {
                for (int z = std::floor<int>(pos.z - half.z + E); z <= std::floor<int>(pos.z + half.z - E); z++) {
                    if (chunks->is_obstacle(x, y, z)) {
                        pos.y = y - half.y - E;
                        vel.y = 0.0f;
                        break;
                    }
                }
            }
        }

        vel.x += gravity.x * dt;
        pos.x += vel.x * dt;

        if (vel.x < 0.0f) {
            int x = std::floor(pos.x - half.x);
            for (int y = std::floor(pos.y - half.y + E); y <= std::floor(pos.y + half.y - E); y++) {
                for (int z = std::floor(pos.z - half.z + E); z <= std::floor(pos.z + half.z - E); z++) {
                    if (chunks->is_obstacle(x, y, z)) {
                        pos.x = x + 1.0f + half.x + E;
                        vel.x = 0.0f;
                        break;
                    }
                }
            }
        } else if (vel.x > 0.0f) {
            int x = std::floor(pos.x + half.x);
            for (int y = std::floor(pos.y - half.y + E); y <= std::floor(pos.y + half.y - E); y++) {
                for (int z = std::floor(pos.z - half.z + E); z <= std::floor(pos.z + half.z - E); z++) {
                    if (chunks->is_obstacle(x, y, z)) {
                        pos.x = x - half.x - E;
                        vel.x = 0.0f;
                        break;
                    }
                }
            }
        }

        vel.z += gravity.z * dt;
        pos.z += vel.z * dt;

        if (vel.z < 0.0f) {
            int z = std::floor(pos.z - half.z);
            for (int y = std::floor(pos.y - half.y + E); y <= std::floor(pos.y + half.y - E); y++) {
                for (int x = std::floor(pos.x - half.x + E); x <= std::floor(pos.x + half.x - E); x++) {
                    if (chunks->is_obstacle(x, y, z)) {
                        pos.z = z + 1.0f + half.z + E;
                        vel.z = 0.0f;
                        break;
                    }
                }
            }
        } else if (vel.z > 0.0f) {
            int z = std::floor(pos.z + half.z);
            for (int y = std::floor(pos.y - half.y + E); y <= std::floor(pos.y + half.y - E); y++) {
                for (int x = std::floor(pos.x - half.x + E); x <= std::floor(pos.x + half.x - E); x++) {
                    if (chunks->is_obstacle(x, y, z)) {
                        pos.z = z - half.z - E;
                        vel.z = 0.0f;
                        break;
                    }
                }
            }
        }

        if (shifting && hitbox->grounded && fluid == FluidType::NONE) {
            int check_y = std::floor(pos.y - half.y - 0.5f);

            bool ground_below_z = false;
            for (int x = std::floor(prev_x - half.x + E); x <= std::floor(prev_x + half.x - E); x++) {
                for (int z = std::floor(prev_z - half.z + E); z <= std::floor(prev_z + half.z - E); z++) {
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
            for (int x = std::floor(pos.x - half.x + E); x <= std::floor(pos.x + half.x - E); x++) {
                for (int z = std::floor(prev_z - half.z + E); z <= std::floor(prev_z + half.z - E); z++) {
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
    return x >= std::floor(pos.x - half.x) && x <= std::floor(pos.x + half.x) &&
        z >= std::floor(pos.z - half.z) && z <= std::floor(pos.z + half.z) &&
        y >= std::floor(pos.y - half.y) && y <= std::floor(pos.y + half.y);
}