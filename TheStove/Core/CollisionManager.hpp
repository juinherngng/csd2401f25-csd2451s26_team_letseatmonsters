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

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Scene;

// Forward declare GameObject to avoid circular dependency.
class CollisionManager : public CoreFramework::SystemInterface {
public:
	// Public interface methods.
	explicit CollisionManager(float cellSize = 100.0f);

	// SystemInterface implementation.
	void Initialize() override;
	void Update(float deltaTime) override;
	std::string GetName() override;

	// Set the EntityManager reference (must be called after construction).
	void SetEntityManager(EntityManager* entityMgr);

	// Main update method to rebuild broad-phase grid and update collision world as needed.
	void UpdateCollisions(EntityManager& entityManager);

	// Build static world geometry from authoring structs.
	void BuildWalls(const collision::WalkArea& walkArea,
		const collision::WoodVertical& wood,
		const collision::StageEndGateVertical& endGate);

	// Query grid for objects overlapping an AABB.
	std::vector<GameObject*> QueryNearby(const collision::AABB& queryBox) const;

	// Query grid for objects containing a point.
	std::vector<GameObject*> QueryPoint(const Math::Vector2D& point) const;

	// Access to the static collision world for trimming and other queries.
	collision::World& GetCollisionWorld() {
		return collisionWorld_;
	}
	const collision::World& GetCollisionWorld() const {
		return collisionWorld_;
	}

	// Access to the spatial grid for direct updates or queries.
	SpatialGrid& GetSpatialGrid() {
		return spatialGrid_;
	}
	const SpatialGrid& GetSpatialGrid() const {
		return spatialGrid_;
	}

	// Add static rectangles to the collision world.
	void AddStaticRects(const std::vector<collision::AABB>& rects);

	// Set the current scene reference.
	void SetScene(Scene* scene) {
		scene_ = scene;
	}
	void MarkStaticStateDirty() {
		staticStateDirty_ = true;
	}

	// Clear both the grid and the world geometry.
	void Clear();

	// Profiling counters for performance analysis.
	struct ProfileCounters {
		std::uint64_t updateCalls = 0;
		std::uint64_t earlyOutNoGridChange = 0;
		std::uint64_t fullRebuilds = 0;
		std::uint64_t objectsVisited = 0;
		std::uint64_t dirtyObjectsProcessed = 0;
		std::uint64_t narrowPhaseCollisions = 0;
	};

	// Access profiling counters for performance analysis.
	const ProfileCounters& GetProfileCounters() const {
		return profile_;
	}
	void ResetProfileCounters() {
		profile_ = ProfileCounters{};
	}

private:
	// Internal struct to track broad-phase relevant state for each object and detect changes.
	struct ObjectBroadphaseState {
		int objectID = -1;
		glm::vec3 pos{ 0.0f, 0.0f, 0.0f };
		glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
		std::string layerName;
		bool collidable = true;
		bool visible = true;
		bool enabled = true;
		bool dynamic = false;
		bool broadphaseDirty = true;
	};

	// Internal methods for broad-phase management and change detection.
	bool ShouldRebuildGrid(const std::vector<std::unique_ptr<GameObject>>& allObjects);
	ObjectBroadphaseState BuildBroadphaseState(const GameObject* obj) const;
	bool IsDynamicObject(const GameObject* obj) const;

	// Internal helper methods and state.
	EntityManager* entityManager_ = nullptr;	// Reference to EntityManager (set externally)
	SpatialGrid spatialGrid_;					// Broad-phase acceleration structure
	collision::World collisionWorld_;			// Static world used for trimming

	// Cached broad-phase state for all objects to detect changes and minimize rebuilds
	std::vector<ObjectBroadphaseState> broadphaseStateCache_;
	std::vector<int> dirtyObjectIDs_;
	std::unordered_set<int> dirtyObjectLookupCache_;
	bool forceFullRebuild_ = true;
	bool gridBuilt_ = false;
	bool staticStateDirty_ = true;

	// Reference to the current scene.
	Scene* scene_ = nullptr;
	mutable ProfileCounters profile_;
};
