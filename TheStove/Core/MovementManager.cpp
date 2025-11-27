/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         MovementManager.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu
					Ng Juin Herng, juinherng.ng@digipen.edu

 DESCRIPTION:       Implements MovementManager. Updates player WASD and click-to-move, NPC patrols,
					and passive velocity-based motion. Uses world trimming to resolve step movement
					against walls and updates sprite facing based on effective direction.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <cmath>
#include <iostream>
#include <vector>

#include "../Core/InputManager.hpp"
#include "../Graphics/EntityManager.hpp"
#include "../Graphics/GameObject.hpp"

#include "MovementManager.hpp"
#include "NPCSystem.hpp"

 // ----- SystemInterface implementation -----
void MovementManager::Initialize() {
	std::cout << "MovementManager initialized" << std::endl;
}

void MovementManager::Update(float deltaTime) {
	if (entityManager_ && inputManager_) {
		UpdateMovement(deltaTime, *entityManager_, *inputManager_);
	}
}

std::string MovementManager::GetName() {
	return "MovementManager";
}

void MovementManager::SetEntityManager(EntityManager* entityMgr) {
	entityManager_ = entityMgr;
}

void MovementManager::SetInputManager(InputManager* inputMgr) {
	inputManager_ = inputMgr;
}

// Core Functionality
void MovementManager::UpdateMovement(float deltaTime, EntityManager& entityManager, InputManager& inputManager) {
	// Update player movement (WASD + click-to-move)
	if (playerID_ >= 0) {
		UpdatePlayerMovement(deltaTime, entityManager, inputManager);
		UpdateSpriteDirection(playerID_, entityManager);
	}

	// Update all objects with click-to-move or patrol
	for (auto& [objID, data] : movementData_) {
		if (objID == playerID_) {
			continue;  // Already handled
		}

		if (data.patrolEnabled) {
			UpdateNPCPatrol(objID, data, deltaTime, entityManager);
		}
		else if (data.hasTarget) {
			UpdateClickToMove(objID, data, deltaTime, entityManager);
		}
	}

	// Objects that just have a velocity are updated here (bouncy bounds)
	UpdateVelocityBasedMovement(deltaTime, entityManager);
}

void MovementManager::Clear() {
	movementData_.clear();
	playerID_ = -1;
}

// Player Setup
void MovementManager::SetPlayerID(int playerID) {
	playerID_ = playerID;

	// Initialize player movement data if not exists
	if (movementData_.find(playerID) == movementData_.end()) {
		movementData_[playerID].lastFacing = { 0.f, 1.f };
		movementData_[playerID].moveSpeed = 200.0f;
	}
}

// Movement Control
void MovementManager::SetMoveSpeed(int objectID, float speed) {
	movementData_[objectID].moveSpeed = speed;
}

float MovementManager::GetMoveSpeed(int objectID) const {
	auto it = movementData_.find(objectID);
	if (it != movementData_.end()) {
		return it->second.moveSpeed;
	}

	return 200.0f;
}

void MovementManager::SetMoveTarget(int objectID, const glm::vec2& target) {
	auto& md = movementData_[objectID];
	md.hasTarget = true;
	md.moveTarget = target;
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
	if (it != movementData_.end()) {
		return it->second.moveTarget;
	}

	return glm::vec2(0.f);
}

// NPC Patrol
void MovementManager::SetPatrolPath(int objectID, const std::vector<glm::vec2>& waypoints, bool loop) {
	auto& data = movementData_[objectID];
	data.patrolPath = waypoints;
	data.patrolLoop = loop;
	data.currentWaypoint = 0;
}

void MovementManager::EnablePatrol(int objectID, bool enable) {
	movementData_[objectID].patrolEnabled = enable;
}

// Query
bool MovementManager::IsMoving(int objectID) const {
	auto it = movementData_.find(objectID);
	if (it == movementData_.end()) {
		return false;
	}

	const glm::vec2& vel = it->second.velocity;
	return (vel.x != 0.f || vel.y != 0.f);
}

glm::vec2 MovementManager::GetVelocity(int objectID) const {
	auto it = movementData_.find(objectID);
	if (it != movementData_.end()) {
		return it->second.velocity;
	}

	return glm::vec2(0.f);
}

