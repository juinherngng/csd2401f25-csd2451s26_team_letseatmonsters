/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu

 DESCRIPTION:		Implements the SimpleNpcLogic behaviour, including timed idle-to-move state
					transitions, vertical patrolling based on authored velocity, walk-area clamping,
					and automatic direction reversal when hitting boundaries.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/Collision.hpp"  // for WalkArea definition
#include "../Core/Physics.hpp"      // optional, if you want clamp helpers
#include "../Graphics/SceneManager.hpp"

#include "SimpleNpcLogic.hpp"

void SimpleNpcLogic::Awake(Scene& scene) {
	(void)scene;
	timer = 0.0f;
	state = State::Idle;
	//nextMoveUp = false; // first move: down
}

void SimpleNpcLogic::Update(float dt, Scene& scene, InputManager&) {
	// IMPORTANT: do not run logic in editor mode
	//if (!scene.IsSimulationActive()) {
	//    return;
	//}

	GameObject* npc = GetOwner(scene);
	if (!npc) return;

	timer += dt;

	// Velocity as authored in the editor / level file
	glm::vec2 vel = scene.GetNPCVelocity(npc->GetID());
	glm::vec3 pos = npc->GetPositionGLM();

	// Step 1: compute desired movement based on state
	switch (state) {
		case State::Idle:
		if (timer >= idleDuration) {
			state = nextMoveUp?State::MoveUp:State::MoveDown;
			timer = 0.0f;
		}
		break;

		case State::MoveUp:
		pos.x += vel.x * dt;
		pos.y -= vel.y * dt;
		break;

		case State::MoveDown:
		pos.x += vel.x * dt;
		pos.y += vel.y * dt; // larger y is "down"
		break;
	}

	// Step 2: apply our desired position
	npc->SetPosition(pos);

	// Step 3: clamp to existing walk area / gates
	// This uses your existing world collision logic.
	glm::vec3 beforeClamp = pos;
	scene.ClampToWalkArea(npc);
	glm::vec3 afterClamp = npc->GetPositionGLM();

	// Step 4: detect if we hit a vertical boundary and bounce
	const float eps = 0.1f;
	bool yChangedByClamp = std::fabs(afterClamp.y - beforeClamp.y) > eps;

	if (yChangedByClamp) {
		// We collided with top/bottom. Switch to idle and flip direction.
		if (state == State::MoveUp) {
			nextMoveUp = false; // next time, go down
		}
		else if (state == State::MoveDown) {
			nextMoveUp = true;  // next time, go up
		}

		state = State::Idle;
		timer = 0.0f;
	}
}
