#pragma once

#include "ECS.hpp"
#include "VoxelModel.hpp"
#include "WorldGenerator.hpp"
#include <glm/glm.hpp>
#include <memory>

struct TransformComponent : public ECS::Component<TransformComponent> {
    glm::vec3 position { 0.0f };
    glm::vec3 rotation { 0.0f };
    glm::vec3 scale { 1.0f };
    
    glm::vec3 target_position { 0.0f };
    glm::vec3 target_rotation { 0.0f };

    TransformComponent() = default;

    TransformComponent(glm::vec3 position)
        : position(position) {}
};

struct NetworkComponent : public ECS::Component<NetworkComponent> {
    uint32_t network_id = 0;
    uint32_t owner_client_id = 0;
    uint64_t last_sequence = 0;

    NetworkComponent() = default;

    NetworkComponent(uint32_t net_id, uint32_t client_id)
        : network_id(net_id), owner_client_id(client_id) {}
};

struct PlayerInputComponent : public ECS::Component<PlayerInputComponent> {
    bool move_forward = false;
    bool move_backward = false;
    bool move_left = false;
    bool move_right = false;
    bool jump = false;
    bool sprinting = false;
    bool shifting = false;
    bool is_swimming_up = false;

    float yaw = 0.0f;
    float pitch = 0.0f;
};

struct HitboxComponent : public ECS::Component<HitboxComponent> {
    glm::vec3 position { 0.0f };
    glm::vec3 velocity { 0.0f };
    glm::vec3 halfsize { 0.3f, 0.9f, 0.3f };
    bool grounded = false;

    HitboxComponent() = default;

    HitboxComponent(glm::vec3 size, glm::vec3 pos)
        : position(pos), halfsize(size), velocity(0.0f), grounded(false) {}
};

struct RenderComponent : public ECS::Component<RenderComponent> {
    std::shared_ptr<VoxelModel> model = nullptr;
    bool visible = true;

    RenderComponent() = default;

    explicit RenderComponent(std::shared_ptr<VoxelModel> model)
        : model(std::move(model)) {}
};

struct LocalPlayerComponent : public ECS::Component<LocalPlayerComponent> {};

constexpr auto INVENTORY_SIZE = 36;
constexpr auto HOTBAR_SIZE = 9;

struct ItemStack {
    uint8_t id;
    uint8_t count;
};

struct InventoryComponent : public ECS::Component<InventoryComponent> {
    std::array<ItemStack, INVENTORY_SIZE> slots {};
    uint8_t selected_slot = 0;

    InventoryComponent() = default;

    [[nodiscard]] ItemStack &active_item() {
        return slots[selected_slot % HOTBAR_SIZE];
    }

    [[nodiscard]] const ItemStack &active_item() const {
        return slots[selected_slot % HOTBAR_SIZE];
    }

    bool consume_active_item() {
        auto &active = active_item();
        if (active.id != 0 && active.count > 0) {
            active.count--;
            if (active.count == 0) {
                active.id = 0;
            }
            return true;
        }
        return false;
    }

    bool add_item(uint8_t block_id, uint8_t amount = 1) {
        if (block_id == 0 || amount == 0) return false;

        for (auto &slot : slots) {
            if (slot.id == block_id && slot.count < 64) {
                uint8_t space = 64 - slot.count;
                uint8_t to_add = std::min(amount, space);
                slot.count += to_add;
                amount -= to_add;

                if (amount == 0) return true;
            }
        }

        for (auto &slot : slots) {
            if (slot.id == 0 || slot.count == 0) {
                slot.id = block_id;
                uint8_t to_add = std::min<uint8_t>(amount, 64);
                slot.count = to_add;
                amount -= to_add;

                if (amount == 0) return true;
            }
        }

        return amount == 0;
    }

    void swap_slots(std::size_t slot_a, std::size_t slot_b) {
        if (slot_a < INVENTORY_SIZE && slot_b < INVENTORY_SIZE) {
            std::swap(slots[slot_a], slots[slot_b]);
        }
    }
};

inline void register_shared_components(ECS &ecs) {
    ecs.register_type<TransformComponent>();
    ecs.register_type<NetworkComponent>();
    ecs.register_type<InventoryComponent>();
}

inline void register_server_components(ECS &ecs) {
    register_shared_components(ecs);
    ecs.register_type<PlayerInputComponent>();
    ecs.register_type<HitboxComponent>();
}

inline void register_client_components(ECS &ecs) {
    register_shared_components(ecs);
    ecs.register_type<RenderComponent>();
    ecs.register_type<LocalPlayerComponent>();
    ecs.register_type<PlayerInputComponent>();
    ecs.register_type<HitboxComponent>();
}