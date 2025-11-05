#include "PlayerController.hpp"
#include <glm/gtc/constants.hpp>
#include <iostream>

void PlayerController::HandleInput(float deltaTime,
    InputManager& inputManager,
    EntityManager& entityManager,
    MovementManager& movementManager,
    PhysicsManager& physicsManager,
    GraphicsEngine& graphicsEngine,
    int playerID,
    bool useForces) {
    if (playerID < 0) return;

    GameObject* sprite = entityManager.GetByID(playerID);
    if (!sprite) return;

    // Handle scale controls
    HandleScaleInput(inputManager, sprite, deltaTime);

    // Handle rotation controls (if you have playerRotation stored somewhere)
    // HandleRotationInput(inputManager, playerRotation, deltaTime);

    // Handle click-to-move
    HandleClickToMove(inputManager, entityManager, movementManager,
        physicsManager, graphicsEngine, playerID, useForces);
}

void PlayerController::HandleScaleInput(InputManager& inputManager, GameObject* sprite, float deltaTime) {
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

void PlayerController::HandleClickToMove(InputManager& inputManager,
    EntityManager& entityManager,
    MovementManager& movementManager,
    PhysicsManager& physicsManager,
    GraphicsEngine& graphicsEngine,
    int playerID,
    bool useForces) {
    if (!inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        return;
    }

    GameObject* sprite = entityManager.GetByID(playerID);
    if (!sprite) return;

    glm::vec2 mouseWorld;
    if (!graphicsEngine.GetMouseWorldInScene(mouseWorld)) {
        return;
    }

    // Set movement target based on active mode
    if (useForces) {
        physicsManager.SetSeekTarget(playerID, Math::Vector2D(mouseWorld.x, mouseWorld.y));
    }
    else {
        movementManager.SetMoveTarget(playerID, mouseWorld);
    }

    // Update sprite direction
    glm::vec3 position = sprite->GetPositionGLM();
    glm::vec2 toTarget = mouseWorld - glm::vec2(position.x, position.y);
    UpdateSpriteDirection(toTarget, sprite);
}

void PlayerController::UpdateSpriteDirection(const glm::vec2& direction, GameObject* sprite) {
    if (glm::length(direction) <= 0.001f) return;

    float ax = std::abs(direction.x);
    float ay = std::abs(direction.y);

    if (ax > ay) {
        // Horizontal movement dominant
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
        // Vertical movement dominant
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
