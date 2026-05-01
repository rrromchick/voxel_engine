#include "world.hpp"
#include "util/math.hpp"

extern "C" {
	#include <noise1234.h>
}

#define SRAND(seed) srand(seed)
#define RAND(min, max) ((rand() % (max - min + 1)) + min)
#define RANDCHANCE(chance) ((RAND(0, 100000) / 100000.0) <= chance)

#define RADIAL2I(c, r, v)\
	(glm::length(glm::vec2(c) - glm::vec2(v))) / glm::length(glm::vec2(r))

#define RADIAL3I(c, r, v)\
	(glm::length(glm::vec3(c) - glm::vec3(v))) / glm::length(glm::vec3(r))

constexpr int WATER_LEVEL = 64;

enum class Biome {
	OCEAN,
	PLAINS,
	BEACH,
	MOUNTAIN
};

auto octave_compute = [](void *ctx, f32 seed, f32 x, f32 z) -> f32 {
	auto *p = static_cast<Octave *>(ctx);
	f32 u = 1.0f, v = 0.0f;
	for (int i = 0; i < p->n; i++) {
		v += noise3(x / u, z / u, seed + i + (p->o * 32)) * u;
		u *= 2.0f;
	}
	return v;
};

static Noise octave(int n, int o) {
	Noise result(std::move(octave_compute));
	Octave params = { n, o };
	std::memcpy(&result.params, &params, sizeof(Octave));
	return result;
}

auto combined_compute = [](void *ctx, f32 seed, f32 x, f32 z) -> f32 {
	auto *p = static_cast<Combined *>(ctx);
	return p->n->compute(
		&p->n->params, seed, x + p->m->compute(&p->m->params, seed, x, z), z);
};

static Noise combined(Noise *n, Noise *m) {
	Noise result(std::move(combined_compute));
	Combined params = { n, m };
	std::memcpy(&result.params, &params, sizeof(Combined));
	return result;
}

static u32 get(Chunk *chunk, int x, int y, int z) {
	auto p = glm::vec3(x, y, z);
	if (chunk->in_bounds(p)) {
		return chunk->get_data(p);
	} else {
		return chunk->get_world()->get_data(chunk->get_pos() + p);
	}
}

static void set(Chunk *chunk, int x, int y, int z, u32 d) {
	auto p = glm::vec3(x, y, z);
	if (chunk->in_bounds(p)) {
		chunk->set_data(p, d);
	} else {
		chunk->get_world()->set_data(chunk->get_pos() + p, d);
	}
}

static void tree(Chunk *chunk, GetFn get, SetFn set, int x, int y, int z) {
	auto under = static_cast<BlockId>(get(chunk, x, y - 1, z));
	if (under != BlockId::GRASS && under != BlockId::DIRT) {
		return;
	}

	int h = RAND(3, 5);
	
	for (int yy = y; yy <= (y + h); yy++) {
		set(chunk, x, yy, z, BlockId::LOG);
	}

	int lh = RAND(2, 3);

	for (int xx = (x - 2); xx <= (x + 2); xx++) {
		for (int zz = (z - 2); zz <= (z + 2); zz++) {
			for (int yy = (y + h); yy <= (y + h + 1); yy++) {
				int c = 0;
				c += xx == (x - 2) || xx == (x + 2);
				c += zz == (z - 2) || zz == (z + 2);
				bool corner = c == 2;

				if ((!(xx == x && zz == z) || y > (y + h)) &&
					!(corner && yy == (y + h + 1) && RANDCHANCE(0.4))) {
					set(chunk, xx, yy, zz, BlockId::LEAVES);
				}
			}
		}
	}

	for (int xx = (x - 1); xx <= (x + 1); xx++) {
		for (int zz = (z - 1); zz <= (z + 1); zz++) {
			for (int yy = (y + h + 2); yy <= (y + h + lh); yy++) {
				int c = 0;
				c += xx == (x - 1) || xx == (x + 1);
				c += zz == (z - 1) || zz == (z + 1);
				bool corner = c == 2;

				if (!(corner && yy == (y + h + lh) && RANDCHANCE(0.8))) {
					set(chunk, xx, yy, zz, LEAVES);
				}
			}
		}
	}
}

