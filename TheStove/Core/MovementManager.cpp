#include "MovementManager.hpp"
#include "../Graphics/EntityManager.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Core/InputManager.hpp"
#include <iostream>
#include <cmath>

// ===== Core Functionality =====

void MovementManager::Update(float deltaTime, EntityManager& entityManager, InputManager& inputManager) {
	// Update player movement (WASD + click-to-move)
	if (playerID_ >= 0) {
		UpdatePlayerMovement(deltaTime, entityManager, inputManager);
		UpdateSpriteDirection(playerID_, entityManager);
	}

	// Update all objects with click-to-move or patrol
	for (auto& [objID, data] : movementData_) {
		if (objID == playerID_) continue;  // Already handled

		if (data.patrolEnabled) {
			UpdateNPCPatrol(objID, data, deltaTime, entityManager);
		}
		else if (data.hasTarget) {
			UpdateClickToMove(objID, data, deltaTime, entityManager);
		}
	}
}

void MovementManager::Clear() {
	movementData_.clear();
	playerID_ = -1;
}

// ===== Player Setup =====

void MovementManager::SetPlayerID(int playerID) {
	playerID_ = playerID;

	// Initialize player movement data if not exists
	if (movementData_.find(playerID) == movementData_.end()) {
		movementData_[playerID].lastFacing = { 0.f, 1.f };
		movementData_[playerID].moveSpeed = 200.0f;
	}
}

// ===== Movement Control =====

void MovementManager::SetMoveSpeed(int objectID, float speed) {
	movementData_[objectID].moveSpeed = speed;
}

float MovementManager::GetMoveSpeed(int objectID) const {
	auto it = movementData_.find(objectID);
	return (it != movementData_.end()) ? it->second.moveSpeed : 200.0f;
}

void MovementManager::SetMoveTarget(int objectID, const glm::vec2& target) {
	std::cout << "SetMoveTarget called for objectID: " << objectID << " to (" << target.x << ", " << target.y << ")" << std::endl;

	movementData_[objectID].hasTarget = true;
	movementData_[objectID].moveTarget = target;

	std::cout << "After setting: hasTarget = " << movementData_[objectID].hasTarget << std::endl;
}


void MovementManager::ClearMoveTarget(int objectID) {
	auto it = movementData_.find(objectID);
	if (it != movementData_.end()) {
		it->second.hasTarget = false;
		it->second.velocity = { 0.f, 0.f };
	}
}

bool MovementManager::HasMoveTarget(int objectID) const {
	auto it = movementData_.find(objectID);
	return (it != movementData_.end()) && it->second.hasTarget;
}

glm::vec2 MovementManager::GetMoveTarget(int objectID) const {
	auto it = movementData_.find(objectID);
	return (it != movementData_.end()) ? it->second.moveTarget : glm::vec2(0.f);
}

// ===== NPC Patrol =====

void MovementManager::SetPatrolPath(int objectID, const std::vector<glm::vec2>& waypoints, bool loop) {
	auto& data = movementData_[objectID];
	data.patrolPath = waypoints;
	data.patrolLoop = loop;
	data.currentWaypoint = 0;
}

void MovementManager::EnablePatrol(int objectID, bool enable) {
	movementData_[objectID].patrolEnabled = enable;
}

// ===== Query =====

bool MovementManager::IsMoving(int objectID) const {
	auto it = movementData_.find(objectID);
	if (it == movementData_.end()) return false;

	const glm::vec2& vel = it->second.velocity;
	return (vel.x != 0.f || vel.y != 0.f);
}

glm::vec2 MovementManager::GetVelocity(int objectID) const {
	auto it = movementData_.find(objectID);
	return (it != movementData_.end()) ? it->second.velocity : glm::vec2(0.f);
}

// ===== Private Helper Methods =====

