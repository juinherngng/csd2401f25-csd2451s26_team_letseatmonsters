/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GraphicsEngine.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(60%)

 DESCRIPTION:		Implements the DebugVisualizer static helper used to draw
					collider rectangles, player path lines, spatial grid outlines,
					and nearby collision candidates.

		All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "DebugVisualizer.hpp"

#include <cmath>

bool DebugVisualizer::sDrawPathLine = true;

/**
 * @brief Draws debug info.
 * @param entityManager Entity manager containing the active objects.
 * @param collisionManager Collision manager used for collision queries.
 * @param movementManager Movement manager used for movement updates.
 * @param playerID Identifier of the player object.
 * @param showAuxiliary Parameter for show auxiliary.
 * @return Result produced by this operation.
 */
void DebugVisualizer::DrawDebugInfo(EntityManager& entityManager,
	CollisionManager& collisionManager,
	MovementManager& movementManager,
	int playerID,
	bool showAuxiliary) {
	// Guard: global debug toggle
	if (!DebugRenderer::IsEnabled()) {
		return;
	}

	// Draw all colliders every frame
	DrawAllColliders(entityManager.GetObjectStorage());

	// Player-focused overlays (path, grid cells, nearby candidates)
	if (showAuxiliary && playerID >= 0) {
		GameObject* player = entityManager.GetByID(playerID);
		if (player) {
			DrawPlayerDebug(player, movementManager, playerID);
			DrawSpatialGrid(player, collisionManager);
			DrawCandidates(player, collisionManager);
		}
	}
}

/**
 * @brief Draws all colliders.
 * @param objects Parameter for objects.
 * @return Result produced by this operation.
 */
void DebugVisualizer::DrawAllColliders(const std::vector<std::unique_ptr<GameObject>>& objects) {
	for (const auto& objPtr : objects) {
		GameObject* obj = objPtr.get();
		if (obj == nullptr) {
			continue;
		}

		const auto colliderSize = obj->GetColliderSize();
		if (colliderSize.x <= 0.0f || colliderSize.y <= 0.0f) {
			continue;
		}

		const auto colliderOffset = obj->GetColliderOffset();
		const glm::vec3 worldPos = obj->GetPositionGLM();

		const float halfX = colliderSize.x * 0.5f;
		const float halfY = colliderSize.y * 0.5f;

		const glm::vec3 minCorner(worldPos.x + colliderOffset.x - halfX,
			worldPos.y + colliderOffset.y - halfY,
			0.0f);
		const glm::vec3 maxCorner(worldPos.x + colliderOffset.x + halfX,
			worldPos.y + colliderOffset.y + halfY,
			0.0f);

		// Red rectangle for all colliders
		DebugRenderer::DrawRect(minCorner, maxCorner, { 1.0f, 0.0f, 0.0f });
	}
}

/**
 * @brief Sets draw path line.
 * @param enable Boolean flag controlling whether the feature is enabled.
 * @return Result produced by this operation.
 */
void DebugVisualizer::SetDrawPathLine(bool enable) {
	sDrawPathLine = enable;
}

/**
 * @brief Draws player debug.
 * @param player Parameter for player.
 * @param movementManager Movement manager used for movement updates.
 * @param playerID Identifier of the player object.
 * @return Result produced by this operation.
 */
