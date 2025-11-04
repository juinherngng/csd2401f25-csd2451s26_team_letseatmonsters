#pragma once

#include "Physics.hpp"
#include "CollisionManager.hpp"
#include "InputManager.hpp"
#include "Math.hpp"
#include "../Graphics/EntityManager.hpp"
#include <unordered_map>
#include <glm/glm.hpp>

/**
 * @class PhysicsManager
 * @brief Centralized physics simulation and collision response
 *
 * Responsibilities:
 * - Velocity integration for NPCs
 * - Lane-based movement with wall bouncing
 * - Player vs NPC collision separation
 * - Boundary clamping
 */
class PhysicsManager {
public:
    PhysicsManager() = default;
    ~PhysicsManager() = default;

    // Core update - integrate velocities and resolve collisions
    void Update(float deltaTime,
        EntityManager& entityManager,
        CollisionManager& collisionManager,
        InputManager& inputManager,
        int playerID,
        const std::unordered_map<int, glm::vec2>& npcVelocities);

    // Access to step controller (for debug/pause)
    physics::StepController& GetStepController() { return physicsStep_; }
    const physics::StepController& GetStepController() const { return physicsStep_; }

private:
    physics::StepController physicsStep_;

    // Helper methods
    void IntegrateNPCVelocities(float dt,
        EntityManager& entityManager,
        const std::unordered_map<int, glm::vec2>& npcVelocities);
};