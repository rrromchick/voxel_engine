#pragma once

#include "ECS.hpp"
#include "VoxelModel.hpp"
#include <glm/glm.hpp>
#include <memory>

struct TransformComponent : public ECS::Component<TransformComponent> {
    glm::vec3 position { 0.0f };
    glm::vec3 rotation { 0.0f };
    glm::vec3 scale { 0.0f };
    
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

inline void register_shared_components(ECS &ecs) {
    ecs.register_type<TransformComponent>();
    ecs.register_type<NetworkComponent>();
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