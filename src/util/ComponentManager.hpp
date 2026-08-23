#pragma once

#include <vector>
#include <array>
#include <memory>
#include <string>
#include <optional>
#include <typeinfo>
#include <cstring>
#include <cstdint>
#include <concepts>
#include <utility>
#include <algorithm>
#include <new>
#include <cassert>

struct Level;

enum class ComponentManagerType {
    ENTITY,
    TILE,
    ITEM
};

template <
    typename T,
    typename I,
    typename ObjBase,
    typename CompBase,
    typename CompTypeBase,  
    std::size_t MAX_COMPONENTS,
    std::size_t MAX_OBJECTS,
    ComponentManagerType Type>
struct ComponentManager {
    struct BaseComponent : public CompBase {
        uint8_t block = 0;

        BaseComponent() = default;
        BaseComponent(const BaseComponent &other) = delete;
        BaseComponent(BaseComponent &&other) = default;
        BaseComponent &operator=(const BaseComponent &other) = delete;
        BaseComponent &operator=(BaseComponent &&other) = default;
        virtual ~BaseComponent() = default;
        
        virtual void init() {};
    };

    struct ComponentType : public CompTypeBase {
        uint64_t id = 0;
        std::size_t size = 0;
        std::size_t alignment = 16;
        bool registered = false;

        template <typename C>
        static inline ComponentType from(ComponentManager *system, uint64_t id) {
            ComponentType result;
            result.id = id;
            result.registered = true;

            if constexpr (requires { C::size(); }) {
                result.size = C::size();
            } else {
                const std::size_t align = alignof(C);
                result.size = (sizeof(C) + align - 1) & ~(align - 1);
            }
            result.alignment = std::max<std::size_t>(alignof(C), 16);

            return result;
        }
    };

    struct Object : public ObjBase {
        I id = 0;
        T *p = nullptr;

        Object() = default;
        Object(T *p, I id) : id(id), p(p) {}

        inline operator I() const { return id; }
        inline bool operator==(const Object &rhs) const { return id == rhs.id; }
        inline auto operator<=>(const Object &rhs) const { return id <=> rhs.id; }

        inline T &parent() const { return *p; }

        template <typename C>
        inline C &get() const { 
            return p->template component<C>(*this);
        }

        template <typename C>
        inline C *opt() const {
            return p->template opt_component<C>(*this);
        }

        template <typename C>
        inline bool has() const {
            return p->template has_component<C>(*this);
        }

        template <typename C>
        inline C &add(C &&component = C()) const {
            static_assert(std::is_base_of_v<BaseComponent, C>, "Component must inherit from BaseComponent");
            return p->add_component(*this, std::forward<C>(component));
        }

        template <typename C>
        inline void remove() const {
            p->template remove_component<C>(*this);
        }

        template <typename _ = T>
            requires (Type == ComponentManagerType::ENTITY)
        inline void destroy() const {
            p->enqueue_destroy(*this);
        }

        template <typename _ = T>
            requires (Type != ComponentManagerType::ENTITY)
        inline void destroy() const {
            p->destroy(*this);
        }

        template <typename _ = T>
            requires (Type == ComponentManagerType::ENTITY)
        inline auto &level() const {
            return p->level;
        }

        inline auto &signature() const {
            return p->signatures[static_cast<std::size_t>(this->id)];
        }

        inline std::string to_string() const {
            std::string prefix;
            if constexpr (Type == ComponentManagerType::ENTITY) prefix = "Entity";
            else if constexpr (Type == ComponentManagerType::ITEM) prefix = "Item";
            else if constexpr (Type == ComponentManagerType::TILE) prefix = "Tile";
            return prefix + "(" + std::to_string(id) + ")";
        }
    };

    template <typename C>
    struct Component : public BaseComponent {
        inline static uint64_t _id = static_cast<uint64_t>(-1);
        inline static T *_p = nullptr;
        
