#include "chunk.hpp"
#include "world.hpp"
#include "state.hpp"

GlobalBuffer global_buffers[3];

Mesh::Mesh(Chunk *chunk) 
	: chunk(chunk),
	vbo(GL_ARRAY_BUFFER, false),
	ibo(GL_ELEMENT_ARRAY_BUFFER, false) {
	this->vao = VAO();

	std::array<MeshBuffer *, 3> buffers = {
		&this->data, &this->indices, &this->faces,
	};

	std::array<usize, 3> buffer_caps =
	{ DATA_BUFFER_SIZE, INDICES_BUFFER_SIZE, FACES_BUFFER_SIZE };

	for (usize i = 0; i < buffers.size(); i++) {
		buffers[i]->capacity = std::move(buffer_caps[i]);
	}
}

void Mesh::depth_sort(glm::vec3 center) {
	auto *faces_ptr = static_cast<Face *>(this->faces.data);
	auto *mesh_indices = static_cast<u16 *>(this->indices.data);

	for (usize i = 0; i < this->faces.count; i++) {
		faces_ptr[i].distance = glm::length2(center - faces_ptr[i].position);
	}

	std::sort(faces_ptr, faces_ptr + this->faces.count, [](const Face &a, const Face &b) -> int {
		return static_cast<int>(-glm::sign(a.distance - b.distance));
	});
	
	u16 *t = global_buffers[2].indices.data();

	for (usize i = 0; i < this->faces.count; i++) {
		auto &face = faces_ptr[i];

		std::memcpy(
			&t[i * 6],
			&mesh_indices[face.indices_base],
			6 * sizeof(u16));
		face.indices_base = static_cast<u32>(i * 6);
	}
	
	std::memcpy(mesh_indices, t, this->indices.count * sizeof(u16));
}

void Mesh::prepare(usize global_buffers_index) {
	this->vertex_count = 0;

	std::array<MeshBuffer*, 3> buffers = {
		&this->data, &this->indices, &this->faces
	};

	for (usize i = 0; i < buffers.size(); i++) {
		auto *buffer = buffers[i];
		buffer->count = 0;
		buffer->index = 0;
		buffer->data = global_buffers[global_buffers_index][i];
	}
}

void Mesh::finalize(bool depth_sort) {
	std::array<MeshBuffer *, 3> buffers = {
		&this->data, &this->indices, &this->faces
	};

	for (usize i = 0; i < 3; i++) {
		auto *buffer = buffers[i];
		buffer->count = buffer->index;
		buffer->index = 0;
	}

	this->vbo.sub_data(
		this->data.data, 0, (this->data.count) * sizeof(f32));

	if (depth_sort) {
		this->depth_sort(this->chunk->get_world()->get_player()->get_camera()->position);
	}

	this->ibo.sub_data(
		this->indices.data, 0, (this->indices.count) * sizeof(u16));
}

void Mesh::render() {
	auto state = chunk->get_world()->get_state();
	state->shader->use();
	state->shader->set_camera(*state->get_world()->get_player()->get_camera());

	auto m = glm::mat4(1.0f);
	m = glm::translate(m, this->chunk->get_pos());
	state->shader->set_mat4("m", std::move(m));

	state->shader->set_texture_2d("tex", *(state->get_atlas()->get_atlas()->texture()), 0);
	
	const usize vertex_size = 8 * sizeof(f32);
	this->vao.attr(this->vbo, 0, 3, GL_FLOAT, vertex_size, 0 * sizeof(f32));
	this->vao.attr(this->vbo, 1, 2, GL_FLOAT, vertex_size, 3 * sizeof(f32));
	this->vao.attr(this->vbo, 2, 3, GL_FLOAT, vertex_size, 5 * sizeof(f32));

	this->vao.bind();
	this->ibo.bind();
	glDrawElements(GL_TRIANGLES, this->indices.count, GL_UNSIGNED_SHORT, nullptr);
}

