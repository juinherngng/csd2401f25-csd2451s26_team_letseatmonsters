/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CollisionManager.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu (20%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(55%)
					Ng Juin Herng, juinherng.ng@digipen.edu (25%)

 DESCRIPTION:       Declares CollisionManager. Owns a static collision::World and a dynamic
					spatial grid of GameObjects, rebuilds broad-phase data every frame, and
					exposes helper queries for nearby/point lookups used by gameplay code.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "../Graphics/EntityManager.hpp"

#include "Collision.hpp"
#include "Math.hpp"
#include "SpatialGrid.hpp"
#include "System.hpp"

#include <unordered_map>
#include <vector>

class Scene;

/**
 * @class CollisionManager
 * @brief Broad-phase grid + world collision owner. Rebuilt every frame from EntityManager,
 *        supports movement trimming via resolve(), and simple spatial queries.
 */
class CollisionManager : public CoreFramework::SystemInterface {
public:
	// Public interface methods
	explicit CollisionManager(float cellSize = 100.0f);

	// SystemInterface implementation
	void Initialize() override;
	void Update(float deltaTime) override;
	std::string GetName() override;

	// Set the EntityManager reference (must be called after construction)
	void SetEntityManager(EntityManager* entityMgr);

	/**
	 * @brief Rebuild spatial grid from current objects each frame.
	 * @param entityManager Reference to entity manager with all game objects.
	 */
	void UpdateCollisions(EntityManager& entityManager);

	// Build static world geometry from authoring structs.
	void BuildWalls(const collision::WalkArea& walkArea,
		const collision::WoodVertical& wood,
		const collision::StageEndGateVertical& endGate);

	// Query grid for objects overlapping an AABB.
	std::vector<GameObject*> QueryNearby(const collision::AABB& queryBox) const;

	/**
	 * @brief Query grid for objects overlapping a point.
	 * @param point 2D point to query.
	 * @return Vector of GameObject pointers that contain the point.
	 */
	std::vector<GameObject*> QueryPoint(const Math::Vector2D& point) const;

	/**
	 * @brief Collision world access
	 */
	collision::World& GetCollisionWorld() {
		return collisionWorld_;
	}
	const collision::World& GetCollisionWorld() const {
		return collisionWorld_;
	}

	// Spatial grid access
	SpatialGrid& GetSpatialGrid() {
		return spatialGrid_;
	}
	const SpatialGrid& GetSpatialGrid() const {
		return spatialGrid_;
	}

	void AddStaticRects(const std::vector<collision::AABB>& rects);

	void SetScene(Scene* scene) {
		scene_ = scene;
	}

	// Clear both the grid and the world geometry.
	void Clear();

private:
	// Internal helper methods and state
	EntityManager* entityManager_ = nullptr;	// Reference to EntityManager (set externally)
	SpatialGrid spatialGrid_;					// Broad-phase acceleration structure
	collision::World collisionWorld_;			// Static world used for trimming

	Scene* scene_ = nullptr;
};
