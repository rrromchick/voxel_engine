#include "world.hpp"

bool World::chunk_in_bounds(glm::ivec3 offset) const {
	glm::ivec3 p = offset - this->chunks_origin;
	return p.x >= 0 && p.z >= 0 &&
		p.x < this->chunks_size && p.z < this->chunks_size;
}

Chunk *World::get_chunk(glm::ivec3 offset) const {
	if (!this->chunk_in_bounds(offset)) {
		return nullptr;
	} else {
		return this->chunks[chunk_index(offset)];
	}
}

bool World::contains_chunk(glm::ivec3 offset) const {
	return this->get_chunk(offset) != nullptr;
}

bool World::contains(glm::ivec3 pos) const {
	return this->contains_chunk(pos_to_offset(pos));
}

bool World::in_bounds(glm::ivec3 pos) const {
	return this->contains_chunk(pos_to_offset(pos));
}

void World::load_chunk(glm::ivec3 offset) {
	assert(!contains_chunk(offset));

	auto *chunk = new Chunk(this, offset);
	this->generate(chunk);
	this->chunks[chunk_index(offset)] = chunk;
}

World::World(Window *wnd)
	: chunks_size(20), player(this, wnd) {
	this->throttles.load.max = 2;
	this->throttles.mesh.max = 2;

	this->unloaded_data = std::vector<WorldUnloadedData>(WORLD_UNLOADED_DATA_CAP);
	
	this->chunks = std::vector<Chunk *>(chunks_size * chunks_size);
	this->set_center(glm::ivec3(0.0f));
}

World::~World() {
	world_foreach(this, chunk) {
		if (chunk != nullptr) {
			delete chunk;
		}
	}
}

void World::append_unloaded_data(glm::ivec3 pos, u32 data) {
	this->unloaded_data.emplace_back(WorldUnloadedData(pos, data));
}

void World::remove_unloaded_data(usize i) {
	if (i < this->unloaded_data.size()) {
		unloaded_data.erase(unloaded_data.begin() + i);
	}
}

void World::set_data(glm::ivec3 pos, u32 data) {
	if (pos.y < 0 || pos.y >= CHUNK_SIZE.y) {
		return;
	}

	glm::ivec3 offset = this->pos_to_offset(pos);
	if (this->contains_chunk(offset)) {
		this->get_chunk(offset)->set_data(pos_to_chunk_pos(pos), data);
	} else {
		this->append_unloaded_data(pos, data);
	}
}

u32 World::get_data(glm::ivec3 pos) const {
	glm::ivec3 offset = this->pos_to_offset(pos);
	if (pos.y >= 0 && pos.y < CHUNK_SIZE.y && this->contains_chunk(offset)) {
		return this->get_chunk(offset)->get_data(pos_to_chunk_pos(pos));
	}
	return 0;
}

void World::load_empty_chunks() {
	for (usize i = 0; i < chunks_size * chunks_size; i++) {
		if (this->chunks[i] == nullptr &&
			this->throttles.load.count < this->throttles.load.max) {
			this->load_chunk(chunk_offset(i));
			this->throttles.load.count++;
		}
	}
}

void World::set_center(glm::ivec3 center_pos) {
	glm::ivec3 new_offset = pos_to_offset(center_pos);
	glm::ivec3 new_origin = 
		new_offset - glm::ivec3((this->chunks_size / 2) - 1, 0, (this->chunks_size / 2) - 1);
	
	if (!std::memcmp(&new_origin, &this->chunks_origin, sizeof(glm::ivec3))) {
		return;
	}

	this->center_offset = new_offset;
	this->chunks_origin = new_origin;

	usize n_chunks = chunks_size * chunks_size;

	std::vector<Chunk *> old = std::move(this->chunks);
	this->chunks.assign(chunks_size * chunks_size, nullptr);

	for (usize i = 0; i < n_chunks; i++) {
		Chunk *c = old[i];
		if (c == nullptr) {
			continue;
		} else if (chunk_in_bounds(c->get_offset())) {
			this->chunks[chunk_index(c->get_offset())] = c;
		} else {
			delete c;
		}
	}

	load_empty_chunks();
}

void World::render() {
	std::vector<glm::ivec3> offsets;
	for (auto *chunk : this->chunks) {
		if (chunk) offsets.push_back(chunk->get_offset());
	}
	
	std::sort(offsets.begin(), offsets.end(), btf_cmp);
	for (auto &offset : offsets) {
		if (auto *chunk = this->get_chunk(offset)) {
			chunk->render();

			glDisable(GL_CULL_FACE);
			chunk->render_transparent();
			glEnable(GL_CULL_FACE);
		}
	}

	this->player.render();
}

void World::update() {
	this->throttles.load.count = 0;
	this->throttles.mesh.count = 0;

	load_empty_chunks();

	world_foreach(this, chunk) {
		if (chunk != nullptr) {
			chunk->update();
		}
	}

	this->player.update();
}

void World::tick() {
	world_foreach(this, chunk) {
		if (chunk != nullptr) {
			chunk->tick();
		}
	}

	this->player.tick();
}