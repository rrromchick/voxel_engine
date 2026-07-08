//#pragma once
//
//#include "state.hpp"
//#include "std.hpp"
//#include "typedefs.hpp"
//
//struct Window;
//
//struct StateGame : public State {
//	std::unique_ptr<Window> window;
//
//	bool wireframe = false;
//
//	StateGame() : State(STATE_GAME) {}
//
//	void init() override;
//	void destroy() override;
//	void tick() override;
//	void update() override;
//	void render() override;
//
//	void loop();
//};