        inline Object object() const {
            return parent().template object_from_component<C>(
                reinterpret_cast<const void*>(this));
        }

        inline T &parent() const {
            return *Component<C>::_p;
        }

        template <typename _ = T>
            requires (Type == ComponentManagerType::ENTITY)
        inline Object entity() const { return object(); }

        template <typename _ = T>
            requires (Type == ComponentManagerType::ENTITY)
        inline auto &level() const { return *parent().level; }

        template <typename _ = T>
            requires (Type == ComponentManagerType::TILE) 
        inline Object tile() const { return object(); }

        template <typename _ = T>
            requires (Type == ComponentManagerType::ITEM)
        inline Object item() const { return object(); }
    };

    struct ComponentArray {
        static constexpr auto BLOCK_SIZE = 512;
        static constexpr auto NUM_BLOCKS = (MAX_OBJECTS / BLOCK_SIZE) + 1;

        struct AlignedDeleter {
            std::size_t align = 16;
            void operator()(uint8_t* ptr) const noexcept {
                ::operator delete[](ptr, std::align_val_t(align));
            }
        };

        struct Block {
            const ComponentArray *parent = nullptr;
            std::unique_ptr<uint8_t[], AlignedDeleter> data = nullptr;

            Block() = default;
            Block(const Block&) = delete;
            Block& operator=(const Block&) = delete;
            Block(Block&&) noexcept = default;
            Block& operator=(Block&&) noexcept = default;

            Block(const ComponentArray *parent, std::size_t n)
                : parent(parent) {
                const auto data_size = BLOCK_SIZE * parent->type->size;
                const auto alignment = parent->type->alignment;

                auto* raw_ptr = static_cast<uint8_t*>(
                    ::operator new[](data_size, std::align_val_t(alignment))
                );

                data = std::unique_ptr<uint8_t[], AlignedDeleter>(raw_ptr, AlignedDeleter{ alignment });
            }

            inline BaseComponent *operator[](std::size_t i) const {
                return reinterpret_cast<BaseComponent*>(
                    data.get() + (parent->type->size * i));
            }

            inline std::size_t to_index(const void *p) const {
                const auto offset = static_cast<std::size_t>(
                    reinterpret_cast<const uint8_t*>(p) - data.get());
                return offset / parent->type->size;
            }
        };

        ComponentType *type = nullptr;
        std::array<Block, NUM_BLOCKS> blocks;
        std::size_t n_blocks = 0;

        ComponentArray() = default;
        ComponentArray(ComponentType *type) : type(type) {}

        void resize(std::size_t n) {
            if (!type || !type->registered) return;
            while (n > (n_blocks * BLOCK_SIZE)) {
                blocks[n_blocks] = Block(this, n_blocks);
                n_blocks++;
            }
        }

        inline uint8_t block(std::size_t i) const {
            return static_cast<uint8_t>(i / BLOCK_SIZE);
        }

        inline BaseComponent *operator[](std::size_t i) const {
            return blocks[i / BLOCK_SIZE][i % BLOCK_SIZE];
        }

        inline std::size_t to_index(const void *p) const {
            const auto *c = reinterpret_cast<const BaseComponent*>(p);
            return std::size_t(c->block * BLOCK_SIZE) + blocks[c->block].to_index(p);
        }
    };

    std::size_t n_components = 0;
    std::size_t size = 0;

    std::vector<std::vector<bool>> signatures;
    std::array<ComponentArray, MAX_COMPONENTS> components;
    std::array<ComponentType, MAX_COMPONENTS> component_types;

    ComponentManager() { resize(256); }
    virtual ~ComponentManager() = default;

    ComponentManager(const ComponentManager &other) = delete;
    ComponentManager(ComponentManager &&other) = default;
    ComponentManager &operator=(const ComponentManager &other) = delete;
    ComponentManager &operator=(ComponentManager &&other) = default;

