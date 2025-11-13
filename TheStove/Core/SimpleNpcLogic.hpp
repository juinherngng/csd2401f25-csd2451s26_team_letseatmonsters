#pragma once
#include "GameObjectLogic.hpp"

class SimpleNpcLogic : public GameObjectLogic {
public:
	using GameObjectLogic::GameObjectLogic;

	void Awake(Scene& scene) override;
	void Update(float dt, Scene& scene, InputManager& input) override;
	std::string GetName() const override {
		return "SimpleNpcLogic";
	}

private:
	enum class State {
		Idle, MoveUp, MoveDown
	};

	State state = State::Idle;
	float timer = 0.0f;

	float idleDuration = 0.5f;   // pause at top/bottom
	float speed = 100.0f; // pixels per second

	// Which direction we will move next after an Idle
	bool nextMoveUp = false;     // start by moving DOWN
};