void MovementManager::UpdatePlayerMovement(float deltaTime, EntityManager& entityManager, InputManager& inputManager) {
	GameObject* player = entityManager.GetByID(playerID_);
	if (!player) return;

	auto& data = movementData_[playerID_];
	//std::cout << "Player found. hasTarget: " << data.hasTarget << std::endl;

	// Priority 1: WASD movement (cancels click-to-move)
	glm::vec2 desiredMove(0.f, 0.f);
	bool pressingWASD = false;

	if (inputManager.IsKeyPressed(GLFW_KEY_W)) { desiredMove.y = -1.f; pressingWASD = true; }
	if (inputManager.IsKeyPressed(GLFW_KEY_S)) { desiredMove.y = 1.f; pressingWASD = true; }
	if (inputManager.IsKeyPressed(GLFW_KEY_A)) { desiredMove.x = -1.f; pressingWASD = true; }
	if (inputManager.IsKeyPressed(GLFW_KEY_D)) { desiredMove.x = 1.f; pressingWASD = true; }

	//std::cout << "pressingWASD: " << pressingWASD << std::endl;

	if (pressingWASD) {
		//std::cout << "WASD pressed - cancelling click-to-move" << std::endl;
		// WASD pressed - cancel click-to-move and use keyboard input
		data.hasTarget = false;

		// Normalize diagonal movement
		float length = std::sqrt(desiredMove.x * desiredMove.x + desiredMove.y * desiredMove.y);
		if (length > 0.f) {
			desiredMove /= length;
		}

		// record facing intent
		data.hasFacingHint = (length > 0.f);
		data.facingHint = (length > 0.f) ? desiredMove : glm::vec2(0.f);

		data.velocity = desiredMove * data.moveSpeed;

	}
	else if (data.hasTarget) {
		std::cout << "Processing click-to-move to (" << data.moveTarget.x << ", " << data.moveTarget.y << ")" << std::endl;
		// Priority 2: Click-to-move (only if not pressing WASD)
		glm::vec3 pos3D = player->GetPositionGLM();
		glm::vec2 pos(pos3D.x, pos3D.y);

		// Calculate direction to target
		glm::vec2 toTarget = data.moveTarget - pos;
		float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

		// Arrival threshold
		if (distance < 5.0f) {
			data.hasTarget = false;
			data.velocity = { 0.f, 0.f };
		}
		else {
			// Move towards target
			glm::vec2 direction = toTarget / distance;

			// record facing intent
			data.hasFacingHint = true;
			data.facingHint = direction;

			data.velocity = direction * data.moveSpeed;
		}

	}
	else {
		// Priority 3: No input - stop moving
		data.velocity = { 0.f, 0.f };
		data.hasFacingHint = false;
		data.facingHint = { 0.f, 0.f };
	}

	// World-aware apply: if a wall trims our step, stop & clear click target
	glm::vec3 pos = player->GetPositionGLM();

	// desired movement this frame from current velocity
	glm::vec2 desiredDelta2D = data.velocity * deltaTime;
	glm::vec2 allowedDelta2D = desiredDelta2D;

	// helper: detect if trimmed by resolver
	auto impacted = [](const glm::vec2& d, const glm::vec2& a) {
		const float eps = 1e-4f;
		return (std::fabs(d.x - a.x) > eps) || (std::fabs(d.y - a.y) > eps);
		};

	if (world_) {
		// Build current AABB from collider (same math you use in DebugVisualizer)
		const auto colSize = player->GetColliderSize();
		const auto colOff = player->GetColliderOffset();

		const Math::Vector3D center(pos.x + colOff.x, pos.y + colOff.y, pos.z);
		const Math::Vector3D scale(colSize.x, colSize.y, 1.0f);

		collision::AABB start = collision::World::makeAABBFromCenter(center, scale);

		// resolve desired step
		Math::Vector2D desiredDelta(desiredDelta2D.x, desiredDelta2D.y);
		Math::Vector2D allowedDelta = world_->resolve(start, desiredDelta);

		allowedDelta2D = { allowedDelta.x, allowedDelta.y };

		// If trimmed: keep the allowed slide, only cancel click target if we're really blocked.
		if (impacted(desiredDelta2D, allowedDelta2D)) {
			// consider "blocked" if the allowed motion this frame is tiny
			const float allowedLen = std::sqrt(allowedDelta2D.x * allowedDelta2D.x +
				allowedDelta2D.y * allowedDelta2D.y);
			// tune threshold as needed (in pixels per frame)
			if (data.hasTarget && allowedLen < 0.50f) {
				data.hasTarget = false;  // hide the green line when we truly can't advance
			}
		}
	}

	// apply the allowed movement (or raw if no world_)
	pos.x += allowedDelta2D.x;
	pos.y += allowedDelta2D.y;
	player->SetPosition(pos);

	// IMPORTANT: update velocity to the actually-allowed motion so WASD can slide
	if (deltaTime > 0.0f) {
		data.velocity = allowedDelta2D / deltaTime;
	}
	else {
		data.velocity = { 0.f, 0.f };
	}


}


void MovementManager::UpdateClickToMove(int objectID, MovementData& data, float deltaTime, EntityManager& entityManager) {
	GameObject* obj = entityManager.GetByID(objectID);
	if (!obj || !data.hasTarget) return;

	glm::vec3 pos3D = obj->GetPositionGLM();
	glm::vec2 pos(pos3D.x, pos3D.y);

	// Calculate direction to target
	glm::vec2 toTarget = data.moveTarget - pos;
	float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

	// Arrival threshold
	if (distance < 5.0f) {
		data.hasTarget = false;
		data.velocity = { 0.f, 0.f };
		return;
	}

	// Move towards target
	glm::vec2 direction = toTarget / distance;
	data.velocity = direction * data.moveSpeed;

	pos3D.x += data.velocity.x * deltaTime;
	pos3D.y += data.velocity.y * deltaTime;
	obj->SetPosition(pos3D);
}

