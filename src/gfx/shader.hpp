#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "texture.hpp"
#include "typedefs.hpp"
#include "camera.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

struct Shader {
	uint id;

	Shader() = default;

	explicit Shader(const char *vertex_path, const char *fragment_path) {
		std::string vertex_code;
		std::string fragment_code;
		std::ifstream v_shader_file;
		std::ifstream f_shader_file;

		v_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		f_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try {
			v_shader_file.open(vertex_path);
			f_shader_file.open(fragment_path);
			std::stringstream v_shader_stream, f_shader_stream;

			v_shader_stream << v_shader_file.rdbuf();
			f_shader_stream << f_shader_file.rdbuf();

			v_shader_file.close();
			f_shader_file.close();

			vertex_code = v_shader_stream.str();
			fragment_code = f_shader_stream.str();
		} catch (std::ifstream::failure &e) {
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: "
				<< e.what() << std::endl;
		}

		const char *v_shader_code = vertex_code.c_str();
		const char *f_shader_code = fragment_code.c_str();

		uint vertex, fragment;

		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &v_shader_code, nullptr);
		glCompileShader(vertex);
		this->check_compile_errors(vertex, "VERTEX");

		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &f_shader_code, nullptr);
		glCompileShader(fragment);
		this->check_compile_errors(fragment, "FRAGMENT");

		this->id = glCreateProgram();
		glAttachShader(this->id, vertex);
		glAttachShader(this->id, fragment);
		glLinkProgram(this->id);
		this->check_compile_errors(this->id, "FRAGMENT");

		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	~Shader() {}

	inline void use() const {
		glUseProgram(this->id);
	}

	inline void set_bool(const std::string &name, bool value) const {
		glUniform1i(glGetUniformLocation(this->id, name.c_str()),
			static_cast<int>(value));
	}

	inline void set_int(const std::string &name, int value) const {
		glUniform1i(glGetUniformLocation(this->id, name.c_str()), value);
	}

	inline void set_float(const std::string &name, float value) const {
		glUniform1f(glGetUniformLocation(this->id, name.c_str()), value);
	}

	inline void set_vec2(const std::string &name, const glm::vec2 &value) const {
		glUniform2fv(glGetUniformLocation(this->id, name.c_str()), 1, &value[0]);
	}

	inline void set_vec2(const std::string &name, float x, float y) const {
		glUniform2f(glGetUniformLocation(this->id, name.c_str()), x, y);
	}

	inline void set_vec3(const std::string &name, const glm::vec3 &value) const {
		glUniform3fv(glGetUniformLocation(this->id, name.c_str()), 1, &value[0]);
	}

	inline void set_vec3(const std::string &name, float x, float y, float z) const {
		glUniform3f(glGetUniformLocation(this->id, name.c_str()), x, y, z);
	}
	
	inline void set_vec4(const std::string &name, const glm::vec4 &value) const {
		glUniform4fv(glGetUniformLocation(this->id, name.c_str()), 1, &value[0]);
	}

	inline void set_vec4(const std::string &name, float x, float y, float z, float w) {
		glUniform4f(glGetUniformLocation(this->id, name.c_str()), x, y, z, w);
	}

	inline void set_mat2(const std::string &name, const glm::mat4 &mat) const {
		glUniformMatrix2fv(glGetUniformLocation(this->id, name.c_str()),
			1, GL_FALSE, &mat[0][0]);
	}

	inline void set_mat3(const std::string &name, const glm::mat3 &mat) const {
		glUniformMatrix3fv(glGetUniformLocation(this->id, name.c_str()),
			1, GL_FALSE, &mat[0][0]);
	}

	inline void set_mat4(const std::string &name, const glm::mat4 &mat) const {
		glUniformMatrix4fv(glGetUniformLocation(this->id, name.c_str()),
			1, GL_FALSE, &mat[0][0]);
	}

	inline void set_camera(const Camera &camera) const {
		this->set_mat4("p", camera.proj);
		this->set_mat4("v", camera.view);
	}

	inline void set_texture_2d(
		const std::string &name, const Texture &texture, GLuint n) const {
		glActiveTexture(GL_TEXTURE0 + n);
		texture.bind();
		glUniform1i(glGetUniformLocation(this->id, name.c_str()), n);
	}

private:
	inline void check_compile_errors(GLuint shader, std::string type) {
		GLint success;
		GLchar info_log[1024];

		if (type != "PROGRAM") {
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(shader, 1024, nullptr, info_log);
				std::cout << "ERROR::SHADER::COMPILATION_ERROR of type: "
					<< type << "\n" << info_log << std::endl;
			}
		} else {
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(shader, 1024, nullptr, info_log);
				std::cout << "ERROR::SHADER_LINKING_ERROR of type: "
					<< type << "\n" << info_log << std::endl;
			}
		}
	}
};