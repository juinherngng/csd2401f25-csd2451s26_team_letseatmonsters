#include "DebugVisualizer.hpp"

void DebugVisualizer::DrawDebugInfo(EntityManager& entityManager,
    CollisionManager& collisionManager,
    MovementManager& movementManager,
    int playerID,
    bool showAuxiliary) {
    if (!DebugRenderer::IsEnabled()) return;

    // Get all objects
    std::vector<GameObject*> allObjects = entityManager.GetAllObjects();

    // Always draw colliders
    DrawAllColliders(allObjects);

    // Draw auxiliary debug info if enabled
    if (showAuxiliary && playerID >= 0) {
        GameObject* player = entityManager.GetByID(playerID);
        if (player) {
            DrawPlayerDebug(player, movementManager, playerID);
            DrawSpatialGrid(player, collisionManager);
            DrawCandidates(player, collisionManager);
        }
    }
}

void DebugVisualizer::DrawAllColliders(const std::vector<GameObject*>& objects) {
    for (GameObject* obj : objects) {
        if (!obj) continue;

        const auto colSize = obj->GetColliderSize();
        if (colSize.x <= 0.0f || colSize.y <= 0.0f) continue;

        const auto colOff = obj->GetColliderOffset();
        const glm::vec3 pos = obj->GetPositionGLM();

        const float hx = colSize.x * 0.5f;
        const float hy = colSize.y * 0.5f;

        const glm::vec3 mn(pos.x + colOff.x - hx, pos.y + colOff.y - hy, 0.0f);
        const glm::vec3 mx(pos.x + colOff.x + hx, pos.y + colOff.y + hy, 0.0f);

        // Red rectangle for colliders
        DebugRenderer::DrawRect(mn, mx, { 1.0f, 0.0f, 0.0f });
    }
}

void DebugVisualizer::DrawPlayerDebug(GameObject* player,
    MovementManager& movementManager,
    int playerID) {
    const glm::vec3 pos = player->GetPositionGLM();

    // Draw path line to target
    if (movementManager.HasMoveTarget(playerID)) {
        glm::vec2 target = movementManager.GetMoveTarget(playerID);
        DebugRenderer::DrawLine(
            glm::vec3(pos.x, pos.y, 0.0f),
            glm::vec3(target.x, target.y, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }

    // Draw player collider corners
    const auto pSize = player->GetColliderSize();
    const auto pOff = player->GetColliderOffset();

    if (pSize.x > 0.0f && pSize.y > 0.0f) {
        const glm::vec3 c(pos.x + pOff.x, pos.y + pOff.y, 0.0f);
        const glm::vec3 mn(c.x - pSize.x * 0.5f, c.y - pSize.y * 0.5f, 0.0f);
        const glm::vec3 mx(c.x + pSize.x * 0.5f, c.y + pSize.y * 0.5f, 0.0f);

        // Yellow corner points
        DebugRenderer::DrawPoint({ mn.x, mn.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f);
        DebugRenderer::DrawPoint({ mx.x, mn.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f);
        DebugRenderer::DrawPoint({ mx.x, mx.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f);
        DebugRenderer::DrawPoint({ mn.x, mx.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f);

        // Red center point
        DebugRenderer::DrawPoint(c, { 1.0f, 0.0f, 0.0f }, 7.0f);
    }
}

void DebugVisualizer::DrawSpatialGrid(GameObject* player, CollisionManager& collisionManager) {
    if (!player) return;

    const glm::vec3 pos = player->GetPositionGLM();
    const auto pSize = player->GetColliderSize();
    const auto pOff = player->GetColliderOffset();

    if (pSize.x <= 0.0f || pSize.y <= 0.0f) return;

    // Build player AABB
    const Math::Vector3D pCenterM(pos.x + pOff.x, pos.y + pOff.y, pos.z);
    const Math::Vector3D pScaleM(pSize.x, pSize.y, 1.0f);
    const collision::AABB pBox = collision::World::makeAABBFromCenter(pCenterM, pScaleM);

    // Get cell size from spatial grid
    float cellSize = collisionManager.GetSpatialGrid().CellSize();

    // Calculate neighborhood bounds
    const int minCx = static_cast<int>(std::floor(pBox.min.x / cellSize)) - 1;
    const int maxCx = static_cast<int>(std::floor(pBox.max.x / cellSize)) + 1;
    const int minCy = static_cast<int>(std::floor(pBox.min.y / cellSize)) - 1;
    const int maxCy = static_cast<int>(std::floor(pBox.max.y / cellSize)) + 1;

    // Draw grid cells around player
    for (int cy = minCy; cy <= maxCy; ++cy) {
        for (int cx = minCx; cx <= maxCx; ++cx) {
            const float x0 = cx * cellSize;
            const float y0 = cy * cellSize;
            const float x1 = x0 + cellSize;
            const float y1 = y0 + cellSize;

            // Draw cell outline with green lines
            DebugRenderer::DrawLine({ x0, y0, 0 }, { x1, y0, 0 }, { 0, 1, 0 }); // bottom
            DebugRenderer::DrawLine({ x1, y0, 0 }, { x1, y1, 0 }, { 0, 1, 0 }); // right
            DebugRenderer::DrawLine({ x1, y1, 0 }, { x0, y1, 0 }, { 0, 1, 0 }); // top
            DebugRenderer::DrawLine({ x0, y1, 0 }, { x0, y0, 0 }, { 0, 1, 0 }); // left
        }
    }
}


void DebugVisualizer::DrawCandidates(GameObject* player, CollisionManager& collisionManager) {
    const glm::vec3 pos = player->GetPositionGLM();
    const auto pSize = player->GetColliderSize();
    const auto pOff = player->GetColliderOffset();

    collision::AABB pBox = collision::World::makeAABBFromCenter(
        { pos.x + pOff.x, pos.y + pOff.y, pos.z },
        { pSize.x, pSize.y, 1.0f }
    );

    std::vector<GameObject*> candidates;
    collisionManager.GetSpatialGrid().Query(pBox, candidates);

    for (GameObject* obj : candidates) {
        if (!obj || obj == player) continue;

        const auto gSize = obj->GetColliderSize();
        const auto gOff = obj->GetColliderOffset();
        if (gSize.x <= 0.0f || gSize.y <= 0.0f) continue;

        const glm::vec3 gp = obj->GetPositionGLM();
        const float hx = gSize.x * 0.5f;
        const float hy = gSize.y * 0.5f;

        const glm::vec3 mn(gp.x + gOff.x - hx, gp.y + gOff.y - hy, 0.0f);
        const glm::vec3 mx(gp.x + gOff.x + hx, gp.y + gOff.y + hy, 0.0f);

        // Cyan rectangle for nearby colliders
        DebugRenderer::DrawRect(mn, mx, { 0.0f, 1.0f, 1.0f });

        // Cyan center point
        DebugRenderer::DrawPoint({ gp.x + gOff.x, gp.y + gOff.y, 0.0f },
            { 0.0f, 1.0f, 1.0f }, 5.0f);
    }
}
