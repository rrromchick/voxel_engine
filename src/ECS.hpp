#pragma once

#include "ComponentManager.hpp"

struct ObjBase {};
struct CompBase {};
struct CompTypeBase {};

using EntityId = uint32_t;

struct ECS : public ComponentManager<
    ECS,
    EntityId,
    ObjBase,
    CompBase,
    CompTypeBase,
    64,
    10000,
    ComponentManagerType::ENTITY
> {    
    std::unique_ptr<Level> level;

    bool full() const override {
        return size >= 10000;
    }

    std::optional<uint64_t> new_object_id() override {
        for (std::size_t i = 0; i < size; i++) {
            bool empty = true;
            for (bool b : signatures[i]) {
                if (b) {
                    empty = false;
                    break;
                }
            }
            if (empty) {
                return static_cast<uint64_t>(i);
            }
        }

        if (size < 10000) {
            uint64_t new_id = size;
            resize(size + 1);
            return new_id;
        }

        return std::nullopt;
    }
};