// Internals
void MovementManager::UpdatePlayerMovement(float deltaTime, EntityManager& entityManager, InputManager& inputManager) {
	GameObject* player = entityManager.GetByID(playerID_);
	if (player == nullptr) {
		return;
	}

	auto& data = movementData_[playerID_];

	// WASD movement (cancels click-to-move)
	glm::vec2 desiredMove(0.f, 0.f);
	bool pressingWASD = false;

	if (inputManager.IsKeyPressed(GLFW_KEY_W)) {
		desiredMove.y = -1.f; pressingWASD = true;
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_S)) {
		desiredMove.y = 1.f; pressingWASD = true;
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_A)) {
		desiredMove.x = -1.f; pressingWASD = true;
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_D)) {
		desiredMove.x = 1.f; pressingWASD = true;
	}

	if (pressingWASD) {
		// Cancel click-to-move and use keyboard input
		data.hasTarget = false;

		// Normalize diagonal movement
		const float lenSq = desiredMove.x * desiredMove.x + desiredMove.y * desiredMove.y;
		if (lenSq > 0.f) {
			const float invLen = 1.0f / std::sqrt(lenSq);
			desiredMove *= invLen;
		}

		// Record facing intent
		data.hasFacingHint = (lenSq > 0.f);
		data.facingHint = (lenSq > 0.f)?desiredMove:glm::vec2(0.f);

		data.velocity = desiredMove * data.moveSpeed;
	}
	else if (data.hasTarget) {
		// Click-to-move (only if not pressing WASD)
		const glm::vec3 pos3D = player->GetPositionGLM();
		const glm::vec2 pos(pos3D.x, pos3D.y);

		// Direction to target
		const glm::vec2 toTarget = data.moveTarget - pos;
		const float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

		// Arrival threshold
		if (distance < 5.0f) {
			data.hasTarget = false;
			data.velocity = { 0.f, 0.f };
		}
		else {
			const glm::vec2 direction = toTarget / distance;

			// Record facing intent
			data.hasFacingHint = true;
			data.facingHint = direction;

			data.velocity = direction * data.moveSpeed;
		}
	}
	else {
		// No input, stop moving
		data.velocity = { 0.f, 0.f };
		data.hasFacingHint = false;
		data.facingHint = { 0.f, 0.f };
	}

	// If a wall trims our step, stop & clear click target
	glm::vec3 pos = player->GetPositionGLM();

	// Desired movement this frame from current velocity
	const glm::vec2 desiredDelta2D = data.velocity * deltaTime;
	glm::vec2 allowedDelta2D = desiredDelta2D;

	// Detect if trimmed by resolver
	auto impacted = [](const glm::vec2& d, const glm::vec2& a) {
		const float eps = 1e-4f;
		return (std::fabs(d.x - a.x) > eps) || (std::fabs(d.y - a.y) > eps);
	};

	if (world_ != nullptr) {
		// Build current AABB from collider
		const auto colSize = player->GetColliderSize();
		const auto colOff = player->GetColliderOffset();

		const Math::Vector3D center(pos.x + colOff.x, pos.y + colOff.y, pos.z);
		const Math::Vector3D scale(colSize.x, colSize.y, 1.0f);

		collision::AABB start = collision::World::makeAABBFromCenter(center, scale);

		// Resolve desired step
		const Math::Vector2D desiredDelta(desiredDelta2D.x, desiredDelta2D.y);
		const Math::Vector2D allowedDelta = world_->resolve(start, desiredDelta);

		allowedDelta2D = { allowedDelta.x, allowedDelta.y };

		// If trimmed: keep the allowed slide, only cancel click target if we're really blocked
		if (impacted(desiredDelta2D, allowedDelta2D)) {
			const float allowedLen = std::sqrt(allowedDelta2D.x * allowedDelta2D.x +
											   allowedDelta2D.y * allowedDelta2D.y);

			// Tune threshold as needed (in pixels per frame)
			if (data.hasTarget && allowedLen < 0.50f) {
				data.hasTarget = false;  // hide the green path when we truly can't advance
			}
		}
	}

	// Apply the allowed movement
	pos.x += allowedDelta2D.x;
	pos.y += allowedDelta2D.y;
	player->SetPosition(pos);

	// Update velocity to the actually-allowed motion so WASD can slide
	if (deltaTime > 0.0f) {
		data.velocity = allowedDelta2D / deltaTime;
	}
	else {
		data.velocity = { 0.f, 0.f };
	}
}

void MovementManager::UpdateClickToMove(int objectID, MovementData& data, float deltaTime, EntityManager& entityManager) {
	GameObject* obj = entityManager.GetByID(objectID);
	if (obj == nullptr || !data.hasTarget) {
		return;
	}

	glm::vec3 pos3D = obj->GetPositionGLM();
	glm::vec2 pos(pos3D.x, pos3D.y);

	// Direction to target
	const glm::vec2 toTarget = data.moveTarget - pos;
	const float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

	// Arrival threshold
	if (distance < 5.0f) {
		data.hasTarget = false;
		data.velocity = { 0.f, 0.f };
		return;
	}

	// Move towards target
	const glm::vec2 direction = toTarget / distance;
	data.velocity = direction * data.moveSpeed;

	pos3D.x += data.velocity.x * deltaTime;
	pos3D.y += data.velocity.y * deltaTime;
	obj->SetPosition(pos3D);
}

void MovementManager::UpdateNPCPatrol(int objectID, MovementData& data, float deltaTime, EntityManager& entityManager) {
	if (data.patrolPath.empty()) {
		return;
	}

	GameObject* obj = entityManager.GetByID(objectID);
	if (obj == nullptr) {
		return;
	}

	glm::vec3 pos3D = obj->GetPositionGLM();
	glm::vec2 pos(pos3D.x, pos3D.y);

	// Current waypoint
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

		// Next waypoint
		waypoint = data.patrolPath[data.currentWaypoint];
		toWaypoint = waypoint - pos;
		distance = std::sqrt(toWaypoint.x * toWaypoint.x + toWaypoint.y * toWaypoint.y);
	}

	// Move towards waypoint
	const glm::vec2 direction = toWaypoint / std::max(distance, 1e-6f);
	data.velocity = direction * data.moveSpeed;

	pos3D.x += data.velocity.x * deltaTime;
	pos3D.y += data.velocity.y * deltaTime;
	obj->SetPosition(pos3D);
}

