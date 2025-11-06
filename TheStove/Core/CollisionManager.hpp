#pragma once

#include "SpatialGrid.hpp"
#include "Collision.hpp"
#include "Math.hpp"
#include "../Graphics/EntityManager.hpp"
#include <vector>
#include <unordered_map>

/**
 * @class CollisionManager
 * @brief Centralized collision detection using SpatialGrid + collision::World
 *
 * Responsibilities:
 * - Maintain spatial grid for broad-phase queries
 * - Provide collision::World for player/NPC vs walls
 * - Encapsulate all collision logic previously in Scene
 */
class CollisionManager {
public:
	// Constructor
	explicit CollisionManager(float cellSize = 100.0f);

	// Core lifecycle
	void Update(EntityManager& entityManager);
	void Clear();

	// Spatial grid access (for queries in Scene or other systems)
	SpatialGrid& GetSpatialGrid() { return spatialGrid_; }
	const SpatialGrid& GetSpatialGrid() const { return spatialGrid_; }

	// Collision world access (for wall collision resolution)
	collision::World& GetCollisionWorld() { return collisionWorld_; }
	const collision::World& GetCollisionWorld() const { return collisionWorld_; }

	// Build static walls from level geometry
	void BuildWalls(const collision::WalkArea& walkArea,
		const collision::WoodVertical& wood,
		const collision::StageEndGateVertical& endGate);

	// Query helpers
	std::vector<GameObject*> QueryNearby(const collision::AABB& queryBox) const;
	std::vector<GameObject*> QueryPoint(const Math::Vector2D& point) const;

	const collision::World& GetWorld() const { return collisionWorld_; }

private:
	SpatialGrid spatialGrid_;
	collision::World collisionWorld_;
};
