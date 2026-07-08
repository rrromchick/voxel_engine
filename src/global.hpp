#pragma once

#include "std.hpp"
#include "typedefs.hpp"
#include "util/time.hpp"
#include <array>

struct StateGame;
struct Window;

struct Global {
	std::unique_ptr<Window> window;
	std::unique_ptr<Time> time;
};

extern Global global;