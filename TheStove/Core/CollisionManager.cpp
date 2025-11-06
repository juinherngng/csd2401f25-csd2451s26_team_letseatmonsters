/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CollisionManager.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:       Implements CollisionManager. Rebuilds a spatial grid of scene objects each frame,
					builds/owns world collision geometry, resolves step trimming, and exposes broad-
					phase queries for overlap/point tests.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "CollisionManager.hpp"

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

void CollisionManager::SetEntityManager(EntityManager* entityMgr) {
	entityManager_ = entityMgr;
}

// Per-frame rebuild
void CollisionManager::UpdateCollisions(EntityManager& entityManager) {
	// Clear spatial grid from previous frame
	spatialGrid_.Clear();

	// Get all objects as vector of pointers
	std::vector<GameObject*> allObjects = entityManager.GetAllObjects();

	// Insert each object into spatial grid
	for (GameObject* obj : allObjects) {
		if (obj == nullptr) {
			continue;
		}

		// Build AABB from object's position and scale
		const Math::Vector3D pos(obj->GetPosition().x,
			obj->GetPosition().y,
			obj->GetPosition().z);
		const glm::vec3 scale = obj->GetScaleGLM();

		collision::AABB box = collision::World::makeAABBFromCenter(
			pos,
			Math::Vector3D(scale.x, scale.y, scale.z)
		);

		// Insert into grid
		spatialGrid_.Insert(obj, box);
	}
}

// Lifecycle
void CollisionManager::Clear() {
	spatialGrid_.Clear();
	collisionWorld_.clear();
}

// World building
void CollisionManager::BuildWalls(const collision::WalkArea& walkArea,
	const collision::WoodVertical& wood,
	const collision::StageEndGateVertical& endGate) {
	collisionWorld_.build(walkArea, wood, endGate);
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
