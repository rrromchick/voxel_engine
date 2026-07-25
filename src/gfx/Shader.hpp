#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <exception>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

struct Shader {
	Shader() : id(0) {}
	Shader(unsigned int id) : id(id) {}

	~Shader() {
		if (id != 0) glDeleteProgram(id);
	}

	Shader(const Shader &other) = delete;
	Shader &operator=(const Shader &other) = delete;
	
	Shader(Shader &&other) noexcept
		: id(other.id),
		uniform_locations(std::move(other.uniform_locations)) {
		other.id = 0;
	}

	Shader &operator=(Shader &&other) noexcept {
		if (this == &other) return *this;
		if (id != 0) glDeleteProgram(id);
		id = other.id;
		uniform_locations = std::move(other.uniform_locations);
		other.id = 0;
		return *this;
	}

	inline void use() const {
		glUseProgram(id);
	}

	inline void uniform_matrix(const std::string &name, const glm::mat4 &matrix) {
		glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, glm::value_ptr(matrix));
	}

	inline void uniform_matrix(const std::string &name, const glm::mat3 &matrix) {
		glUniformMatrix3fv(get_uniform_location(name), 1, GL_FALSE, glm::value_ptr(matrix));
	}

	inline void uniform_1i(const std::string &name, int x) {
		glUniform1i(get_uniform_location(name), x);
	}

	inline void uniform_1f(const std::string &name, float x) {
		glUniform1f(get_uniform_location(name), x);
	}

	inline void uniform_2f(const std::string &name, float x, float y) {
		glUniform2f(get_uniform_location(name), x, y);
	}

	inline void uniform_2f(const std::string &name, const glm::vec2 &xy) {
		glUniform2f(get_uniform_location(name), xy.x, xy.y);
	}

	inline void uniform_2i(const std::string &name, const glm::ivec2 &xy) {
		glUniform2i(get_uniform_location(name), xy.x, xy.y);
	}

	inline void uniform_3f(const std::string &name, float x, float y, float z) {
		glUniform3f(get_uniform_location(name), x, y, z);
	}

	inline void uniform_3f(const std::string &name, const glm::vec3 &xyz) {
		glUniform3f(get_uniform_location(name), xyz.x, xyz.y, xyz.z);
	}

	inline void uniform_4f(const std::string &name, const glm::vec4 &xyzw) {
		glUniform4f(get_uniform_location(name), xyzw.x, xyzw.y, xyzw.z, xyzw.w);
	}

	inline void uniform_1v(const std::string &name, int length, const int *v) {
		glUniform1iv(get_uniform_location(name), length, v);
	}

	inline void uniform_1v(const std::string &name, int length, const float *v) {
		glUniform1fv(get_uniform_location(name), length, v);
	}

	inline void uniform_2v(const std::string &name, int length, const float *v) {
		glUniform2fv(get_uniform_location(name), length, v);
	}

	inline void uniform_3v(const std::string &name, int length, const float *v) {
		glUniform3fv(get_uniform_location(name), length, v);
	}

	inline void uniform_4v(const std::string &name, int length, const float *v) {
		glUniform4fv(get_uniform_location(name), length, v);
	}

	static inline Shader *load(
		std::string vertex_file, std::string fragment_file) {
		std::string vertex_code;
		std::string fragment_code;
		std::ifstream v_shader_file;
		std::ifstream f_shader_file;

		v_shader_file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
		f_shader_file.exceptions(std::ifstream::badbit | std::ifstream::failbit);

		try {
			v_shader_file.open(vertex_file);
			f_shader_file.open(fragment_file);
			std::stringstream v_shader_stream, f_shader_stream;

			v_shader_stream << v_shader_file.rdbuf();
			f_shader_stream << f_shader_file.rdbuf();

			v_shader_file.close();
			f_shader_file.close();

			vertex_code = v_shader_stream.str();
			fragment_code = f_shader_stream.str();
		} catch (std::ifstream::failure &e) {
			std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << std::endl;
			return nullptr;
		}

		const GLchar *v_shader_code = vertex_code.c_str();
		const GLchar *f_shader_code = fragment_code.c_str();

		GLuint vertex, fragment;
		GLint success;
		GLchar info_log[512];

		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &v_shader_code, nullptr);
		glCompileShader(vertex);
		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(vertex, 512, nullptr, info_log);
			std::cerr << "SHADER::VERTEX: compilation failed" << std::endl;
			std::cerr << info_log << std::endl;
			return nullptr;
		}

		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &f_shader_code, nullptr);
		glCompileShader(fragment);
		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(fragment, 512, nullptr, info_log);
			std::cerr << "SHADER::FRAGMENT: compilation failed" << std::endl;
			std::cerr << info_log << std::endl;
			return nullptr;
		}

		GLuint id = glCreateProgram();
		glAttachShader(id, vertex);
		glAttachShader(id, fragment);
		glLinkProgram(id);

		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(id, 512, nullptr, info_log);
			std::cerr << "SHADER::PROGRAM: linking failed" << std::endl;
			std::cerr << info_log << std::endl;

			glDeleteShader(vertex);
			glDeleteShader(fragment);
			return nullptr;
		}

		glDeleteShader(vertex);
		glDeleteShader(fragment);

		return new Shader(id);
	}

private:
	unsigned int id;
	std::unordered_map<std::string, int> uniform_locations;

	unsigned int get_uniform_location(const std::string &name) {
		auto found = uniform_locations.find(name);
		if (found == uniform_locations.end()) {
			int location = glGetUniformLocation(id, name.c_str());
			uniform_locations.try_emplace(name, location);
			return location;
		}
		return found->second;
	}
};