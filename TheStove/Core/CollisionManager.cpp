/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CollisionManager.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(40%)
					Ng Juin Herng, juinherng.ng@digipen.edu (20%)

 DESCRIPTION:       Implements CollisionManager. Rebuilds a spatial grid of scene objects each frame,
					builds/owns world collision geometry, resolves step trimming, and exposes broad-
					phase queries for overlap/point tests.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Graphics/SceneManager.hpp"

#include "CollisionManager.hpp"

#include <unordered_set>

 // Constructor
CollisionManager::CollisionManager(float cellSize)
	: spatialGrid_(cellSize) {
}

// SystemInterface implementation
void CollisionManager::Initialize() {
	// No special initialization needed
}

void CollisionManager::Update(float deltaTime) {
	(void)deltaTime; // Suppress unused parameter warning

	// Update collisions using the EntityManager reference
	if (entityManager_) {
		UpdateCollisions(*entityManager_);
	}
}

std::string CollisionManager::GetName() {
	return "CollisionManager";
}

// Engine integration
void CollisionManager::SetEntityManager(EntityManager* entityMgr) {
	entityManager_ = entityMgr;
}

// Broad-phase state tracking
bool CollisionManager::IsDynamicObject(const GameObject* obj) const {
	if (!obj) {
		return false;
	}

	const Math::Vector2D velocity = obj->GetVelocity();
	const bool hasVelocity = (velocity.x != 0.0f || velocity.y != 0.0f);
	return obj->IsMovableByPhysics() || hasVelocity;
}

// Build the broad-phase state for an object, including position/scale and layer properties relevant to collision logic. Used for change detection to minimize grid rebuilds.
CollisionManager::ObjectBroadphaseState CollisionManager::BuildBroadphaseState(const GameObject* obj) const {
	ObjectBroadphaseState state;
	if (!obj) {
		return state;
	}

	state.objectID = obj->GetID();
	state.pos = obj->GetPositionGLM();
	state.scale = obj->GetScaleGLM();
	state.dynamic = IsDynamicObject(obj);

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

// Determine if we need to rebuild the spatial grid based on changes to objects' broad-phase state (position/scale/layer properties).
// If only a few objects changed, we can do a partial update instead of a full rebuild.
bool CollisionManager::ShouldRebuildGrid(const std::vector<std::unique_ptr<GameObject>>& allObjects) {
	std::vector<ObjectBroadphaseState> nextState;
	nextState.reserve(allObjects.size());

	for (const auto& objPtr : allObjects) {
		if (objPtr) {
			nextState.push_back(BuildBroadphaseState(objPtr.get()));
		}
	}

	dirtyObjectIDs_.clear();
	forceFullRebuild_ = !gridBuilt_;

	if (!forceFullRebuild_ && nextState.size() != broadphaseStateCache_.size()) {
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
				if (curr.pos != prev.pos || curr.scale != prev.scale) {
					dirtyObjectIDs_.push_back(curr.objectID);
				}
			}
			else if (staticStateDirty_) {
				if (curr.pos != prev.pos || curr.scale != prev.scale) {
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

// Per-frame rebuild
void CollisionManager::UpdateCollisions(EntityManager& entityManager) {
	// Get all objects as vector of pointers
	const auto& allObjects = entityManager.GetObjectStorage();

	if (!ShouldRebuildGrid(allObjects)) {
		return;
	}

	if (forceFullRebuild_) {
		spatialGrid_.Clear();
	}

	std::unordered_set<int> dirtySet;
	if (!forceFullRebuild_) {
		dirtySet.insert(dirtyObjectIDs_.begin(), dirtyObjectIDs_.end());
	}

	for (const auto& objPtr : allObjects) {
		GameObject* obj = objPtr.get();
		if (obj == nullptr) {
			continue;
		}

		if (!forceFullRebuild_ && dirtySet.find(obj->GetID()) == dirtySet.end()) {
			continue;
		}

		bool canCollide = true;
		if (scene_) {
			Layer* layer = scene_->GetObjectLayerPtr(obj->GetID());

			if (layer) {
				canCollide = layer->IsEnabled() && layer->IsVisible() && layer->IsCollidable();
			}
		}

		if (!canCollide) {
			spatialGrid_.Remove(obj);
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
	}
}

// Lifecycle
void CollisionManager::Clear() {
	spatialGrid_.Clear();
	collisionWorld_.clear();
	broadphaseStateCache_.clear();
	dirtyObjectIDs_.clear();
	forceFullRebuild_ = true;
	gridBuilt_ = false;
	staticStateDirty_ = true;
}

// World building
void CollisionManager::BuildWalls(const collision::WalkArea& walkArea,
	const collision::WoodVertical& wood,
	const collision::StageEndGateVertical& endGate) {
	collisionWorld_.build(walkArea, wood, endGate);
}

void CollisionManager::AddStaticRects(const std::vector<collision::AABB>& rects) {
	for (const auto& r : rects) {
		collisionWorld_.addWall(r);
	}
}

// Queries
std::vector<GameObject*> CollisionManager::QueryNearby(const collision::AABB& queryBox) const {
	std::vector<GameObject*> candidates;
	spatialGrid_.Query(queryBox, candidates);

	return candidates;
}

std::vector<GameObject*> CollisionManager::QueryPoint(const Math::Vector2D& point) const {
	std::vector<GameObject*> candidates;
	spatialGrid_.QueryPoint(point, candidates);

	return candidates;
}
