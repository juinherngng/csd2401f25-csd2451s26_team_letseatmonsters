#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

#include "Collision.hpp"
#include "Math.hpp"

class EntityManager;  // Forward declaration
class InputManager;
class NPCSystem;

/**
 * @brief Manages movement for all game objects
 *
 * Handles player input-based movement (WASD, click-to-move),
 * NPC patrol logic, and target following.
 */
class MovementManager {
public:
	MovementManager() = default;
	~MovementManager() = default;

	// Core functionality
	void Update(float deltaTime, EntityManager& entityManager, InputManager& inputManager);
	void Clear();

	// Player movement setup
	void SetPlayerID(int playerID);
	int GetPlayerID() const { return playerID_; }

	// Movement control
	void SetMoveSpeed(int objectID, float speed);
	float GetMoveSpeed(int objectID) const;

	// Click-to-move
	void SetMoveTarget(int objectID, const glm::vec2& target);
	void ClearMoveTarget(int objectID);
	bool HasMoveTarget(int objectID) const;
	glm::vec2 GetMoveTarget(int objectID) const;

	// NPC patrol
	void SetPatrolPath(int objectID, const std::vector<glm::vec2>& waypoints, bool loop = true);
	void EnablePatrol(int objectID, bool enable);

	void SetNPCSystem(const NPCSystem* npcSys) { npcSystem_ = npcSys; }

	// Query
	bool IsMoving(int objectID) const;
	glm::vec2 GetVelocity(int objectID) const;

	// Sprite direction update
	void UpdateSpriteDirection(int entityID, EntityManager& entityManager);

	void SetCollisionWorld(const collision::World* w) { world_ = w; }

private:
	// Player tracking
	int playerID_ = -1;

	// Movement data per object
	struct MovementData {
		float moveSpeed = 200.0f;           // pixels/second
		glm::vec2 velocity = { 0.f, 0.f };    // current velocity

		// Click-to-move
		bool hasTarget = false;
		glm::vec2 moveTarget = { 0.f, 0.f };

		// NPC patrol
		bool patrolEnabled = false;
		bool patrolLoop = true;
		std::vector<glm::vec2> patrolPath;
		size_t currentWaypoint = 0;

		// Facing hint: stores player intent so we can choose the right sprite when motion is blocked
		glm::vec2 facingHint = { 0.f, 0.f };
		bool      hasFacingHint = false;

		// Remember last visible facing so idle/stop keeps orientation
		glm::vec2 lastFacing = { 0.f, 1.f }; // default = front (positive Y)


	};

	std::unordered_map<int, MovementData> movementData_;
	const collision::World* world_ = nullptr;
	const NPCSystem* npcSystem_ = nullptr;

	// Helper methods
	void UpdatePlayerMovement(float deltaTime, EntityManager& entityManager, InputManager& inputManager);
	void UpdateClickToMove(int objectID, MovementData& data, float deltaTime, EntityManager& entityManager);
	void UpdateNPCPatrol(int objectID, MovementData& data, float deltaTime, EntityManager& entityManager);
	void UpdateVelocityBasedMovement(float deltaTime, EntityManager& entityManager);
};

