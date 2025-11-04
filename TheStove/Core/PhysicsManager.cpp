#include "PhysicsManager.hpp"
#include <iostream>

void PhysicsManager::Update(float deltaTime,
    EntityManager& entityManager,
    CollisionManager& collisionManager,
    InputManager& inputManager,
    int playerID,
    const std::unordered_map<int, glm::vec2>& npcVelocities) {

    // Resolve physics timestep using StepController
    // This handles step-mode vs real-time automatically
    float physicsDt = physicsStep_.resolveDt(inputManager, deltaTime);

    // If no physics step this frame, skip
    if (physicsDt <= 0.0f) return;

    // Integrate NPC velocities
    IntegrateNPCVelocities(physicsDt, entityManager, npcVelocities);
}

void PhysicsManager::IntegrateNPCVelocities(float dt,
    EntityManager& entityManager,
    const std::unordered_map<int, glm::vec2>& npcVelocities) {

    for (const auto& [npcID, velocity] : npcVelocities) {
        GameObject* npc = entityManager.GetByID(npcID);
        if (!npc) continue;

        // Get current position
        Math::Vector3D pos = npc->GetPosition();

        // Simple Euler integration
        pos.x += velocity.x * dt;
        pos.y += velocity.y * dt;

        // Update position
        npc->SetPosition(pos);
    }
}