    template <typename V>
    void register_type() {
        assert(n_components < MAX_COMPONENTS && "Exceeded MAX_COMPONENTS count");
        const uint64_t id = n_components++;
        Component<V>::_id = id;

        V::_p = static_cast<T*>(this);
        component_types[id] = ComponentType::template from<V>(this, id);
        components[id] = ComponentArray(&component_types[id]);
        components[id].resize(size);
    }

    template <typename C>
    Object object_from_component(const void *c) {
        assert(C::_id < MAX_COMPONENTS && "Component type not registered!");
        return Object {
            reinterpret_cast<T*>(this),
            static_cast<I>(components[C::_id].to_index(c))
        };
    }

    std::optional<Object> create(std::optional<uint64_t> preset_id = std::nullopt) {
        const auto id = preset_id ? *preset_id : new_object_id().value_or(size);

        if (id >= size) {
            resize(std::max<std::size_t>(id + 1, size * 2));
        }

        const auto obj = Object { reinterpret_cast<T*>(this), static_cast<I>(id) };
        std::fill(signatures[obj.id].begin(), signatures[obj.id].end(), false);
        on_create(obj);
        return obj;
    }

    void destroy(Object object) {
        on_destroy(object);
        auto &signature = signatures[object.id];

        for (std::size_t i = 0; i < MAX_COMPONENTS; i++) {
            if (signature[i]) {
                const auto &type = component_types[i];
                auto *base = components[type.id][object.id];
                on_component_destroy(type.id, reinterpret_cast<void*>(base), object);
                base->~BaseComponent();
            }
            signature[i] = false;
        }
    }

    template <typename C>
    inline C &add_component(Object object, C &&component) {
        assert(C::_id < MAX_COMPONENTS && "Attempted to add unregistered component!");
        auto &type = component_types[C::_id];
        auto &signature = signatures[object.id];

        signature[type.id] = true;
        const auto &arr = components[type.id];
        auto *p = arr[object.id];
        C *c = new (p) C(std::forward<C>(component));
        c->block = arr.block(object.id);
        c->init();
        on_component_create(type.id, p, object);
        return *c;
    }

    template <typename C>
    inline void remove_component(Object object) {
        assert(C::_id < MAX_COMPONENTS && "Attempted to remove unregistered component!");
        const auto &type = component_types[C::_id];
        auto *base = components[type.id][object.id];
        auto *c = static_cast<C*>(base);
        this->on_component_destroy(type.id, reinterpret_cast<void*>(c), object);
        base->~BaseComponent();
        signatures[object.id][type.id] = false;
    }

    template <typename C>
    inline bool has_component(Object object) const {
        if (C::_id >= MAX_COMPONENTS) return false;
        return signatures[object.id][C::_id];
    }

    template <typename C>
    inline C &component(Object object) {
        assert(C::_id < MAX_COMPONENTS && "Attempted to fetch unregistered component!");
        void *p = components[C::_id][object.id];
        return *static_cast<C*>(p);
    }

    template <typename C>
    inline C *opt_component(Object object) {
        if (C::_id >= MAX_COMPONENTS) return nullptr;
        const auto &signature = signatures[object.id];
        const auto id = C::_id;
        return signature[id] ? static_cast<C*>(components[id][object.id]) : nullptr;
    }

    virtual bool full() const = 0;
    virtual std::optional<uint64_t> new_object_id() = 0;
    virtual void on_create(Object object) {}
    virtual void on_destroy(Object object) {}
    virtual void on_component_create(uint64_t id, void *component, Object object) {}
    virtual void on_component_destroy(uint64_t id, void *component, Object object) {}

private:
    void resize(std::size_t new_size) {
        for (std::size_t i = 0; i < n_components; i++) {
            components[i].resize(new_size);
        }
        signatures.resize(new_size, std::vector<bool>(MAX_COMPONENTS, false));
        size = new_size;
    }
};