void MovementManager::UpdateNPCPatrol(int objectID, MovementData& data, float deltaTime, EntityManager& entityManager) {
	if (data.patrolPath.empty()) return;

	GameObject* obj = entityManager.GetByID(objectID);
	if (!obj) return;

	glm::vec3 pos3D = obj->GetPositionGLM();
	glm::vec2 pos(pos3D.x, pos3D.y);

	// Get current waypoint
	glm::vec2 waypoint = data.patrolPath[data.currentWaypoint];
	glm::vec2 toWaypoint = waypoint - pos;
	float distance = std::sqrt(toWaypoint.x * toWaypoint.x + toWaypoint.y * toWaypoint.y);

	// Reached waypoint?
	if (distance < 10.0f) {
		data.currentWaypoint++;

		// End of path?
		if (data.currentWaypoint >= data.patrolPath.size()) {
			if (data.patrolLoop) {
				data.currentWaypoint = 0;  // Loop back
			}
			else {
				data.patrolEnabled = false;  // Stop
				data.velocity = { 0.f, 0.f };
				return;
			}
		}

		// Get next waypoint
		waypoint = data.patrolPath[data.currentWaypoint];
		toWaypoint = waypoint - pos;
		distance = std::sqrt(toWaypoint.x * toWaypoint.x + toWaypoint.y * toWaypoint.y);
	}

	// Move towards waypoint
	glm::vec2 direction = toWaypoint / distance;
	data.velocity = direction * data.moveSpeed;

	pos3D.x += data.velocity.x * deltaTime;
	pos3D.y += data.velocity.y * deltaTime;
	obj->SetPosition(pos3D);
}

void MovementManager::UpdateSpriteDirection(int entityID, EntityManager& entityManager) {
	auto it = movementData_.find(entityID);
	if (it == movementData_.end()) return;

	GameObject* sprite = entityManager.GetByID(entityID);
	if (!sprite) return;

	const MovementData& data = it->second;

	// base on actual motion
	glm::vec2 basis = data.velocity;
	float speed = std::sqrt(basis.x * basis.x + basis.y * basis.y);

	// if velocity is tiny or one axis got clipped, use intent hint
	if (data.hasFacingHint) {
		if (speed < 0.001f) {
			basis = data.facingHint;
		}
		else {
			// if we intended vertical but Y got clamped (top/bottom wall)
			if (std::abs(basis.y) < 0.1f && std::abs(data.facingHint.y) > std::abs(data.facingHint.x)) {
				basis = data.facingHint;
			}
			// if we intended horizontal but X got clamped (side wall)
			else if (std::abs(basis.x) < 0.1f && std::abs(data.facingHint.x) > std::abs(data.facingHint.y)) {
				basis = data.facingHint;
			}
		}
	}

	// Remember the chosen facing if it's meaningful
	if (std::fabs(basis.x) > 0.01f || std::fabs(basis.y) > 0.01f) {
		it->second.lastFacing = basis;
	}
	else {
		// If we stopped completely, keep using the previous lastFacing
		basis = it->second.lastFacing;
	}
	float ax = std::abs(basis.x);
	float ay = std::abs(basis.y);

	// --- smooth facing selection ---
	glm::vec2 prev = it->second.lastFacing;
	float prevAx = std::abs(prev.x);
	float prevAy = std::abs(prev.y);

	// small bias to keep same orientation unless direction clearly changes
	const float switchBias = 1.2f; // larger = more stickiness to previous facing

	bool useHorizontal;
	if (prevAx > prevAy) {
		// we were horizontal last frame
		useHorizontal = (ax * switchBias >= ay);
	}
	else {
		// we were vertical last frame
		useHorizontal = (ax > ay * switchBias);
	}

	if (useHorizontal) {
		if (basis.x > 0.0f) {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_right.png", "../assets/mc_sprite_right.png"));
			it->second.lastFacing = { 1.f, 0.f };
		}
		else {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_left.png", "../assets/mc_sprite_left.png"));
			it->second.lastFacing = { -1.f, 0.f };
		}
	}
	else {
		if (basis.y > 0.0f) {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_front.png", "../assets/mc_sprite_front.png"));
			it->second.lastFacing = { 0.f, 1.f };
		}
		else {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_back.png", "../assets/mc_sprite_back.png"));
			it->second.lastFacing = { 0.f, -1.f };
		}
	}

}

