/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerController.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Declares PlayerController, which handles player-facing input such as scaling,
					rotation, and click-to-move. Integrates with movement/physics managers and
					updates sprite facing based on the movement direction.

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>

#include "InputManager.hpp"
#include "Math.hpp"
#include "MovementManager.hpp"
#include "PhysicsManager.hpp"

#include "../Graphics/EntityManager.hpp"
#include "../Graphics/ResourceManager.hpp"

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
};
