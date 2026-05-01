#include "player.hpp"
#include "world/world.hpp"
#include "state.hpp"
#include "util/math.hpp"

EntityPlayer::EntityPlayer(World *world, Window *wnd)
	: world(world), camera(wnd, glm::radians(75.0f)) {}

void EntityPlayer::update() {
	auto state = world->get_state();

	constexpr f32 mouse_sensitivity = 3.2f;
	this->camera.update();
	this->camera.pitch -=
		state->get_wnd()->get_mouse().delta.y / state->get_wnd()->frame_delta
		/ (mouse_sensitivity * 10000.0f);
	this->camera.yaw -=
		state->get_wnd()->get_mouse().delta.x / state->get_wnd()->frame_delta
		/ (mouse_sensitivity * 10000.0f);

	glm::ivec3 block_pos = this->world->pos_to_block(this->camera.position);
	glm::ivec3 offset = this->world->pos_to_offset(block_pos);

	if (block_pos != this->block_pos) {
		this->block_pos = block_pos;
		this->block_pos_changed = true;
	} else {
		this->block_pos_changed = false;
	}

	if (offset != this->offset) {
		this->offset = offset;
		this->offset_changed = true;
	} else {
		this->offset_changed = false;
	}

	for (usize i = 0; i < 10; i++) {
		if (state->get_wnd()->get_keyboard().keys[GLFW_KEY_0 + i].down) {
			this->selected_block = blocks[(BlockId)i].id;
		}
	}
}

void EntityPlayer::tick() {
	auto state = world->get_state();

	auto raycast_block_fn = [&](glm::ivec3 v) {
		return state->get_world()->get_data(v) != 0;
	};
	
	constexpr f32 speed = 0.22f;
	glm::vec3 movement, direction, forward, right;
	forward = glm::vec3(glm::sin(this->camera.yaw), 0, glm::cos(this->camera.yaw));
	right = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), forward);

	if (state->get_wnd()->get_keyboard().keys[GLFW_KEY_W].down) {
		direction += forward;
	}

	if (state->get_wnd()->get_keyboard().keys[GLFW_KEY_S].down) {
		direction -= forward;
	}

	if (state->get_wnd()->get_keyboard().keys[GLFW_KEY_A].down) {
		direction += right;
	}

	if (state->get_wnd()->get_keyboard().keys[GLFW_KEY_D].down) {
		direction -= right;
	}

	if (state->get_wnd()->get_keyboard().keys[GLFW_KEY_SPACE].down) {
		direction += glm::vec3(0.0f, 1.0f, 0.0f);
	}

	if (state->get_wnd()->get_keyboard().keys[GLFW_KEY_LEFT_SHIFT].down) {
		direction -= glm::vec3(0.0f, 1.0f, 0.0f);
	}

	if (glm::isnan(glm::length(direction))) {
		movement = glm::vec3(0.0f);
	} else {
		movement = direction;
		glm::normalize(movement);
		movement *= speed;
	}

	this->camera.position = camera.position + movement;

	constexpr f32 reach = 6.0f;
	this->has_look_block = math::ray_block(
		math::Ray(this->camera.position, this->camera.direction),
		reach, raycast_block_fn, &this->look_block, &this->look_face);

	if (this->has_look_block) {
		if (state->get_wnd()->get_mouse().buttons[GLFW_MOUSE_BUTTON_LEFT].pressed_tick) {
			this->world->set_data(this->look_block, 0);
		} 

		if (state->get_wnd()->get_mouse().buttons[GLFW_MOUSE_BUTTON_RIGHT].pressed_tick) {
			this->world->set_data(
				this->look_block + direction::dir_to_vec3(this->look_face), 
				this->selected_block);
		}
	}
}

void EntityPlayer::render() {}