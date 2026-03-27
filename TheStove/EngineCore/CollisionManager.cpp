/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CollisionManager.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu (15%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(75%)
					Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:       Implements CollisionManager. Rebuilds a spatial grid of scene objects each frame,
					builds/owns world collision geometry, resolves step trimming, and exposes broad-
					phase queries for overlap/point tests.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <unordered_set>

#include "EngineCore/CollisionManager.hpp"
#include "EngineGraphics/SceneManager.hpp"

namespace {

	/**
	 * @brief Returns whether overlapsaabb.
	 * @param lhs Parameter for lhs.
	 * @param rhs Parameter for rhs.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool OverlapsAABB(const collision::AABB& lhs, const collision::AABB& rhs) {
		return !(lhs.max.x < rhs.min.x || lhs.min.x > rhs.max.x || lhs.max.y < rhs.min.y || lhs.min.y > rhs.max.y);
	}
}

/**
 * @brief Performs collision manager.
 * @param cellSize Parameter for cell size.
 * @return Result produced by this operation.
 */
CollisionManager::CollisionManager(float cellSize)
	: spatialGrid_(cellSize) {
}

/**
 * @brief Initializes this object.
 * @return Result produced by this operation.
 */
void CollisionManager::Initialize() {
	// No special initialization needed
}

/**
 * @brief Updates this object.
 * @param deltaTime Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void CollisionManager::Update(float deltaTime) {
	(void)deltaTime; // Suppress unused parameter warning

	// Update collisions using the EntityManager reference
	if (entityManager_) {
		UpdateCollisions(*entityManager_);
	}
}

/**
 * @brief Returns the stable name for this object.
 * @return Requested value.
 */
std::string CollisionManager::GetName() {
	return "CollisionManager";
}

/**
 * @brief Sets entity manager.
 * @param entityMgr Parameter for entity mgr.
 * @return Result produced by this operation.
 */
void CollisionManager::SetEntityManager(EntityManager* entityMgr) {
	entityManager_ = entityMgr;
}

/**
 * @brief Returns whether dynamic object.
 * @param obj Parameter for obj.
 * @return True when the operation succeeds or the condition is met.
 */
bool CollisionManager::IsDynamicObject(const GameObject* obj) const {
	if (!obj) {
		return false;
	}

	const Math::Vector2D velocity = obj->GetVelocity();
	const bool hasVelocity = (velocity.x != 0.0f || velocity.y != 0.0f);
	return obj->IsMovableByPhysics() || hasVelocity;
}

/**
 * @brief Builds broadphase state.
 * @param obj Parameter for obj.
 * @return Result produced by this operation.
 */
CollisionManager::ObjectBroadphaseState CollisionManager::BuildBroadphaseState(const GameObject* obj) const {
	ObjectBroadphaseState state;
	if (!obj) {
		return state;
	}

	state.objectID = obj->GetID();
	state.pos = obj->GetPositionGLM();
	state.scale = obj->GetScaleGLM();
	state.dynamic = IsDynamicObject(obj);
	state.broadphaseDirty = obj->IsBroadphaseDirty();

	if (scene_) {
		if (const Layer* layer = scene_->GetObjectLayerPtr(obj->GetID())) {
			state.layerName = layer->GetName();
			state.enabled = layer->IsEnabled();
			state.visible = layer->IsVisible();
			state.collidable = layer->IsCollidable();
		}
	}

	return state;
}

/**
 * @brief Returns whether rebuild grid.
 * @param allObjects Parameter for all objects.
 * @return True when the operation succeeds or the condition is met.
 */
bool CollisionManager::ShouldRebuildGrid(const std::vector<std::unique_ptr<GameObject>>& allObjects) {
	std::vector<ObjectBroadphaseState> nextState;
	nextState.reserve(allObjects.size());

	for (const auto& objPtr : allObjects) {
		++profile_.objectsVisited;
		if (objPtr) {
			nextState.push_back(BuildBroadphaseState(objPtr.get()));
		}
	}

	dirtyObjectIDs_.clear();
	forceFullRebuild_ = !gridBuilt_;

	if (!forceFullRebuild_ && nextState.size() != broadphaseStateCache_.size()) {
		// Object count changed (spawn/despawn), so positional diffs are no longer index-safe.
		forceFullRebuild_ = true;
		staticStateDirty_ = true;
	}

	if (!forceFullRebuild_) {
		for (size_t i = 0; i < nextState.size(); ++i) {
			const ObjectBroadphaseState& curr = nextState[i];
			const ObjectBroadphaseState& prev = broadphaseStateCache_[i];

			if (curr.objectID != prev.objectID) {
				forceFullRebuild_ = true;
				staticStateDirty_ = true;
				break;
			}

			const bool layerStateChanged =
				(curr.layerName != prev.layerName) ||
				(curr.enabled != prev.enabled) ||
				(curr.visible != prev.visible) ||
				(curr.collidable != prev.collidable);
			if (layerStateChanged || curr.dynamic != prev.dynamic) {
				dirtyObjectIDs_.push_back(curr.objectID);
				if (!curr.dynamic && !prev.dynamic) {
					staticStateDirty_ = true;
				}

				continue;
			}

			if (curr.dynamic) {
				if (curr.broadphaseDirty || curr.pos != prev.pos || curr.scale != prev.scale) {
					dirtyObjectIDs_.push_back(curr.objectID);
				}
			}
			else if (staticStateDirty_) {
				if (curr.broadphaseDirty || curr.pos != prev.pos || curr.scale != prev.scale) {
					dirtyObjectIDs_.push_back(curr.objectID);
				}
			}
		}
	}

	const bool changed = forceFullRebuild_ || !dirtyObjectIDs_.empty();
	if (changed) {
		broadphaseStateCache_ = std::move(nextState);
		gridBuilt_ = true;
		staticStateDirty_ = false;
	}

	return changed;
}

/**
 * @brief Updates collisions.
 * @param entityManager Entity manager containing the active objects.
 * @return Result produced by this operation.
 */
void CollisionManager::UpdateCollisions(EntityManager& entityManager) {
	++profile_.updateCalls;
	// Get all objects as vector of pointers
	const auto& allObjects = entityManager.GetObjectStorage();

	if (!ShouldRebuildGrid(allObjects)) {
		++profile_.earlyOutNoGridChange;
		return;
	}

	if (forceFullRebuild_) {
		++profile_.fullRebuilds;
		spatialGrid_.Clear();
	}

	dirtyObjectLookupCache_.clear();
	if (!forceFullRebuild_) {
		// Build a lookup cache of dirty object IDs for efficient per-object updates.
		dirtyObjectLookupCache_.reserve(dirtyObjectIDs_.size());
		dirtyObjectLookupCache_.insert(dirtyObjectIDs_.begin(), dirtyObjectIDs_.end());
	}

	for (const auto& objPtr : allObjects) {
		++profile_.objectsVisited;
		GameObject* obj = objPtr.get();
		if (obj == nullptr) {
			continue;
		}

		if (!forceFullRebuild_ && dirtyObjectLookupCache_.find(obj->GetID()) == dirtyObjectLookupCache_.end()) {
			continue;
		}

		++profile_.dirtyObjectsProcessed;

		bool canCollide = true;
		if (scene_) {
			Layer* layer = scene_->GetObjectLayerPtr(obj->GetID());

			if (layer) {
				canCollide = layer->IsEnabled() && layer->IsVisible() && layer->IsCollidable();
			}
		}

		if (!canCollide) {
			spatialGrid_.Remove(obj);
			obj->MarkBroadphaseClean();
			continue;
		}

		const Math::Vector3D pos(obj->GetPosition().x,
			obj->GetPosition().y,
			obj->GetPosition().z);
		const glm::vec3 scale = obj->GetScaleGLM();

		collision::AABB box = collision::World::makeAABBFromCenter(
			pos,
			Math::Vector3D(scale.x, scale.y, scale.z)
		);

		if (forceFullRebuild_) {
			spatialGrid_.Insert(obj, box);
		}
		else {
			spatialGrid_.Update(obj, box);
		}

		obj->MarkBroadphaseClean();
	}
}

/**
 * @brief Clears this object.
 * @return Result produced by this operation.
 */
void CollisionManager::Clear() {
	spatialGrid_.Clear();
	collisionWorld_.clear();
	broadphaseStateCache_.clear();
	dirtyObjectIDs_.clear();
	forceFullRebuild_ = true;
	gridBuilt_ = false;
	staticStateDirty_ = true;

}

/**
 * @brief Builds walls.
 * @param walkArea Parameter for walk area.
 * @param wood Parameter for wood.
 * @param endGate Parameter for end gate.
 * @return Result produced by this operation.
 */
void CollisionManager::BuildWalls(const collision::WalkArea& walkArea,
	const collision::WoodVertical& wood,
	const collision::StageEndGateVertical& endGate) {
	collisionWorld_.build(walkArea, wood, endGate);
}

/**
 * @brief Adds static rects.
 * @param rects Parameter for rects.
 * @return Result produced by this operation.
 */
void CollisionManager::AddStaticRects(const std::vector<collision::AABB>& rects) {
	for (const auto& r : rects) {
		collisionWorld_.addWall(r);
	}
}

/**
 * @brief Performs query nearby.
 * @param queryBox Parameter for query box.
 * @return Result produced by this operation.
 */
std::vector<GameObject*> CollisionManager::QueryNearby(const collision::AABB& queryBox) const {
	std::vector<GameObject*> candidates;
	spatialGrid_.Query(queryBox, candidates);

	std::vector<GameObject*> overlaps;
	overlaps.reserve(candidates.size());

	for (GameObject* obj : candidates) {
		if (!obj) {
			continue;
		}

		const Math::Vector3D pos(obj->GetPosition().x, obj->GetPosition().y, obj->GetPosition().z);
		const glm::vec3 scale = obj->GetScaleGLM();
		const collision::AABB candidate = collision::World::makeAABBFromCenter(
			pos, Math::Vector3D(scale.x, scale.y, scale.z));
		if (OverlapsAABB(candidate, queryBox)) {
			++profile_.narrowPhaseCollisions;
			overlaps.push_back(obj);
		}
	}

	return overlaps;
}

/**
 * @brief Performs query point.
 * @param point Parameter for point.
 * @return Result produced by this operation.
 */
std::vector<GameObject*> CollisionManager::QueryPoint(const Math::Vector2D& point) const {
	std::vector<GameObject*> candidates;
	spatialGrid_.QueryPoint(point, candidates);

	return candidates;
}