static void flowers(Chunk *chunk, GetFn get, SetFn set, int x, int y, int z) {
	auto flower = RANDCHANCE(0.6) ? BlockId::ROSE : BlockId::BUTTERCUP;

	int s = RAND(2, 6);
	int l = RAND(s - 1, s + 1);
	int h = RAND(s - 1, s + 1);

	for (int xx = (x - l); xx <= (x + l); xx++) {
		for (int zz = (z - h); zz <= (z + h); zz++) {
			auto under = static_cast<BlockId>(get(chunk, xx, y, zz));
			if ((under == GRASS) && RANDCHANCE(0.5)) {
				set(chunk, xx, y + 1, zz, flower);
			}
		}
	}
}

static void orevein(
	Chunk *chunk, GetFn get, SetFn set, int x, int y, int z, BlockId block) {
	int h = RAND(1, y - 4);

	if (h < 0 || h > y - 4) {
		return;
	}

	int s;
	switch (block) {
		case BlockId::COAL:
			s = RAND(2, 4);
			break;
		case BlockId::COPPER:
		default:
			s = RAND(1, 3);
			break;
	}

	int l = RAND(s - 1, s + 1);
	int w = RAND(s - 1, s + 1);
	int i = RAND(s - 1, s + 1);

	for (int xx = (x - l); xx <= (x + l); xx++) {
		for (int zz = (z - w); zz <= (z + w); zz++) {
			for (int yy = (h - i); yy <= (h + i); yy++) {
				f32 d = 1.0f - RADIAL3I(
					glm::vec3(x, h, z), glm::vec3(l + 1, w + 1, i + 1),
					glm::vec3(xx, yy, zz));

				if (get(chunk, xx, yy, zz) == BlockId::STONE && RANDCHANCE(0.2 + d * 0.7)) {
					set(chunk, xx, yy, zz, block);
				}
			}
		}
	}
}

static void lavapool(Chunk *chunk, GetFn get, SetFn set, int x, int y, int z) {
	int h = y + 1;

	int s = RAND(1, 5);
	int l = RAND(s - 1, s + 1);
	int w = RAND(s - 1, s + 1);

	for (int xx = (x - l); xx <= (x + l); xx++) {
		for (int zz = (z - w); zz <= (z + w); zz++) {
			f32 d = 1.0f - RADIAL2I(
				glm::vec2(x, z), glm::vec2(l + 1, w + 1), glm::vec2(xx, zz));
			
			bool allow = true;

			for (int i = -1; i <= 1; i++) {
				for (int j = -1; j <= 1; j++) {
					auto block = static_cast<BlockId>(get(chunk, xx + i, h, zz + j));
					if (block != BlockId::LAVA &&
						blocks[block].is_transparent(chunk->get_world(), glm::ivec3(xx + i, h, zz + i))) {
						allow = false;
						break;
					}
				}
			}

			if (!allow) {
				continue;
			}

			if (RANDCHANCE(0.2 + d * 0.95)) {
				set(chunk, xx, h, zz, BlockId::LAVA);
			}
		}
	}
}

