#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "gfx/shader.hpp"
#include "typedefs.hpp"

#define MAX_BONE_INFLUENCE 4

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 tex_coords;
	glm::vec3 tangent;
	glm::vec3 bitangent;

	int bone_ids[MAX_BONE_INFLUENCE];
	f32 weights[MAX_BONE_INFLUENCE];
};

struct Texture {
	uint id;
	std::string type;
	std::string path;
};

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<uint> indices;
	std::vector<Texture> textures;
	GLuint vao;

	Mesh(
		std::vector<Vertex> vertices,
		std::vector<uint> indices,
		std::vector<Texture> textures) {
		this->vertices = std::move(vertices);
		this->indices = std::move(indices);
		this->textures = std::move(textures);

		this->setup_mesh();
	}

	void draw(Shader &shader) {
		uint diffuse_nr = 1;
		uint specular_nr = 1;
		uint normal_nr = 1;
		uint height_nr = 1;
		for (usize i = 0; i < this->textures.size(); i++) {
			glActiveTexture(GL_TEXTURE0 + i);

			std::string number;
			std::string name = this->textures[i].type;
			if (name == "texture_diffuse") {
				number = std::to_string(diffuse_nr++);
			} else if (name == "texture_specular") {
				number = std::to_string(specular_nr++);
			} else if (name == "texture_normal") {
				number = std::to_string(normal_nr++);
			} else if (name == "texture_height") {
				number = std::to_string(height_nr++);
			}

			glUniform1i(
				glGetUniformLocation(shader.id, (name + number).c_str()), i);
			glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
		}

		glBindVertexArray(this->vao);
		glDrawElements(
			GL_TRIANGLES,
			static_cast<uint>(indices.size()),
			GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);
		glActiveTexture(GL_TEXTURE0);
	}

private:
	uint vbo, ebo;

	void setup_mesh() {
		glGenVertexArrays(1, &this->vao);
		glGenBuffers(1, &this->vbo);
		glGenBuffers(1, &this->ebo);

		glBindVertexArray(this->vao);
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo);

		glBufferData(
			GL_ARRAY_BUFFER,
			this->vertices.size() * sizeof(Vertex),
			&this->vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
		glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			this->indices.size() * sizeof(uint),
			&this->indices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(
			0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
			reinterpret_cast<void *>(0));

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(
			1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
			reinterpret_cast<void *>(offsetof(Vertex, normal)));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(
			2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
			reinterpret_cast<void *>(offsetof(Vertex, tex_coords)));

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(
			3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
			reinterpret_cast<void *>(offsetof(Vertex, tangent)));

		glEnableVertexAttribArray(4);
		glVertexAttribPointer(
			4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
			reinterpret_cast<void *>(offsetof(Vertex, bitangent)));

		glEnableVertexAttribArray(5);
		glVertexAttribIPointer(
			5, 4, GL_INT, sizeof(Vertex),
			reinterpret_cast<void *>(offsetof(Vertex, bone_ids)));

		glEnableVertexAttribArray(6);
		glVertexAttribPointer(
			6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
			reinterpret_cast<void *>(offsetof(Vertex, weights)));

		glBindVertexArray(0);
	}
};