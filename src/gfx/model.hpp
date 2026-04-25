#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "typedefs.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "gfx/mesh.hpp"
#include "gfx/shader.hpp"

unsigned int texture_from_file(
	const char *path,
	const std::string &directory,
	bool gamma = false);

struct Model {
	std::vector<texture> textures_loaded;
	std::vector<Mesh> meshes;
	std::string directory;
	bool gamma_correction;

	Model(
		const std::string &path,
		bool gamma = false)
		: gamma_correction(gamma) {
		this->load_model(path);
	}

	void draw(Shader &shader) {
		for (usize i = 0; i < this->meshes.size(); i++) {
			this->meshes[i].draw(shader);
		}
	}

private:
	void load_model(const std::string &path) {
		Assimp::Importer importer;
		const aiScene *scene = importer.ReadFile(
			path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
			aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
			return;
		}

		this->directory = path.substr(0, path.find_last_of('/'));
		this->process_node(scene->mRootNode, scene);
	}

	void process_node(aiNode *node, const aiScene *scene) {
		for (uint i = 0; i < node->mNumMeshes; i++) {
			aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
			this->meshes.push_back(process_mesh(mesh, scene));
		}

		for (uint i = 0; i < node->mNumChildren; i++) {
			this->process_node(node->mChildren[i], scene);
		}
	}

	Mesh process_mesh(aiMesh *mesh, const aiScene *scene) {
		std::vector<vertex> vertices;
		std::vector<uint> indices;
		std::vector<texture> textures;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			vertex vert;
			glm::vec3 vector;

			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vert.position = vector;

			if (mesh->HasNormals()) {
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vert.normal = vector;
			}

			if (mesh->mTextureCoords[0]) {
				glm::vec2 vec;

				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vert.tex_coords = vec;

				vector.x = mesh->mTangents[i].x;
				vector.y = mesh->mTangents[i].y;
				vector.z = mesh->mTangents[i].z;
				vert.tangent = vector;

				vector.x = mesh->mBitangents[i].x;
				vector.y = mesh->mBitangents[i].y;
				vector.z = mesh->mBitangents[i].z;
				vert.bitangent = vector;
			} else {
				vert.tex_coords = glm::vec2(0.0f, 0.0f);
			}

			vertices.push_back(vert);
		}

		for (uint i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];

			for (uint j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j]);
			}
		}

		aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

		std::vector<texture> diffuse_maps = this->load_material_textures(
			material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuse_maps.begin(), diffuse_maps.end());

		std::vector<texture> specular_maps = this->load_material_textures(
			material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specular_maps.begin(), specular_maps.end());

		std::vector<texture> normal_maps = this->load_material_textures(
			material, aiTextureType_AMBIENT, "texture_normal");
		textures.insert(textures.end(), normal_maps.begin(), normal_maps.end());
	
		std::vector<texture> height_maps = load_material_textures(
			material, aiTextureType_AMBIENT, "texture_height");
		textures.insert(textures.end(), height_maps.begin(), height_maps.end());

		return Mesh(vertices, indices, textures);
	}

	std::vector<texture> load_material_textures(
		aiMaterial *mat,
		aiTextureType type,
		std::string type_name) {
		std::vector<texture> textures;
		for (uint i = 0; i < mat->GetTextureCount(type); i++) {
			aiString str;
			mat->GetTexture(type, i, &str);

			bool skip = false;
			for (uint j = 0; j < this->textures_loaded.size(); j++) {
				if (std::strcmp(
					this->textures_loaded[j].path.data(), str.C_Str()) == 0) {
					textures.push_back(textures_loaded[j]);
					skip = true;
					break;
				}
			}

			if (!skip) {
				texture tex;
				tex.id = texture_from_file(str.C_Str(), this->directory);
				tex.type = type_name;
				tex.path = str.C_Str();
				textures.push_back(tex);
				this->textures_loaded.push_back(tex);
			}
		}
		return textures;
	}
};

unsigned int texture_from_file(
	const char *path,
	const std::string &directory,
	bool gamma) {
	std::string filename = std::string(path);
	filename = directory + '/' + filename;

	unsigned int texture_id;
	glGenTextures(1, &texture_id);

	int width, height, nr_components;
	unsigned char *data = stbi_load(
		filename.c_str(), &width, &height, &nr_components, 0);

	if (data) {
		GLenum format;
		if (nr_components == 1) {
			format = GL_RED; 
		} else if (nr_components == 3) {
			format = GL_RGB;
		} else if (nr_components == 4) {
			format = GL_RGBA;
		}

		glBindTexture(GL_TEXTURE_2D, texture_id);
		glTexImage2D(
			GL_TEXTURE_2D,
			0, format, width, height,
			0, format, GL_UNSIGNED_BYTE, data);

		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(
			GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	} else {
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return texture_id;
}