/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			DebugVisualizer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (30%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu		(70%)

 DESCRIPTION:		Declares the DebugVisualizer utility (static-only) used to render
					collision boxes, spatial grid cells, and pathing cues for selected
					GameObjects. Functions are grouped by purpose and kept lightweight
					so they can be called from the main debug pass each frame.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <vector>

#include "EngineCore/CollisionManager.hpp"
#include "EngineCore/MovementManager.hpp"
#include "EngineGraphics/DebugRenderer.hpp"
#include "EngineGraphics/EntityManager.hpp"

// Forward declare GameObject to avoid circular dependency.
class DebugVisualizer {
public:

	/**
	 * @brief Constructs a `DebugVisualizer` instance.
	 */
	DebugVisualizer() = default;

	/**
	 * @brief Draws debug info.
	 * @param entityManager Entity manager containing the active objects.
	 * @param collisionManager Collision manager used for collision queries.
	 * @param movementManager Movement manager used for movement updates.
	 * @param playerId Identifier of the player object.
	 * @param showAuxiliary Parameter for show auxiliary.
	 * @return Result produced by this operation.
	 */
	static void DrawDebugInfo(EntityManager& entityManager,
		CollisionManager& collisionManager,
		MovementManager& movementManager,
		int playerId,
		bool showAuxiliary);

	/**
	 * @brief Draws all colliders.
	 * @param objects Parameter for objects.
	 * @return Result produced by this operation.
	 */
	static void DrawAllColliders(const std::vector<std::unique_ptr<GameObject>>& objects);

	/**
	 * @brief Draws player debug.
	 * @param player Parameter for player.
	 * @param movementManager Movement manager used for movement updates.
	 * @param playerID Identifier of the player object.
	 * @return Result produced by this operation.
	 */
	static void DrawPlayerDebug(GameObject* player, MovementManager& movementManager, int playerID);

	/**
	 * @brief Draws spatial grid.
	 * @param player Parameter for player.
	 * @param collisionManager Collision manager used for collision queries.
	 * @return Result produced by this operation.
	 */
	static void DrawSpatialGrid(GameObject* player, CollisionManager& collisionManager);

	/**
	 * @brief Draws candidates.
	 * @param player Parameter for player.
	 * @param collisionManager Collision manager used for collision queries.
	 * @return Result produced by this operation.
	 */
	static void DrawCandidates(GameObject* player, CollisionManager& collisionManager);

	/**
	 * @brief Sets draw path line.
	 * @param enable Boolean flag controlling whether the feature is enabled.
	 * @return Result produced by this operation.
	 */
	static void SetDrawPathLine(bool enable);

private:
	// Internal toggle for the click-to-move line
	static bool sDrawPathLine;
};
