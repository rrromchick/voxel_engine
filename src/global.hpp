#pragma once

#include "Time.hpp"
#include "std.hpp"
#include "typedefs.hpp"
#include <array>

struct Window;
struct Chunks;

struct Global {
	std::unique_ptr<Window> window;
    std::unique_ptr<Chunks> chunks;
	std::unique_ptr<Time> time;
};

extern Global global;