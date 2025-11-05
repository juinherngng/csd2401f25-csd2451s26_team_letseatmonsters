#pragma once

#include "../Graphics/EntityManager.hpp"
#include "../Graphics/DebugRenderer.hpp"
#include "CollisionManager.hpp"
#include "MovementManager.hpp"
#include <vector>

/**
 * @brief Handles all debug visualization
 *
 * Responsibilities:
 * - Draw collider boxes for all objects
 * - Draw player-specific debug info (corners, center)
 * - Draw movement paths and targets
 * - Draw spatial grid cells
 */
class DebugVisualizer {
public:
    DebugVisualizer() = default;

    // Draw all debug visuals
    void DrawDebugInfo(EntityManager& entityManager,
        CollisionManager& collisionManager,
        MovementManager& movementManager,
        int playerID,
        bool showAuxiliary);

private:
    void DrawAllColliders(const std::vector<GameObject*>& objects);
    void DrawPlayerDebug(GameObject* player, MovementManager& movementManager, int playerID);
    void DrawSpatialGrid(GameObject* player, CollisionManager& collisionManager);
    void DrawCandidates(GameObject* player, CollisionManager& collisionManager);
};

