/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GraphicsEngine.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Seah Wang Hua, wanghua.seah@digipen.edu
CO-AUTHORS:			Yat Chun Wee, y.chunwee@digipen.edu

DESCRIPTION:		Declares the DebugVisualizer utility (static-only) used to render
					collision boxes, spatial grid cells, and pathing cues for selected
					GameObjects. Functions are grouped by purpose and kept lightweight
					so they can be called from the main debug pass each frame.

		All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <vector>

#include "CollisionManager.hpp"
#include "MovementManager.hpp"

#include "../Graphics/DebugRenderer.hpp"
#include "../Graphics/EntityManager.hpp"

/**
 * @class DebugVisualizer
 * @brief Static helper for drawing runtime debug visuals (colliders, grids, paths).
 */
class DebugVisualizer {
public:
	DebugVisualizer() = default;

	// Entry point to render all debug overlays for the current frame.
	static void DrawDebugInfo(EntityManager& entityManager,
		CollisionManager& collisionManager,
		MovementManager& movementManager,
		int playerId,
		bool showAuxiliary);

	// Draws AABBs for every object that has a valid collider size.
	static void DrawAllColliders(const std::vector<GameObject*>& objects);

	// Draws player-specific helpers (path line and collider corners/center).
	static void DrawPlayerDebug(GameObject* player, MovementManager& movementManager, int playerID);

	// Outlines spatial grid cells around the player's collider AABB.
	static void DrawSpatialGrid(GameObject* player, CollisionManager& collisionManager);

	// Highlights nearby collision candidates fetched from the spatial grid.
	static void DrawCandidates(GameObject* player, CollisionManager& collisionManager);

	// Allow other systems to enable/disable the click-to-move path line.
	static void SetDrawPathLine(bool enable);

private:
	// Internal toggle for the click-to-move line
	static bool sDrawPathLine;
};
