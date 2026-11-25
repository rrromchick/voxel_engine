#include <gtest/gtest.h>
#include <cstring>

#include "Lightmap.hpp"
#include "LightSolver.hpp"
#include "Lighting.hpp"
#include "Level.hpp"
#include "Chunk.hpp"
#include "Block.hpp"
#include "Global.hpp"
#include "Window.hpp"

Global global;

namespace {
    constexpr uint8_t BLOCK_AIR = 0;
    constexpr uint8_t BLOCK_SOLID = 1;
    constexpr uint8_t BLOCK_LAMP = 2;

    struct LightingTestEnv : ::testing::Test {
        void SetUp() override {
            global.blocks[BLOCK_AIR] = std::make_unique<Block>(BLOCK_AIR, 0);
            global.blocks[BLOCK_AIR]->light_passing = true;
            global.blocks[BLOCK_AIR]->obstacle = false;

            global.blocks[BLOCK_SOLID] = std::make_unique<Block>(BLOCK_SOLID, 1);
            global.blocks[BLOCK_SOLID]->light_passing = false;
            global.blocks[BLOCK_SOLID]->obstacle = true;

            global.blocks[BLOCK_LAMP] = std::make_unique<Block>(BLOCK_LAMP, 2);
            global.blocks[BLOCK_LAMP]->light_passing = true;
            global.blocks[BLOCK_LAMP]->obstacle = false;
            global.blocks[BLOCK_LAMP]->emission = { 15, 0, 0 };

            global.ecs = std::make_unique<ECS>();
            global.ecs->level = std::make_unique<Level>();
            global.lighting = std::make_unique<Lighting>();

            create_chunk(0, 0, 0);
            create_chunk(1, 0, 0);
        }

        void TearDown() override {
            global.lighting.reset();
            global.ecs.reset();
            for (auto &b : global.blocks) {
                b.reset();
            }
        }

        void create_chunk(int cx, int cy, int cz) {
            auto chunk = std::make_unique<Chunk>(cx, cy, cz);
            std::memset(chunk->voxels.get(), 0, Chunk::VOLUME * sizeof(voxel));
            chunk->lightmap = std::make_unique<Lightmap>();
            global.ecs->level->chunks[ChunkPos{cx, cy, cz}] = std::move(chunk);
        }

        void place_block(int x, int y, int z, uint8_t id) {
            auto *level = global.ecs->level.get();
            if (auto *vox = level->get_voxel(x, y, z)) {
                vox->id = id;
            }
            if (auto *chunk = level->get_chunk_by_voxel(x, y, z)) {
                chunk->modified = true;
            }
            global.lighting->on_block_set(x, y, z, id);
        }
    };

    TEST_F(LightingTestEnv, LightDecaysAcrossChunkBoundaryAndRespectsSolidWall) {
        auto *level = global.ecs->level.get();
        const int lamp_x = Chunk::WIDTH - 1;
        const int wall_x = Chunk::WIDTH + 1;

        place_block(lamp_x, 1, 1, BLOCK_LAMP);
        EXPECT_EQ(level->get_light(lamp_x, 1, 1, 0), 15);
        EXPECT_EQ(level->get_light(Chunk::WIDTH, 1, 1, 0), 14);
        EXPECT_EQ(level->get_light(wall_x, 1, 1, 0), 13);
        place_block(wall_x, 1, 1, BLOCK_SOLID);
        EXPECT_EQ(level->get_light(Chunk::WIDTH, 1, 1, 0), 14);
    }

    TEST_F(LightingTestEnv, RemovingOneOfTwoSourcesKeepsSecondSourceLight) {
        auto *level = global.ecs->level.get();

        place_block(2, 2, 2, BLOCK_LAMP);
        place_block(15, 2, 2, BLOCK_LAMP);

        place_block(2, 2, 2, BLOCK_AIR);
        EXPECT_LT(level->get_light(2, 2, 2, 0), 15);
        EXPECT_EQ(level->get_light(15, 2, 2, 0), 15);
        EXPECT_GT(level->get_light(10, 2, 2, 0), 0);
    }

    TEST_F(LightingTestEnv, ClearResetsLightmapChannels) {
        auto *level = global.ecs->level.get();
        place_block(2, 2, 2, BLOCK_LAMP);
        ASSERT_EQ(level->get_light(2, 2, 2, 0), 15);

        global.lighting->clear();
        EXPECT_EQ(level->get_light(2, 2, 2, 0), 0);
        EXPECT_EQ(level->get_light(3, 2, 2, 0), 0);
    }
} 
