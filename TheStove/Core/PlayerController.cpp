/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerController.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(30%)
					Vu Phan Hung, phanhung.vu@digipen.edu	(30%)

 DESCRIPTION:		Implements PlayerController. Reads input, adjusts scale/rotation,
					sets click-to-move targets (either physics-based or direct), and
					updates sprite facing textures accordingly.

		 All content  2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Graphics/GraphicsEngine.hpp"

#include "PlayerController.hpp"

#include <algorithm>
#include <vector>

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
 * @return Result produced by this operation.
 */
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

	UpdateClickIndicator(deltaTime, entityManager);
}

/**
 * @brief Handles scale input.
 * @param inputManager Input manager for the current frame.
 * @param sprite Parameter for sprite.
 * @param deltaTime Frame delta time in seconds.
 * @return Result produced by this operation.
 */
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

	(void)deltaTime;
}

/**
 * @brief Handles rotation input.
 * @param inputManager Input manager for the current frame.
 * @param deltaTime Frame delta time in seconds.
 * @return Result produced by this operation.
 */
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

/**
 * @brief Handles click to move.
 * @param inputManager Input manager for the current frame.
 * @param entityManager Entity manager containing the active objects.
 * @param movementManager Movement manager used for movement updates.
 * @param physicsManager Physics manager used for physics updates.
 * @param graphicsEngine Graphics engine used for rendering-related queries.
 * @param playerID Identifier of the player object.
 * @param useForces Parameter for use forces.
 * @return Result produced by this operation.
 */
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

	// Trigger click indicator using animated arrow sprite-sheet artwork.
	clickIndicatorWorld_ = mouseWorld;
	clickIndicatorTimer_ = 0.5f;
	clickIndicatorAnimTime_ = 0.0f;
}

/**
 * @brief Updates click indicator.
 * @param deltaTime Frame delta time in seconds.
 * @param entityManager Entity manager containing the active objects.
 * @return Result produced by this operation.
 */
void PlayerController::UpdateClickIndicator(float deltaTime, EntityManager& entityManager) {
	if (clickIndicatorTimer_ <= 0.0f) {
		return;
	}

	if (clickIndicatorID_ < 0 || entityManager.GetByID(clickIndicatorID_) == nullptr) {
		constexpr float kIndicatorSize = 96.0f;
		std::vector<glm::vec4> arrowFrames;
		arrowFrames.reserve(8);
		constexpr float kFrameWidth = 1.0f / 8.0f;
		for (int i = 0; i < 8; ++i) {
			arrowFrames.emplace_back(i * kFrameWidth, 0.0f, kFrameWidth, 1.0f);
		}

		GameObject* indicator = entityManager.SpawnAnimatedSprite(
			"../assets/arrow-Sheet.png",
			glm::vec3(clickIndicatorWorld_.x, clickIndicatorWorld_.y, 0.0f),
			glm::vec2(kIndicatorSize, kIndicatorSize),
			arrowFrames,
			0.06f,
			true);

		if (!indicator) {
			return;
		}

		clickIndicatorID_ = indicator->GetID();
		indicator->SetRenderLayer(99);
		indicator->SetRenderSortOrder(1000);
	}

	GameObject* indicator = entityManager.GetByID(clickIndicatorID_);
	if (!indicator) {
		clickIndicatorID_ = -1;
		return;
	}

	clickIndicatorTimer_ = std::max(0.0f, clickIndicatorTimer_ - deltaTime);
	clickIndicatorAnimTime_ += deltaTime;

	const float normalized = clickIndicatorTimer_ / 0.5f;
	const float pulse = 1.0f + 0.08f * std::sin((1.0f - normalized) * 14.0f);

	constexpr int kArrowFrameCount = 8;
	constexpr float kFrameWidth = 1.0f / static_cast<float>(kArrowFrameCount);
	constexpr float kArrowFrameDuration = 0.06f;
	const float animLength = kArrowFrameDuration * static_cast<float>(kArrowFrameCount);
	const float wrappedTime = std::fmod(clickIndicatorAnimTime_, animLength);
	const int frameIndex = std::min(kArrowFrameCount - 1, static_cast<int>(wrappedTime / kArrowFrameDuration));

	indicator->SetPosition(glm::vec3(clickIndicatorWorld_.x, clickIndicatorWorld_.y, 0.0f));
	indicator->SetScale(glm::vec3(96.0f * pulse));
	indicator->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, normalized));
	indicator->SetUVRect(glm::vec4(frameIndex * kFrameWidth, 0.0f, kFrameWidth, 1.0f));
}

/**
 * @brief Updates sprite direction.
 * @param direction Parameter for direction.
 * @param sprite Parameter for sprite.
 * @return Result produced by this operation.
 */
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

/**
 * @brief Samples input.
 * @param deltaTime Frame delta time in seconds.
 * @param inputManager Input manager for the current frame.
 * @param graphicsEngine Graphics engine used for rendering-related queries.
 * @return Result produced by this operation.
 */
void PlayerController::SampleInput(float deltaTime,
	InputManager& inputManager,
	GraphicsEngine& graphicsEngine) {
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

