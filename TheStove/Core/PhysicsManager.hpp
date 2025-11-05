#pragma once

#include "Physics.hpp"
#include "CollisionManager.hpp"
#include "InputManager.hpp"
#include "Math.hpp"
#include "Forces.hpp"
#include "../Graphics/EntityManager.hpp"
#include <unordered_map>
#include <memory>

/**
 * @brief Lightweight force-based physics for entities
 *
 * Manages per-entity physics state (velocity, mass) and force generators
 * without requiring RigidBody2D components.
 */
class PhysicsManager {
public:
    PhysicsManager() = default;
    ~PhysicsManager() = default;

    // Core update
    void Update(float deltaTime,
        EntityManager& entityManager,
        InputManager& inputManager);

    // Entity physics setup
    void EnablePhysics(int entityID, float mass = 1.0f);
    void DisablePhysics(int entityID);
    bool HasPhysics(int entityID) const;

    // Force-based movement control
    void SetSeekTarget(int entityID, const Math::Vector2D& target);
    void ClearSeekTarget(int entityID);

    // Access to step controller
    physics::StepController& GetStepController() { return physicsStep_; }

    void Clear();

private:
    struct PhysicsState {
        Math::Vector2D velocity{ 0.0f, 0.0f };
        Math::Vector2D forceAccum{ 0.0f, 0.0f };
        float invMass = 1.0f;
        float damping = 0.98f;
    };

    physics::StepController physicsStep_;

    // Per-entity physics state
    std::unordered_map<int, PhysicsState> physicsStates_;

    // Per-entity seek targets
    std::unordered_map<int, Math::Vector2D> seekTargets_;

    // Force generators (shared across all entities)
    DragForce dragForce_{ 0.9f, 0.1f };  // Air resistance

    // Constants
    static constexpr float SEEK_MAX_ACCEL = 600.0f;
    static constexpr float ARRIVE_RADIUS = 10.0f;

    // Helpers
    void IntegrateEntity(int entityID, float dt, EntityManager& entityManager);
};
