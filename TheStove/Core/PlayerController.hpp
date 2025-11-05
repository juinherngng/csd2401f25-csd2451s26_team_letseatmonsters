#pragma once

#include "../Graphics/EntityManager.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "InputManager.hpp"
#include "MovementManager.hpp"
#include "PhysicsManager.hpp"
#include "Math.hpp"
#include <glm/glm.hpp>

/**
 * @brief Manages player-specific logic and input
 *
 * Responsibilities:
 * - Process player input (clicks, scale, rotation)
 * - Update player sprite direction based on movement
 * - Coordinate between kinematic and physics movement
 */
class PlayerController {
public:
    PlayerController() = default;

    // Handle player input and commands
    void HandleInput(float deltaTime,
        InputManager& inputManager,
        EntityManager& entityManager,
        MovementManager& movementManager,
        PhysicsManager& physicsManager,
        GraphicsEngine& graphicsEngine,
        int playerID,
        bool useForces);

    // Update player sprite texture based on movement direction
    void UpdateSpriteDirection(const glm::vec2& direction, GameObject* sprite);

    float GetRotation() const { return rotation_; }

private:
    void HandleScaleInput(InputManager& inputManager, GameObject* sprite, float deltaTime);
    void HandleRotationInput(InputManager& inputManager, float deltaTime);
    void HandleClickToMove(InputManager& inputManager,
        EntityManager& entityManager,
        MovementManager& movementManager,
        PhysicsManager& physicsManager,
        GraphicsEngine& graphicsEngine,
        int playerID,
        bool useForces);

    float rotation_ = 0.0f;
};