void DebugVisualizer::DrawPlayerDebug(GameObject* player,
	MovementManager& movementManager,
	int playerID) {
	const glm::vec3 pos = player->GetPositionGLM();

	// Draw path line from player to current target (only when enabled)
	if (sDrawPathLine && movementManager.HasMoveTarget(playerID)) {
		glm::vec2 target = movementManager.GetMoveTarget(playerID);
		DebugRenderer::DrawLine(
			glm::vec3(pos.x, pos.y, 0.0f),
			glm::vec3(target.x, target.y, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);
	}

	// Player collider corners + center
	const auto playerSize = player->GetColliderSize();
	const auto playerOffset = player->GetColliderOffset();

	if (playerSize.x > 0.0f && playerSize.y > 0.0f) {
		const glm::vec3 center(pos.x + playerOffset.x, pos.y + playerOffset.y, 0.0f);
		const glm::vec3 minCorner(center.x - playerSize.x * 0.5f,
			center.y - playerSize.y * 0.5f,
			0.0f);
		const glm::vec3 maxCorner(center.x + playerSize.x * 0.5f,
			center.y + playerSize.y * 0.5f,
			0.0f);

		// Yellow corners
		DebugRenderer::DrawPoint({ minCorner.x, maxCorner.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f);
		DebugRenderer::DrawPoint({ maxCorner.x, maxCorner.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f);
		DebugRenderer::DrawPoint({ maxCorner.x, minCorner.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f);
		DebugRenderer::DrawPoint({ minCorner.x, minCorner.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f);

		// Red center
		DebugRenderer::DrawPoint(center, { 1.0f, 0.0f, 0.0f }, 7.0f);
	}
}

/**
 * @brief Draws spatial grid.
 * @param player Parameter for player.
 * @param collisionManager Collision manager used for collision queries.
 * @return Result produced by this operation.
 */
void DebugVisualizer::DrawSpatialGrid(GameObject* player, CollisionManager& collisionManager) {
	if (player == nullptr) {
		return;
	}

	const glm::vec3 pos = player->GetPositionGLM();
	const auto playerSize = player->GetColliderSize();
	const auto playerOffset = player->GetColliderOffset();

	if (playerSize.x <= 0.0f || playerSize.y <= 0.0f) {
		return;
	}

	// Build player AABB in engine math types
	const Math::Vector3D center(pos.x + playerOffset.x, pos.y + playerOffset.y, pos.z);
	const Math::Vector3D scale(playerSize.x, playerSize.y, 1.0f);
	const collision::AABB playerBox = collision::World::makeAABBFromCenter(center, scale);

	// Neighborhood bound in cell indices
	const float cellSize = collisionManager.GetSpatialGrid().CellSize();
	const int minCx = static_cast<int>(std::floor(playerBox.min.x / cellSize)) - 1;
	const int maxCx = static_cast<int>(std::floor(playerBox.max.x / cellSize)) + 1;
	const int minCy = static_cast<int>(std::floor(playerBox.min.y / cellSize)) - 1;
	const int maxCy = static_cast<int>(std::floor(playerBox.max.y / cellSize)) + 1;

	// Outline each cell in the neighborhood
	for (int cy = minCy; cy <= maxCy; ++cy) {
		for (int cx = minCx; cx <= maxCx; ++cx) {
			const float x0 = cx * cellSize;
			const float y0 = cy * cellSize;
			const float x1 = x0 + cellSize;
			const float y1 = y0 + cellSize;

			DebugRenderer::DrawLine({ x0, y0, 0.0f }, { x1, y0, 0.0f }, { 0.0f, 1.0f, 0.0f }); // bottom
			DebugRenderer::DrawLine({ x1, y0, 0.0f }, { x1, y1, 0.0f }, { 0.0f, 1.0f, 0.0f }); // right
			DebugRenderer::DrawLine({ x1, y1, 0.0f }, { x0, y1, 0.0f }, { 0.0f, 1.0f, 0.0f }); // top
			DebugRenderer::DrawLine({ x0, y1, 0.0f }, { x0, y0, 0.0f }, { 0.0f, 1.0f, 0.0f }); // left
		}
	}
}

/**
 * @brief Draws candidates.
 * @param player Parameter for player.
 * @param collisionManager Collision manager used for collision queries.
 * @return Result produced by this operation.
 */
void DebugVisualizer::DrawCandidates(GameObject* player, CollisionManager& collisionManager) {
	const glm::vec3 pos = player->GetPositionGLM();
	const auto playerSize = player->GetColliderSize();
	const auto playerOffset = player->GetColliderOffset();

	const collision::AABB playerBox = collision::World::makeAABBFromCenter(
		{ pos.x + playerOffset.x, pos.y + playerOffset.y, pos.z },
		{ playerSize.x, playerSize.y, 1.0f }
	);

	std::vector<GameObject*> candidates;
	collisionManager.GetSpatialGrid().Query(playerBox, candidates);

	for (GameObject* obj : candidates) {
		if (obj == nullptr || obj == player) {
			continue;
		}

		const auto colliderSize = obj->GetColliderSize();
		const auto colliderOffset = obj->GetColliderOffset();
		if (colliderSize.x <= 0.0f || colliderSize.y <= 0.0f) {
			continue;
		}

		const glm::vec3 gp = obj->GetPositionGLM();
		const float halfX = colliderSize.x * 0.5f;
		const float halfY = colliderSize.y * 0.5f;

		const glm::vec3 minCorner(gp.x + colliderOffset.x - halfX,
			gp.y + colliderOffset.y - halfY,
			0.0f);
		const glm::vec3 maxCorner(gp.x + colliderOffset.x + halfX,
			gp.y + colliderOffset.y + halfY,
			0.0f);

		// Cyan rectangle for nearby colliders
		DebugRenderer::DrawRect(minCorner, maxCorner, { 0.0f, 1.0f, 1.0f });

		// Cyan center point
		DebugRenderer::DrawPoint({ gp.x + colliderOffset.x, gp.y + colliderOffset.y, 0.0f },
			{ 0.0f, 1.0f, 1.0f }, 5.0f);
	}
}
