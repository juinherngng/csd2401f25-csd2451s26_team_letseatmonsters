/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerController.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(40%)
					Vu Phan Hung, phanhung.vu@digipen.edu	(20%)

 DESCRIPTION:		Declares PlayerController, which handles player-facing input such as scaling,
					rotation, and click-to-move. Integrates with movement/physics managers and
					updates sprite facing based on the movement direction.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "../Graphics/EntityManager.hpp"
#include "../Graphics/ResourceManager.hpp"

#include "InputManager.hpp"
#include "Math.hpp"
#include "MovementManager.hpp"
#include "PhysicsManager.hpp"

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>

 /**
  * @class PlayerController
  * @brief High-level input bridge for the player. Reads input and delegates actions to the
  *        movement and physics systems while keeping the sprite visuals in sync.
  */
class PlayerController {
public:
	PlayerController() = default;

	// Main per - frame input handler for the player.
	void HandleInput(float deltaTime,
		InputManager& inputManager,
		EntityManager& entityManager,
		MovementManager& movementManager,
		PhysicsManager& physicsManager,
		GraphicsEngine& graphicsEngine,
		int playerID,
		bool useForces);

	float GetRotation() const {
		return rotation_;
	}

	// -------- New: input snapshot for PlayerLogic / other systems --------
	// Sample input for this frame (WASD + click-to-move) WITHOUT moving anything.
	void SampleInput(float deltaTime,
		InputManager& inputManager,
		GraphicsEngine& graphicsEngine);

	// Movement axis from WASD (-1..1 per axis). Same semantics as your old code.
	glm::vec2 GetMoveAxis() const {
		return moveAxis_;
	}

	// Click-to-move snapshot (LMB). Valid only for the frame where the click happened.
	bool IsClickToMoveJustPressed() const {
		return clickToMoveJustPressed_ && clickWorldValid_;
	}
	glm::vec2 GetClickWorld() const {
		return clickWorld_;
	}

private:
	// Handles up/down key scaling with clamped bounds.
	void HandleScaleInput(InputManager& inputManager, GameObject* sprite, float deltaTime);

	// Handles left / right key rotation and normalizes the angle.
	void HandleRotationInput(InputManager& inputManager, float deltaTime);

	// Handles left - click to set a new target(forces or kinematic).
	void HandleClickToMove(InputManager& inputManager,
		EntityManager& entityManager,
		MovementManager& movementManager,
		PhysicsManager& physicsManager,
		GraphicsEngine& graphicsEngine,
		int playerID,
		bool useForces);

	// Update player sprite texture based on movement direction
	void UpdateSpriteDirection(const glm::vec2& direction, GameObject* sprite);

	// Rotation state (degrees).
	float rotation_ = 0.0f;

	glm::vec2 moveAxis_{ 0.0f, 0.0f };  // WASD movement axis
	bool clickToMoveJustPressed_ = false;
	bool clickWorldValid_ = false;
	glm::vec2 clickWorld_{ 0.0f, 0.0f };
};
