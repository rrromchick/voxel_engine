#pragma once

struct State {
	enum Type {
		STATE_GAME,
		STATE_MAIN_MENU
	};

	Type type;
	
	State() = delete;
	State(Type type) : type(type) {}

	virtual ~State() = default;

	State(const State &) = delete;
	State(State &&) = default;
	State &operator=(const State &) = delete;
	State &operator=(State &&) = delete;

	virtual void init() {}
	virtual void destroy() {}
	virtual void tick() {}
	virtual void update() {}
	virtual void render() {}
};