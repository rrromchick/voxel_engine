#pragma once

#include "Time.hpp"
#include "Block.hpp"
#include "Lighting.hpp"
#include "WorldGenerator.hpp"
#include "WorldFiles.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "LineBatch.hpp"
#include "Mesh.hpp"
#include <array>

struct Window;
struct Chunks;

struct Global {
	std::unique_ptr<Window> window;
    std::unique_ptr<Chunks> chunks;
	std::unique_ptr<Time> time;
    std::unique_ptr<Lighting> lighting;
    std::unique_ptr<WorldGenerator> generator;
    std::unique_ptr<WorldFiles> world_files;
    std::unique_ptr<Shader> shader, lines_shader, crosshair_shader;
    std::unique_ptr<Texture> texture;
    std::unique_ptr<LineBatch> line_batch;
    std::unique_ptr<Mesh> crosshair;

    std::array<std::unique_ptr<Block>, 256> blocks;
};

extern Global global;