void MovementManager::UpdateSpriteDirection(int entityID, EntityManager& entityManager) {
	auto it = movementData_.find(entityID);
	if (it == movementData_.end()) {
		return;
	}

	GameObject* sprite = entityManager.GetByID(entityID);
	if (sprite == nullptr) {
		return;
	}

	MovementData& data = it->second;

	// Base on actual motion
	glm::vec2 basis = data.velocity;
	const float speed = std::sqrt(basis.x * basis.x + basis.y * basis.y);

	// If velocity is tiny or one axis got clipped, use intent hint
	if (data.hasFacingHint) {
		if (speed < 0.001f) {
			basis = data.facingHint;
		}
		else {
			// If we intended vertical but Y got clamped (top/bottom wall)
			if (std::abs(basis.y) < 0.1f && std::abs(data.facingHint.y) > std::abs(data.facingHint.x)) {
				basis = data.facingHint;
			}
			// If we intended horizontal but X got clamped (side wall)
			else if (std::abs(basis.x) < 0.1f && std::abs(data.facingHint.x) > std::abs(data.facingHint.y)) {
				basis = data.facingHint;
			}
		}
	}

	// Remember the chosen facing if it’s meaningful
	if (std::fabs(basis.x) > 0.01f || std::fabs(basis.y) > 0.01f) {
		data.lastFacing = basis;
	}
	else {
		// If stopped completely, keep using the previous lastFacing
		basis = data.lastFacing;
	}

	const float ax = std::abs(basis.x);
	const float ay = std::abs(basis.y);

	// Smooth facing selection
	const glm::vec2 prev = data.lastFacing;
	const float prevAx = std::abs(prev.x);
	const float prevAy = std::abs(prev.y);

	// Small bias to keep same orientation unless direction clearly changes
	const float switchBias = 1.2f; // larger = more stickiness to previous facing

	bool useHorizontal;
	if (prevAx > prevAy) {
		// was horizontal last frame
		useHorizontal = (ax * switchBias >= ay);
	}
	else {
		// was were vertical last frame
		useHorizontal = (ax > ay * switchBias);
	}

	if (useHorizontal) {
		if (basis.x > 0.0f) {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_right.png", "../assets/mc_sprite_right.png"));
			data.lastFacing = { 1.f, 0.f };
		}
		else {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_left.png", "../assets/mc_sprite_left.png"));
			data.lastFacing = { -1.f, 0.f };
		}
	}
	else {
		if (basis.y > 0.0f) {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_front.png", "../assets/mc_sprite_front.png"));
			data.lastFacing = { 0.f, 1.f };
		}
		else {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_back.png", "../assets/mc_sprite_back.png"));
			data.lastFacing = { 0.f, -1.f };
		}
	}
}

void MovementManager::UpdateVelocityBasedMovement(float deltaTime, EntityManager& entityManager) {
	std::vector<GameObject*> allObjects = entityManager.GetAllObjects();

	for (GameObject* obj : allObjects) {
		if (obj == nullptr) {
			continue;
		}

		const int objID = obj->GetID();

		// Skip managed objects
		auto it = movementData_.find(objID);
		if (it != movementData_.end() && (it->second.hasTarget || it->second.patrolEnabled)) {
			continue;
		}

		// Skip player
		if (objID == playerID_) {
			continue;
		}

		// Skip lane NPCs, handled by NPCSystem
		if (npcSystem_ && npcSystem_->IsLaneNPC(objID)) {
			continue;
		}

		// Passive velocity update
		Math::Vector2D velocity = obj->GetVelocity();
		if (velocity.x == 0.0f && velocity.y == 0.0f) {
			continue; // No velocity, skip
		}

		// Update position
		glm::vec3 pos = obj->GetPositionGLM();
		pos.x += velocity.x * deltaTime;
		pos.y += velocity.y * deltaTime;

		// Bounce off screen edges
		const float kWorldWidth = 1150.0f;
		const float kWorldHeight = 750.0f;

		if (pos.x < 0.0f) {
			pos.x = 0.0f;
			velocity.x = -velocity.x; // Reverse X
			obj->SetVelocity(velocity);
		}
		if (pos.x > kWorldWidth) {
			pos.x = kWorldWidth;
			velocity.x = -velocity.x; // Reverse X
			obj->SetVelocity(velocity);
		}
		if (pos.y < 0.0f) {
			pos.y = 0.0f;
			velocity.y = -velocity.y; // Reverse Y
			obj->SetVelocity(velocity);
		}
		if (pos.y > kWorldHeight) {
			pos.y = kWorldHeight;
			velocity.y = -velocity.y; // Reverse Y
			obj->SetVelocity(velocity);
		}

		obj->SetPosition(pos);
	}
}
