#include "CollisionManager.hpp"

CollisionManager::CollisionManager(float cellSize)
	: spatialGrid_(cellSize) {
}

void CollisionManager::Update(EntityManager& entityManager) {
	// Clear spatial grid from previous frame
	spatialGrid_.Clear();

	// Get all objects as vector of pointers
	std::vector<GameObject*> allObjects = entityManager.GetAllObjects();

	// Insert each object into spatial grid
	for (GameObject* obj : allObjects) {
		if (!obj) continue;  // Skip null pointers

		// Build AABB from object's position and scale
		const Math::Vector3D pos(obj->GetPosition().x, obj->GetPosition().y, obj->GetPosition().z);
		const glm::vec3 scale = obj->GetScaleGLM();

		collision::AABB box = collision::World::makeAABBFromCenter(
			pos,
			Math::Vector3D(scale.x, scale.y, scale.z)
		);

		// Insert into grid
		spatialGrid_.Insert(obj, box);
	}
}

void CollisionManager::Clear() {
	spatialGrid_.Clear();
	collisionWorld_.clear();
}

void CollisionManager::BuildWalls(const collision::WalkArea& walkArea,
	const collision::WoodVertical& wood,
	const collision::StageEndGateVertical& endGate) {
	collisionWorld_.build(walkArea, wood, endGate);
}

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
