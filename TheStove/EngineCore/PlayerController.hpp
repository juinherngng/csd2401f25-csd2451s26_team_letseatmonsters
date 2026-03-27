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

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>

#include "EngineCore/InputManager.hpp"
#include "EngineCore/Math.hpp"
#include "EngineCore/MovementManager.hpp"
#include "EngineCore/PhysicsManager.hpp"
#include "EngineGraphics/EntityManager.hpp"
#include "EngineGraphics/ResourceManager.hpp"

 // Forward declare GraphicsEngine to avoid circular dependency.
class PlayerController {
public:

	/**
	 * @brief Constructs a `PlayerController` instance.
	 */
	PlayerController() = default;

	/**
	 * @brief Handles input.
	 * @param deltaTime Frame delta time in seconds.
	 * @param inputManager Input manager for the current frame.
	 * @param entityManager Entity manager containing the active objects.
	 * @param movementManager Movement manager used for movement updates.
	 * @param physicsManager Physics manager used for physics updates.
	 * @param graphicsEngine Graphics engine used for rendering-related queries.
	 * @param playerID Identifier of the player object.
	 * @param useForces Parameter for use forces.
	 */
	void HandleInput(float deltaTime,
		InputManager& inputManager,
		EntityManager& entityManager,
		MovementManager& movementManager,
		PhysicsManager& physicsManager,
		GraphicsEngine& graphicsEngine,
		int playerID,
		bool useForces);

	/**
	 * @brief Returns rotation.
	 * @return Requested value.
	 */
	float GetRotation() const {
		return rotation_;
	}

	// Input snapshot for PlayerLogic / other systems
	/**
	 * @brief Samples input.
	 * @param deltaTime Frame delta time in seconds.
	 * @param inputManager Input manager for the current frame.
	 * @param graphicsEngine Graphics engine used for rendering-related queries.
	 */
	void SampleInput(float deltaTime,
		InputManager& inputManager,
		GraphicsEngine& graphicsEngine);

	/**
	 * @brief Returns move axis.
	 * @return Requested value.
	 */
	glm::vec2 GetMoveAxis() const {
		return moveAxis_;
	}

	/**
	 * @brief Returns whether click to move just pressed.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsClickToMoveJustPressed() const {
		return clickToMoveJustPressed_ && clickWorldValid_;
	}

	/**
	 * @brief Returns click world.
	 * @return Requested value.
	 */
	glm::vec2 GetClickWorld() const {
		return clickWorld_;
	}

private:

	/**
	 * @brief Updates click indicator.
	 * @param deltaTime Frame delta time in seconds.
	 * @param entityManager Entity manager containing the active objects.
	 */
	void UpdateClickIndicator(float deltaTime, EntityManager& entityManager);

	/**
	 * @brief Handles scale input.
	 * @param inputManager Input manager for the current frame.
	 * @param sprite Parameter for sprite.
	 * @param deltaTime Frame delta time in seconds.
	 */
	void HandleScaleInput(InputManager& inputManager, GameObject* sprite, float deltaTime);

	/**
	 * @brief Handles rotation input.
	 * @param inputManager Input manager for the current frame.
	 * @param deltaTime Frame delta time in seconds.
	 */
	void HandleRotationInput(InputManager& inputManager, float deltaTime);

	/**
	 * @brief Handles click to move.
	 * @param inputManager Input manager for the current frame.
	 * @param entityManager Entity manager containing the active objects.
	 * @param movementManager Movement manager used for movement updates.
	 * @param physicsManager Physics manager used for physics updates.
	 * @param graphicsEngine Graphics engine used for rendering-related queries.
	 * @param playerID Identifier of the player object.
	 * @param useForces Parameter for use forces.
	 */
	void HandleClickToMove(InputManager& inputManager,
		EntityManager& entityManager,
		MovementManager& movementManager,
		PhysicsManager& physicsManager,
		GraphicsEngine& graphicsEngine,
		int playerID,
		bool useForces);

	/**
	 * @brief Updates sprite direction.
	 * @param direction Parameter for direction.
	 * @param sprite Parameter for sprite.
	 */
	void UpdateSpriteDirection(const glm::vec2& direction, GameObject* sprite);

	// Rotation state (degrees).
	float rotation_ = 0.0f;

	// Movement input state for this frame
	glm::vec2 moveAxis_{ 0.0f, 0.0f };  // WASD movement axis
	bool clickToMoveJustPressed_ = false;
	bool clickWorldValid_ = false;
	glm::vec2 clickWorld_{ 0.0f, 0.0f };

	int clickIndicatorID_ = -1;
	glm::vec2 clickIndicatorWorld_{ 0.0f, 0.0f };
	float clickIndicatorTimer_ = 0.0f;
	float clickIndicatorAnimTime_ = 0.0f;
};