void Chunk::emit_sprite(
	Mesh *mesh, glm::vec3 position, glm::vec2 uv_offset, glm::vec2 uv_unit) const {
	for (usize i = 0; i < 2; i++) {
		Face face;
		face.indices_base = mesh->indices.index + (i * 6);
		face.position = position;

		std::memcpy(
			static_cast<Face *>(mesh->faces.data) + (mesh->faces.index),
			&face, sizeof(Face));
		mesh->faces.index++;
	}

	auto *data = static_cast<f32 *>(mesh->data.data);
	for (usize i = 0; i < 8; i++) {
		const f32 *vertex = &CUBE_VERTICES[i * 3];

		data[mesh->data.index++] = position.x + vertex[0];
		data[mesh->data.index++] = position.y + vertex[1];
		data[mesh->data.index++] = position.z + vertex[2];
		data[mesh->data.index++] = uv_offset.x + (uv_unit.x * CUBE_UVS[((i % 4) * 2) + 0]);
		data[mesh->data.index++] = uv_offset.y + (uv_unit.y * CUBE_UVS[((i % 4) * 2) + 1]);
		
		for (usize j = 0; j < 3; j++) {
			data[mesh->data.index++] = 1.0f;
		}
	}

	auto *indices = static_cast<u16 *>(mesh->indices.data);
	for (usize i = 0; i < 12; i++) {
		indices[mesh->indices.index++] = mesh->vertex_count + SPRITE_INDICES[i];
	}

	mesh->vertex_count += 8;
}

void Chunk::emit_face(
	Mesh *mesh, glm::vec3 position, Direction direction,
	glm::vec2 uv_offset, glm::vec2 uv_unit, bool transparent, bool shorten_y) const {
	if (transparent) {
		Face face;
		face.indices_base = mesh->indices.index;
		face.position = position;

		std::memcpy(
			static_cast<Face *>(mesh->faces.data) + (mesh->faces.index),
			&face, sizeof(Face));
		mesh->faces.index++;
	}

	auto *data = static_cast<f32 *>(mesh->data.data);
	for (usize i = 0; i < 4; i++) {
		const f32 *vertex = &CUBE_VERTICES[CUBE_INDICES[(direction * 6) + UNIQUE_INDICES[i]] * 3];
	
		data[mesh->data.index++] = position.x + vertex[0];
		data[mesh->data.index++] = position.y + ((shorten_y ? 0.9f : 1.0f) * vertex[1]);
		data[mesh->data.index++] = position.z + vertex[2];
		data[mesh->data.index++] = uv_offset.x + (uv_unit.x * CUBE_UVS[(i * 2) + 0]);
		data[mesh->data.index++] = uv_offset.y + (uv_unit.y * CUBE_UVS[(i * 2) + 1]);

		f32 color;
		if (transparent) {
			color = 1.0f;
		} else {
			switch (direction) {
				case UP:
					color = 1.0f;
					break;
				case NORTH:
				case SOUTH:
					color = 0.86f;
					break;
				case EAST:
				case WEST:
					color = 0.8f;
					break;
				case DOWN:
					color = 0.6f;
					break;
			}
		}

		for (usize j = 0; j < 3; j++) {
			data[mesh->data.index++] = color;
		}
	}

	auto *indices = static_cast<u16 *>(mesh->indices.data);
	for (usize i = 0; i < 6; i++) {
		indices[mesh->indices.index++] = mesh->vertex_count + FACE_INDICES[i];
	}

	mesh->vertex_count += 4;
}

void Chunk::get_bordering_chunks(glm::ivec3 pos, std::span<Chunk *, 4> dest) {
	usize i = 0;

	if (pos.x == 0) {
		dest[i++] = this->world->get_chunk(offset + glm::ivec3(-1, 0, 0));
	}

	if (pos.z == 0) {
		dest[i++] = this->world->get_chunk(offset + glm::ivec3(0, 0, -1));
	}

	if (pos.x == (CHUNK_SIZE.x - 1)) {
		dest[i++] = this->world->get_chunk(offset + glm::ivec3(1, 0, 0));
	}

	if (pos.z == (CHUNK_SIZE.z - 1)) {
		dest[i++] = this->world->get_chunk(offset + glm::ivec3(0, 0, 1));
	}
}

