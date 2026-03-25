/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CollisionManager.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu (15%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(75%)
					Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:       Declares CollisionManager. Owns a static collision::World and a dynamic
					spatial grid of GameObjects, rebuilds broad-phase data every frame, and
					exposes helper queries for nearby/point lookups used by gameplay code.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
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

	/**
	 * @brief Constructs a `CollisionManager` instance.
	 * @param cellSize Parameter for cell size.
	 * @return Result produced by this operation.
	 */
	explicit CollisionManager(float cellSize = 100.0f);

	/**
	 * @brief Initializes this object.
	 */
	void Initialize() override;

	/**
	 * @brief Updates this object.
	 * @param deltaTime Frame delta time in seconds.
	 */
	void Update(float deltaTime) override;

	/**
	 * @brief Returns the stable name for this object.
	 * @return Requested value.
	 */
	std::string GetName() override;

	/**
	 * @brief Sets entity manager.
	 * @param entityMgr Parameter for entity mgr.
	 */
	void SetEntityManager(EntityManager* entityMgr);

	/**
	 * @brief Updates collisions.
	 * @param entityManager Entity manager containing the active objects.
	 */
	void UpdateCollisions(EntityManager& entityManager);

	/**
	 * @brief Builds walls.
	 * @param walkArea Parameter for walk area.
	 * @param wood Parameter for wood.
	 * @param endGate Parameter for end gate.
	 */
	void BuildWalls(const collision::WalkArea& walkArea,
		const collision::WoodVertical& wood,
		const collision::StageEndGateVertical& endGate);

	/**
	 * @brief Performs query nearby.
	 * @param queryBox Parameter for query box.
	 * @return Result produced by this operation.
	 */
	std::vector<GameObject*> QueryNearby(const collision::AABB& queryBox) const;

	/**
	 * @brief Performs query point.
	 * @param point Parameter for point.
	 * @return Result produced by this operation.
	 */
	std::vector<GameObject*> QueryPoint(const Math::Vector2D& point) const;

	/**
	 * @brief Returns collision world.
	 * @return Requested value.
	 */
	collision::World& GetCollisionWorld() {
		return collisionWorld_;
	}

	/**
	 * @brief Returns collision world.
	 * @return Requested value.
	 */
	const collision::World& GetCollisionWorld() const {
		return collisionWorld_;
	}

	/**
	 * @brief Returns spatial grid.
	 * @return Requested value.
	 */
	SpatialGrid& GetSpatialGrid() {
		return spatialGrid_;
	}

	/**
	 * @brief Returns spatial grid.
	 * @return Requested value.
	 */
	const SpatialGrid& GetSpatialGrid() const {
		return spatialGrid_;
	}

	/**
	 * @brief Adds static rects.
	 * @param rects Parameter for rects.
	 */
	void AddStaticRects(const std::vector<collision::AABB>& rects);

	/**
	 * @brief Sets scene.
	 * @param scene Scene being processed.
	 */
	void SetScene(Scene* scene) {
		scene_ = scene;
	}

	/**
	 * @brief Performs mark static state dirty.
	 */
	void MarkStaticStateDirty() {
		staticStateDirty_ = true;
	}

	/**
	 * @brief Clears this object.
	 */
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

	/**
	 * @brief Returns profile counters.
	 * @return Requested value.
	 */
	const ProfileCounters& GetProfileCounters() const {
		return profile_;
	}

	/**
	 * @brief Resets profile counters.
	 */
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

	/**
	 * @brief Returns whether rebuild grid.
	 * @param allObjects Parameter for all objects.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool ShouldRebuildGrid(const std::vector<std::unique_ptr<GameObject>>& allObjects);

	/**
	 * @brief Builds broadphase state.
	 * @param obj Parameter for obj.
	 * @return Result produced by this operation.
	 */
	ObjectBroadphaseState BuildBroadphaseState(const GameObject* obj) const;

	/**
	 * @brief Returns whether dynamic object.
	 * @param obj Parameter for obj.
	 * @return True when the operation succeeds or the condition is met.
	 */
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

