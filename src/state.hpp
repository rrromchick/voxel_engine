#pragma once

#include "gfx/window.hpp"
#include "gfx/shader.hpp"
#include "block/block_atlas.hpp"
#include "world/world.hpp"

struct State {
	std::unique_ptr<Window> window;
	std::unique_ptr<World> world;
	std::unique_ptr<Shader> shader;
	std::unique_ptr<BlockAtlas> block_atlas;

	bool wireframe;

	State() = default;
	~State() = default;

	State(const State &other) = delete;
	State &operator=(const State &other) = delete;

	State(State &&other) = delete;
	State &operator=(State &&other) = delete;

	/*State(State &&other)
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
	}*/

	inline Window *get_wnd() const {
		return this->window.get();
	}

	inline Shader *get_shader() const {
		return this->shader.get();
	}

	inline World *get_world() {
		return this->world.get();
	}

	inline BlockAtlas *get_atlas() {
		return this->block_atlas.get();
	}

	inline bool is_wireframe() const {
		return this->wireframe;
	}
};