#pragma once

#include "../Graphics/EntityManager.hpp"
#include "CollisionManager.hpp"
#include "Physics.hpp"
#include <unordered_map>
#include <glm/glm.hpp>

/**
 * @brief Manages NPC AI behavior and movement
 *
 * Responsibilities:
 * - Lane-based movement with wall bouncing
 * - NPC-to-NPC collision and elastic bouncing
 * - Generic velocity integration for NPCs
 */
class NPCSystem {
public:
    NPCSystem() = default;

    // Update all NPCs
    void Update(float deltaTime,
        EntityManager& entityManager,
        CollisionManager& collisionManager,
        const collision::WalkArea& walkArea);

    // NPC management
    void SetNPCVelocity(int npcID, const glm::vec2& velocity);
    glm::vec2 GetNPCVelocity(int npcID) const;
    void RegisterLaneNPC(int npcID, float laneX);
    void Clear();

private:
    std::unordered_map<int, glm::vec2> npcVelocities_;
    std::unordered_map<int, float> laneNPCs_; // NPC ID -> lane X position

    void UpdateLaneNPCs(float deltaTime, EntityManager& entityManager,
        CollisionManager& collisionManager,
        const collision::WalkArea& walkArea);
    void UpdateGenericNPCs(float deltaTime, EntityManager& entityManager,
        const collision::WalkArea& walkArea);
    void HandleNPCCollisions(EntityManager& entityManager);
};
