/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CollisionManager.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:       Declares CollisionManager, which maintains a spatial grid of scene objects for
					broad-phase queries and owns the world collision geometry. Provides helpers to:
					- rebuild broad-phase per frame,
					- resolve intended movement (step trimming),
					- query nearby objects or a point,
					- build static walls/walk areas.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <vector>
#include <unordered_map>

#include "SpatialGrid.hpp"
#include "Collision.hpp"
#include "Math.hpp"

#include "../Graphics/EntityManager.hpp"

 /**
  * @class CollisionManager
  * @brief Broad-phase grid + world collision owner. Rebuilt every frame from EntityManager,
  *        supports movement trimming via resolve(), and simple spatial queries.
  */
class CollisionManager {
public:
	explicit CollisionManager(float cellSize = 100.0f);

	// Rebuild spatial grid from current objects each frame.
	void Update(EntityManager& entityManager);

	// Clear both the grid and the world geometry.
	void Clear();

	// Build static world geometry from authoring structs.
	void BuildWalls(const collision::WalkArea& walkArea,
		const collision::WoodVertical& wood,
		const collision::StageEndGateVertical& endGate);

	// Query grid for objects overlapping an AABB.
	std::vector<GameObject*> QueryNearby(const collision::AABB& queryBox) const;

	// Query grid for objects near a point.
	std::vector<GameObject*> QueryPoint(const Math::Vector2D& point) const;

	// Spatial grid access
	SpatialGrid& GetSpatialGrid() { return spatialGrid_; }
	const SpatialGrid& GetSpatialGrid() const { return spatialGrid_; }

	// Collision world access
	collision::World& GetCollisionWorld() { return collisionWorld_; }
	const collision::World& GetCollisionWorld() const { return collisionWorld_; }

private:
	SpatialGrid spatialGrid_;		  // Broad-phase acceleration structure
	collision::World collisionWorld_; // Static world used for trimming
};
