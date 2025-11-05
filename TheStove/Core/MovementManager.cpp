#include "MovementManager.hpp"
#include "../Graphics/EntityManager.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Core/InputManager.hpp"
#include <iostream>
#include <cmath>

// ===== Core Functionality =====

void MovementManager::Update(float deltaTime, EntityManager& entityManager, InputManager& inputManager) {
    // Update player movement (WASD + click-to-move)
    if (playerID_ >= 0) {
        UpdatePlayerMovement(deltaTime, entityManager, inputManager);
        UpdateSpriteDirection(playerID_, entityManager);
    }

    // Update all objects with click-to-move or patrol
    for (auto& [objID, data] : movementData_) {
        if (objID == playerID_) continue;  // Already handled

        if (data.patrolEnabled) {
            UpdateNPCPatrol(objID, data, deltaTime, entityManager);
        }
        else if (data.hasTarget) {
            UpdateClickToMove(objID, data, deltaTime, entityManager);
        }
    }
}

void MovementManager::Clear() {
    movementData_.clear();
    playerID_ = -1;
}

// ===== Player Setup =====

void MovementManager::SetPlayerID(int playerID) {
    playerID_ = playerID;

    // Initialize player movement data if not exists
    if (movementData_.find(playerID) == movementData_.end()) {
        movementData_[playerID].moveSpeed = 200.0f;
    }
}

// ===== Movement Control =====

void MovementManager::SetMoveSpeed(int objectID, float speed) {
    movementData_[objectID].moveSpeed = speed;
}

float MovementManager::GetMoveSpeed(int objectID) const {
    auto it = movementData_.find(objectID);
    return (it != movementData_.end()) ? it->second.moveSpeed : 200.0f;
}

void MovementManager::SetMoveTarget(int objectID, const glm::vec2& target) {
    std::cout << "SetMoveTarget called for objectID: " << objectID << " to (" << target.x << ", " << target.y << ")" << std::endl;  

    movementData_[objectID].hasTarget = true;
    movementData_[objectID].moveTarget = target;

    std::cout << "After setting: hasTarget = " << movementData_[objectID].hasTarget << std::endl; 
}


void MovementManager::ClearMoveTarget(int objectID) {
    auto it = movementData_.find(objectID);
    if (it != movementData_.end()) {
        it->second.hasTarget = false;
        it->second.velocity = { 0.f, 0.f };
    }
}

bool MovementManager::HasMoveTarget(int objectID) const {
    auto it = movementData_.find(objectID);
    return (it != movementData_.end()) && it->second.hasTarget;
}

glm::vec2 MovementManager::GetMoveTarget(int objectID) const {
    auto it = movementData_.find(objectID);
    return (it != movementData_.end()) ? it->second.moveTarget : glm::vec2(0.f);
}

// ===== NPC Patrol =====

void MovementManager::SetPatrolPath(int objectID, const std::vector<glm::vec2>& waypoints, bool loop) {
    auto& data = movementData_[objectID];
    data.patrolPath = waypoints;
    data.patrolLoop = loop;
    data.currentWaypoint = 0;
}

void MovementManager::EnablePatrol(int objectID, bool enable) {
    movementData_[objectID].patrolEnabled = enable;
}

// ===== Query =====

bool MovementManager::IsMoving(int objectID) const {
    auto it = movementData_.find(objectID);
    if (it == movementData_.end()) return false;

    const glm::vec2& vel = it->second.velocity;
    return (vel.x != 0.f || vel.y != 0.f);
}

glm::vec2 MovementManager::GetVelocity(int objectID) const {
    auto it = movementData_.find(objectID);
    return (it != movementData_.end()) ? it->second.velocity : glm::vec2(0.f);
}

// ===== Private Helper Methods =====

void MovementManager::UpdatePlayerMovement(float deltaTime, EntityManager& entityManager, InputManager& inputManager) {
    GameObject* player = entityManager.GetByID(playerID_);
    if (!player) return;

    auto& data = movementData_[playerID_];
    //std::cout << "Player found. hasTarget: " << data.hasTarget << std::endl;

    // Priority 1: WASD movement (cancels click-to-move)
    glm::vec2 desiredMove(0.f, 0.f);
    bool pressingWASD = false;
    
    if (inputManager.IsKeyPressed(GLFW_KEY_W)) { desiredMove.y = -1.f; pressingWASD = true; }
    if (inputManager.IsKeyPressed(GLFW_KEY_S)) { desiredMove.y = 1.f; pressingWASD = true; }
    if (inputManager.IsKeyPressed(GLFW_KEY_A)) { desiredMove.x = -1.f; pressingWASD = true; }
    if (inputManager.IsKeyPressed(GLFW_KEY_D)) { desiredMove.x = 1.f; pressingWASD = true; }

    //std::cout << "pressingWASD: " << pressingWASD << std::endl;

    if (pressingWASD) {
        //std::cout << "WASD pressed - cancelling click-to-move" << std::endl;
        // WASD pressed - cancel click-to-move and use keyboard input
        data.hasTarget = false;

        // Normalize diagonal movement
        float length = std::sqrt(desiredMove.x * desiredMove.x + desiredMove.y * desiredMove.y);
        if (length > 0.f) {
            desiredMove /= length;
        }

        data.velocity = desiredMove * data.moveSpeed;
    }
    else if (data.hasTarget) {
        std::cout << "Processing click-to-move to (" << data.moveTarget.x << ", " << data.moveTarget.y << ")" << std::endl;
        // Priority 2: Click-to-move (only if not pressing WASD)
        glm::vec3 pos3D = player->GetPositionGLM();
        glm::vec2 pos(pos3D.x, pos3D.y);

        // Calculate direction to target
        glm::vec2 toTarget = data.moveTarget - pos;
        float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

        // Arrival threshold
        if (distance < 5.0f) {
            data.hasTarget = false;
            data.velocity = { 0.f, 0.f };
        }
        else {
            // Move towards target
            glm::vec2 direction = toTarget / distance;
            data.velocity = direction * data.moveSpeed;
        }
    }
    else {
        //std::cout << "No movement - velocity set to 0" << std::endl;
        // Priority 3: No input - stop moving
        data.velocity = { 0.f, 0.f };
    }

    // Apply velocity to position
    glm::vec3 pos = player->GetPositionGLM();
    pos.x += data.velocity.x * deltaTime;
    pos.y += data.velocity.y * deltaTime;
    player->SetPosition(pos);
}