void Chunk::set_data(glm::ivec3 pos, u32 data) {
	assert(this->in_bounds(pos));
	this->data[chunk_pos_to_index(pos)] = data;
	this->dirty = true;

	if (this->on_bounds(pos)) {
		std::array<Chunk *, 4> neighbors = { nullptr, nullptr, nullptr, nullptr };
		this->get_bordering_chunks(pos, neighbors);

		for (usize i = 0; i < 2; i++) {
			if (neighbors[i] != nullptr) {
				neighbors[i]->dirty = true;
			}
		}
	}
}

u32 Chunk::get_data(glm::ivec3 pos) const {
	assert(this->in_bounds(pos));
	return this->data[chunk_pos_to_index(pos)];
}

void Chunk::mesh(MeshPass pass) {
	if (pass == MeshPass::FULL) {
		this->base.prepare(0);
	}

	this->transparent.prepare(1);

	chunk_foreach(pos) {
		auto fpos = static_cast<glm::vec3>(pos);
		glm::ivec3 wpos = pos + this->position;

		u32 data = this->data[chunk_pos_to_index(pos)];
		if (data != 0) {
			Block block = blocks[static_cast<BlockId>(data)], neighbor_block;
			bool transparent = block.is_transparent(this->world, wpos);

			if (block.is_sprite()) {
				auto state = world->get_state();
				this->emit_sprite(
					&this->transparent, fpos,
					state->block_atlas->get_atlas()->offset(blocks[(BlockId) data].get_texture_location(this->world, wpos, NORTH)),
					state->block_atlas->get_atlas()->unit());
			} else {
				bool shorten_y = false;

				if (block.is_liquid()) {
					auto up = glm::ivec3(pos.x, pos.y + 1, pos.z);

					if (this->in_bounds(up)) {
						shorten_y = 
							!blocks[static_cast<BlockId>(this->data[chunk_pos_to_index(up)])].is_liquid();
					} else {
						shorten_y = 
							!blocks[static_cast<BlockId>(this->world->get_data(up + this->position))].is_liquid();
					}
				}

				for (auto d = 0; d < 6; d++) {
					auto dir = static_cast<Direction>(d);
					glm::ivec3 dv = direction::dir_to_vec3(dir);
					glm::ivec3 neighbor = pos + dv, wneighbor = wpos + dv;

					if (this->in_bounds(neighbor)) {
						neighbor_block = blocks[
							static_cast<BlockId>(this->data[chunk_pos_to_index(neighbor)])];
					} else {
						neighbor_block = blocks[
							static_cast<BlockId>(this->world->get_data(wneighbor))];
					}

					bool neighbor_transparent = neighbor_block.is_transparent(this->world, wneighbor);
					if (neighbor_transparent && (
						(pass == MeshPass::FULL && !transparent) ||
						(transparent && neighbor_block.id != block.id))) {
						auto state = world->get_state();
						this->emit_face(
							transparent ? &this->transparent : &this->base, fpos, dir,
							state->block_atlas->get_atlas()->offset(block.get_texture_location(this->world, wpos, dir)),
							state->block_atlas->get_atlas()->unit(), transparent, shorten_y);
					}
				}
			}
		}
	}

	if (pass == MeshPass::FULL) {
		this->base.finalize(false);
	}

	this->transparent.finalize(true);
}

void Chunk::render() {
	if ((this->dirty || this->depth_sort) &&
		this->world->throttles.mesh.count < this->world->throttles.mesh.max) {
		this->mesh(this->dirty ? MeshPass::FULL : MeshPass::TRANSPARENCY);

		this->dirty = false;
		this->depth_sort = false;
		this->world->throttles.mesh.count++;
	}

	this->base.render();
}

void Chunk::render_transparent() {
	this->transparent.render();
}

void Chunk::update() {
	EntityPlayer *player = this->world->get_player();
	this->depth_sort =
		((this->offset == player->offset) && player->block_pos_changed) ||
		(player->offset_changed && glm::distance(glm::vec3(this->offset), glm::vec3(player->offset)) < 2);
}

void Chunk::tick() {}