void World::generate(Chunk *chunk) {
	const u64 seed = 2;
	SRAND(seed + math::ivec3hash(chunk->get_offset()));

	Noise n = octave(6, 0);
	Noise m = octave(6, 1);

	std::array<Noise, 6> os = {
		octave(8, 1),
		octave(8, 2),
		octave(8, 3),
		octave(8, 4),
		octave(8, 5),
		octave(8, 6),
	};

	std::array<Noise, 3> cs = {
		combined(&os[0], &os[1]),
		combined(&os[2], &os[3]),
		combined(&os[4], &os[5]),
	};

	for (int x = 0; x < CHUNK_SIZE.x; x++) {
		for (int z = 0; z < CHUNK_SIZE.z; z++) {
			int wx = chunk->get_pos().x + x, wz = chunk->get_pos().z + z;

			const f32 base_scale = 1.3f;
			int hr;
			int hl = (cs[0].compute(&cs[0].params, seed, wx * base_scale, wz * base_scale) / 6.0f) - 4.0f;
			int hh = (cs[1].compute(&cs[1].params, seed, wx * base_scale, wz * base_scale) / 5.0f) + 6.0f;

			f32 t = n.compute(&n.params, seed, wx, wz);
			f32 r = n.compute(&m.params, seed, wx / 4.0f, wz / 4.0f) / 32.0f;

			if (t > 0) {
				hr = hl;
			} else {
				hr = math::max(hh, hl);
			}

			int h = hr + WATER_LEVEL;
			if (h < 0) h = 0;
			if (h > 256) h = 256;

			Biome biome;
			if (h < WATER_LEVEL) {
				biome = Biome::OCEAN;
			} else if (t < 0.08f && h < WATER_LEVEL + 2) {
				biome = Biome::BEACH;
			} else if (false) {
				biome = Biome::MOUNTAIN;
			} else {
				biome = Biome::PLAINS;
			}

			if (biome == Biome::MOUNTAIN) {
				h += (r + (-t / 12.0f)) * 2 + 2;
			}

			int d = r * 1.4f + 5.0f;

			BlockId top_block;
			switch (biome) {
				case Biome::OCEAN:
					if (r > 0.8f) {
						top_block = BlockId::GRAVEL;
					} else if (r > 0.3f) {
						top_block = BlockId::SAND;
					} else if (r > 0.15f && t < 0.08f) {
						top_block = BlockId::CLAY;
					} else {
						top_block = BlockId::DIRT;
					}
					break;
				case Biome::BEACH:
					top_block = BlockId::SAND;
					break;
				case Biome::PLAINS:
					top_block = (t > 4.0f && r > 0.78f) ? BlockId::GRAVEL : BlockId::GRASS;
					break;
				case Biome::MOUNTAIN:
					if (r > 0.8f) {
						top_block = BlockId::GRAVEL;
					} else if (r > 0.7f) {
						top_block = BlockId::DIRT;
					} else {
						top_block = BlockId::STONE;
					}
					break;
			}

			for (int y = 0; y < h; y++) {
				BlockId block;
				if (y == (h - 1)) {
					block = top_block;
				} else if (y > (h - d)) {
					if (top_block == BlockId::GRASS) {
						block = BlockId::DIRT;
					} else {
						block = top_block;
					}
				} else {
					block = BlockId::STONE;
				}

				chunk->set_data(glm::ivec3(x, y, z), block);
			}

			for (int y = h; y < WATER_LEVEL; y++) {
				chunk->set_data(glm::ivec3(x, y, z), WATER);
			}

			if (RANDCHANCE(0.02)) {
				orevein(chunk, get, set, x, h, z, BlockId::COAL);
			}

			if (RANDCHANCE(0.02)) {
				orevein(chunk, get, set, x, h, z, BlockId::COPPER);
			}

			if (biome != Biome::OCEAN && h <= (WATER_LEVEL + 3) && t < 0.1f && RANDCHANCE(0.01)) {
				lavapool(chunk, get, set, x, h, z);
			}

			if (biome == Biome::PLAINS && RANDCHANCE(0.005)) {
				tree(chunk, get, set, x, h, z);
			}

			if (biome == Biome::PLAINS && RANDCHANCE(0.0085)) {
				flowers(chunk, get, set, x, h, z);
			}
		}
	}

	for (usize i = 0; i < chunk->get_world()->unloaded_data.size(); i++) {
		WorldUnloadedData data = chunk->get_world()->unloaded_data[i];
		if (chunk->get_offset() == pos_to_offset(data.pos)) {
			chunk->set_data(pos_to_chunk_pos(data.pos), data.data);
			chunk->get_world()->remove_unloaded_data(i);
		}
	}
}