#pragma once

#include "gfx/shader.hpp"
#include "world/world.hpp"
#include "gfx/window.hpp"
#include "block/block_atlas.hpp"

struct State {
	State() = default;
	~State() = default;

	State(const State &other) = delete;
	State &operator=(const State &other) = delete;

	State(State &&other)
		: window(std::move(other.window)),
		shader(std::move(other.shader)),
		block_atlas(std::move(other.block_atlas)),
		world(std::move(other.world)), wireframe(other.wireframe) {}

	State &operator=(State &&other) {
		assert(this != &other);

		this->window = std::move(other.window);
		this->shader = std::move(other.shader);
		this->block_atlas = std::move(other.block_atlas);
		this->world = std::move(other.world);
		this->wireframe = other.wireframe;

		other.wireframe = false;
		return *this;
	}

	inline Window *get_wnd() const {
		return this->window.get();
	}

	inline const Shader &get_shader() const {
		return this->shader;
	}

	inline World *get_world() const {
		return this->world.get();
	}

	inline const BlockAtlas &get_atlas() const {
		return this->block_atlas;
	}

	inline bool is_wireframe() const {
		return this->wireframe;
	}

private:
	std::unique_ptr<Window> window;
	std::unique_ptr<World> world;
	Shader shader;
	BlockAtlas block_atlas;

	bool wireframe;
};

State state;