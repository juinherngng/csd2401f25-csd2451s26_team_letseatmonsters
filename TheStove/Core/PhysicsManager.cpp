#include "PhysicsManager.hpp"
#include <iostream>

void PhysicsManager::Update(float deltaTime,
    EntityManager& entityManager,
    InputManager& inputManager) {
    // Resolve physics timestep
    float physicsDt = physicsStep_.resolveDt(inputManager, deltaTime);
    if (physicsDt <= 0.0f) return;

    // Update all entities with physics enabled
    for (auto& [entityID, state] : physicsStates_) {
        IntegrateEntity(entityID, physicsDt, entityManager);
    }
}

void PhysicsManager::EnablePhysics(int entityID, float mass) {
    PhysicsState state;
    state.invMass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
    state.damping = 0.98f;
    state.velocity = { 0.0f, 0.0f };
    state.forceAccum = { 0.0f, 0.0f };

    physicsStates_[entityID] = state;
}

void PhysicsManager::DisablePhysics(int entityID) {
    physicsStates_.erase(entityID);
    seekTargets_.erase(entityID);
}

bool PhysicsManager::HasPhysics(int entityID) const {
    return physicsStates_.find(entityID) != physicsStates_.end();
}

void PhysicsManager::SetSeekTarget(int entityID, const Math::Vector2D& target) {
    if (!HasPhysics(entityID)) return;
    seekTargets_[entityID] = target;
}

void PhysicsManager::ClearSeekTarget(int entityID) {
    seekTargets_.erase(entityID);

    // Stop the entity
    auto it = physicsStates_.find(entityID);
    if (it != physicsStates_.end()) {
        it->second.velocity = { 0.0f, 0.0f };
    }
}

void PhysicsManager::IntegrateEntity(int entityID, float dt, EntityManager& entityManager) {
    GameObject* obj = entityManager.GetByID(entityID);
    if (!obj) return;

    auto& state = physicsStates_[entityID];

    // Clear forces from last frame
    state.forceAccum = { 0.0f, 0.0f };

    // Apply seek force if target is set
    auto seekIt = seekTargets_.find(entityID);
    if (seekIt != seekTargets_.end()) {
        Math::Vector3D pos3D = obj->GetPosition();
        Math::Vector2D currentPos(pos3D.x, pos3D.y);
        Math::Vector2D toTarget = seekIt->second - currentPos;
        float distance = toTarget.Length();

        // Arrive behavior - stop within radius
        if (distance <= ARRIVE_RADIUS) {
            state.velocity = { 0.0f, 0.0f };
            seekTargets_.erase(seekIt);
        }
        else if (distance > 1e-4f) {
            // Apply seek force
            Math::Vector2D direction = toTarget * (1.0f / distance);
            float mass = (state.invMass > 0.0f) ? (1.0f / state.invMass) : 1.0f;
            state.forceAccum = state.forceAccum + (direction * (SEEK_MAX_ACCEL * mass));
        }
    }

    // Apply drag force
    float speed = state.velocity.Length();
    if (speed > 1e-6f) {
        float dragMag = (dragForce_.k1 * speed) + (dragForce_.k2 * speed * speed);
        Math::Vector2D dragDir = state.velocity * (-1.0f / speed);
        state.forceAccum = state.forceAccum + (dragDir * dragMag);
    }

    // Integrate forces
    if (state.invMass > 0.0f) {
        // a = F * invMass
        Math::Vector2D accel = state.forceAccum * state.invMass;

        // v += a * dt
        state.velocity = state.velocity + (accel * dt);

        // Apply damping
        if (state.damping > 0.0f && state.damping < 1.0f) {
            float dampFactor = std::pow(state.damping, dt);
            state.velocity = state.velocity * dampFactor;
        }

        // Update position
        Math::Vector3D pos = obj->GetPosition();
        pos.x += state.velocity.x * dt;
        pos.y += state.velocity.y * dt;
        obj->SetPosition(pos);
    }
}

void PhysicsManager::Clear() {
    physicsStates_.clear();
    seekTargets_.clear();
}
