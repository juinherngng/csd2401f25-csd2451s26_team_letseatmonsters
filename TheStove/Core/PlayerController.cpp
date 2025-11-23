/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerController.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Implements PlayerController. Reads input, adjusts scale/rotation,
					sets click-to-move targets (either physics-based or direct), and
					updates sprite facing textures accordingly.

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "PlayerController.hpp"
#include "../Graphics/GraphicsEngine.hpp"

void PlayerController::HandleInput(float deltaTime,
	InputManager& inputManager,
	EntityManager& entityManager,
	MovementManager& movementManager,
	PhysicsManager& physicsManager,
	GraphicsEngine& graphicsEngine,
	int playerID,
	bool useForces) {
	if (playerID < 0) {
		return;
	}

	GameObject* sprite = entityManager.GetByID(playerID);
	if (!sprite) {
		return;
	}

	// Handle scale controls (Up/Down)
	HandleScaleInput(inputManager, sprite, deltaTime);

	// Handle rotation controls (Left/Right)
	HandleRotationInput(inputManager, deltaTime);
	sprite->SetRotation(rotation_, glm::vec3(0, 0, 1));

	// Handle click-to-move (Left mouse)
	HandleClickToMove(inputManager, entityManager, movementManager,
		physicsManager, graphicsEngine, playerID, useForces);
}

void PlayerController::HandleScaleInput(InputManager& inputManager, GameObject* sprite, float deltaTime) {
	// Simple bounded uniform scaling
	glm::vec3 scale = sprite->GetScaleGLM();

	if (inputManager.IsKeyPressed(GLFW_KEY_UP)) {
		scale *= 1.01f;
		scale = glm::min(scale, glm::vec3(500.0f));
		sprite->SetScale(scale);
	}

	if (inputManager.IsKeyPressed(GLFW_KEY_DOWN)) {
		scale *= 0.99f;
		scale = glm::max(scale, glm::vec3(50.0f));
		sprite->SetScale(scale);
	}
}

void PlayerController::HandleRotationInput(InputManager& inputManager, float deltaTime) {
	// Degrees per second
	const float kRotationSpeed = 10.0f;

	if (inputManager.IsKeyPressed(GLFW_KEY_RIGHT)) {
		rotation_ += kRotationSpeed * deltaTime;
	}

	if (inputManager.IsKeyPressed(GLFW_KEY_LEFT)) {
		rotation_ -= kRotationSpeed * deltaTime;
	}

	// Normalize rotation to [0, 360)
	while (rotation_ >= 360.0f) {
		rotation_ -= 360.0f;
	}

	while (rotation_ < 0.0f) {
		rotation_ += 360.0f;
	}
}

void PlayerController::HandleClickToMove(InputManager& inputManager,
	EntityManager& entityManager,
	MovementManager& movementManager,
	PhysicsManager& physicsManager,
	GraphicsEngine& graphicsEngine,
	int playerID,
	bool useForces) {
	// Only act on the initial press to set a target once.
	if (!inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		return;
	}

	GameObject* sprite = entityManager.GetByID(playerID);
	if (!sprite) {
		return;
	}

	glm::vec2 mouseWorld = {};
	if (!graphicsEngine.GetMouseWorldInScene(mouseWorld)) {
		return;
	}

	// Choose movement mode (forces vs kinematic)
	if (useForces) {
		physicsManager.SetSeekTarget(playerID, Math::Vector2D(mouseWorld.x, mouseWorld.y));
	}
	else {
		movementManager.SetMoveTarget(playerID, mouseWorld);
	}

	// Update facing texture immediately based on target direction.
	const glm::vec3 pos = sprite->GetPositionGLM();
	const glm::vec2 toTarget = mouseWorld - glm::vec2(pos.x, pos.y);
	UpdateSpriteDirection(toTarget, sprite);
}

void PlayerController::UpdateSpriteDirection(const glm::vec2& direction, GameObject* sprite) {
	// Small dead zone to avoid jitter when very close to target.
	if (glm::length(direction) <= 0.001f) {
		return;
	}

	const float absX = std::abs(direction.x);
	const float absY = std::abs(direction.y);

	// Dominant axis decides the facing
	if (absX > absY) {
		// Horizontal dominant
		if (direction.x > 0.0f) {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_right.png", "../assets/mc_sprite_right.png"));
		}
		else {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_left.png", "../assets/mc_sprite_left.png"));
		}
	}
	else {
		// Vertical dominant
		if (direction.y > 0.0f) {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_front.png", "../assets/mc_sprite_front.png"));
		}
		else {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_back.png", "../assets/mc_sprite_back.png"));
		}
	}
}

void PlayerController::SampleInput(float deltaTime,
	InputManager& inputManager,
	GraphicsEngine& graphicsEngine)
{
	(void)deltaTime;

	// --- Movement axis (WASD) ---
	moveAxis_ = glm::vec2(0.0f, 0.0f);

	if (inputManager.IsKeyPressed(GLFW_KEY_A)) moveAxis_.x -= 1.0f;
	if (inputManager.IsKeyPressed(GLFW_KEY_D)) moveAxis_.x += 1.0f;
	if (inputManager.IsKeyPressed(GLFW_KEY_W)) moveAxis_.y -= 1.0f; // up = -Y in your current scheme
	if (inputManager.IsKeyPressed(GLFW_KEY_S)) moveAxis_.y += 1.0f;

	// --- Click-to-move (LMB) snapshot ---
	clickToMoveJustPressed_ = inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);
	clickWorldValid_ = false;

	if (clickToMoveJustPressed_) {
		glm::vec2 mouseWorld{};
		if (graphicsEngine.GetMouseWorldInScene(mouseWorld)) {
			clickWorld_ = mouseWorld;
			clickWorldValid_ = true;
		}
		else {
			// If the click is not over the scene viewport, treat as "no click"
			clickToMoveJustPressed_ = false;
			clickWorldValid_ = false;
		}
	}
}