void MovementManager::UpdateClickToMove(int objectID, MovementData& data, float deltaTime, EntityManager& entityManager) {
    GameObject* obj = entityManager.GetByID(objectID);
    if (!obj || !data.hasTarget) return;

    glm::vec3 pos3D = obj->GetPositionGLM();
    glm::vec2 pos(pos3D.x, pos3D.y);

    // Calculate direction to target
    glm::vec2 toTarget = data.moveTarget - pos;
    float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

    // Arrival threshold
    if (distance < 5.0f) {
        data.hasTarget = false;
        data.velocity = { 0.f, 0.f };
        return;
    }

    // Move towards target
    glm::vec2 direction = toTarget / distance;
    data.velocity = direction * data.moveSpeed;

    pos3D.x += data.velocity.x * deltaTime;
    pos3D.y += data.velocity.y * deltaTime;
    obj->SetPosition(pos3D);
}

void MovementManager::UpdateNPCPatrol(int objectID, MovementData& data, float deltaTime, EntityManager& entityManager) {
    if (data.patrolPath.empty()) return;

    GameObject* obj = entityManager.GetByID(objectID);
    if (!obj) return;

    glm::vec3 pos3D = obj->GetPositionGLM();
    glm::vec2 pos(pos3D.x, pos3D.y);

    // Get current waypoint
    glm::vec2 waypoint = data.patrolPath[data.currentWaypoint];
    glm::vec2 toWaypoint = waypoint - pos;
    float distance = std::sqrt(toWaypoint.x * toWaypoint.x + toWaypoint.y * toWaypoint.y);

    // Reached waypoint?
    if (distance < 10.0f) {
        data.currentWaypoint++;

        // End of path?
        if (data.currentWaypoint >= data.patrolPath.size()) {
            if (data.patrolLoop) {
                data.currentWaypoint = 0;  // Loop back
            }
            else {
                data.patrolEnabled = false;  // Stop
                data.velocity = { 0.f, 0.f };
                return;
            }
        }

        // Get next waypoint
        waypoint = data.patrolPath[data.currentWaypoint];
        toWaypoint = waypoint - pos;
        distance = std::sqrt(toWaypoint.x * toWaypoint.x + toWaypoint.y * toWaypoint.y);
    }

    // Move towards waypoint
    glm::vec2 direction = toWaypoint / distance;
    data.velocity = direction * data.moveSpeed;

    pos3D.x += data.velocity.x * deltaTime;
    pos3D.y += data.velocity.y * deltaTime;
    obj->SetPosition(pos3D);
}

void MovementManager::UpdateSpriteDirection(int entityID, EntityManager& entityManager) {
    auto it = movementData_.find(entityID);
    if (it == movementData_.end()) return;

    GameObject* sprite = entityManager.GetByID(entityID);
    if (!sprite) return;

    const MovementData& data = it->second;  

    // Only update texture if the entity is actually moving
    const glm::vec2& velocity = data.velocity;
    float magnitude = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

    if (magnitude < 0.001f) return; // Not moving enough to determine direction

    float ax = std::abs(velocity.x);
    float ay = std::abs(velocity.y);

    if (ax > ay) {
        // Horizontal movement 
        if (velocity.x > 0.0f) {
            sprite->SetTexture(ResourceManager::Instance().LoadTexture(
                "../assets/mc_sprite_right.png", "../assets/mc_sprite_right.png"));
        }
        else {
            sprite->SetTexture(ResourceManager::Instance().LoadTexture(
                "../assets/mc_sprite_left.png", "../assets/mc_sprite_left.png"));
        }
    }
    else {
        // Vertical movement 
        if (velocity.y > 0.0f) {
            sprite->SetTexture(ResourceManager::Instance().LoadTexture(
                "../assets/mc_sprite_front.png", "../assets/mc_sprite_front.png"));
        }
        else {
            sprite->SetTexture(ResourceManager::Instance().LoadTexture(
                "../assets/mc_sprite_back.png", "../assets/mc_sprite_back.png"));
        }
    }
}

