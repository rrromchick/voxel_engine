#pragma once

#include "std.hpp"
#include "typedefs.hpp"

#define TICKS_PER_SECOND (120)

struct Time {
	using NowFn = std::function<u64(void)>;
	using TickFn = std::function<void(void)>;

	static constexpr u64
		NANOS_PER_SECOND = 1000000000,
		NANOS_PER_MILLIS = 1000000,
		MILLIS_PER_SECOND = 1000,
		NANOS_PER_TICK = (NANOS_PER_SECOND / TICKS_PER_SECOND),
		TICK_MAX = std::numeric_limits<u64>::max();

	template <typename T>
	static inline auto to_millis(T nanos) {
		return nanos / static_cast<T>(NANOS_PER_MILLIS);
	}

	template <typename T>
	static inline auto to_seconds(T nanos) {
		return nanos / static_cast<T>(NANOS_PER_SECOND);
	}

	u64 time, last_frame, last_second;
	u64 ticks, second_ticks, tick_remainder, frame_ticks;
	u64 frames, second_frames, fps;
	u64 delta;

	NowFn now;

	Time() = default;
	Time(const Time &other) = delete;
	Time(Time &&other) = default;
	Time &operator=(const Time &other) = delete;
	Time &operator=(Time &&other) = default;

	explicit Time(NowFn &&now) {
		std::memset(this, 0, sizeof(*this));
		this->now = std::move(now);
		time = this->now();
		last_frame = this->now();
		last_second = this->now();
	}

	void tick(TickFn &&tick);
	void update();
};