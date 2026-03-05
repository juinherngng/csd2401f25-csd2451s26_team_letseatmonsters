/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         MovementManager.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu (50%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(40%)
					Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:       Declares MovementManager, which coordinates player WASD, click-to-move targets,
					and simple NPC patrol paths. Integrates with world collision for step trimming
					and updates sprite facing based on effective movement/intended direction.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "../Graphics/EntityManager.hpp"

#include "Collision.hpp"
#include "InputManager.hpp"
#include "Math.hpp"
#include "NpcSystem.hpp"
#include "System.hpp"

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

 /**
  * @class MovementManager
  * @brief Handles per-entity movement intents:
  *        - Player WASD & click-to-move (with arrival & wall-trim stop)
  *        - NPC patrol paths (loop or stop)
  *        - Passive velocity-based movement for objects not otherwise managed
  */
class MovementManager : public CoreFramework::SystemInterface {
public:
	MovementManager() = default;
	~MovementManager() override = default;

	// ----- SystemInterface implementation -----
	void Initialize() override;
	void Update(float deltaTime) override;
	std::string GetName() override;

	// ----- Core update & lifecycle -----

	// Per-frame update for player, click-to-move, patrol, and passive-velocity objects.
	void UpdateMovement(float deltaTime, EntityManager& entityManager, InputManager& inputManager);

	// Clear all runtime movement state.
	void Clear();

	// ----- Entity Manager Reference -----
	void SetEntityManager(EntityManager* entityMgr);
	void SetInputManager(InputManager* inputMgr);

	// ----- Player setup -----

	// Set the entity ID that represents the player.
	void SetPlayerID(int playerID);

	int GetPlayerID() const {
		return playerID_;
	}

	// ----- Movement control (generic) -----

	// Override the move speed for an object (pixels per second).
	void SetMoveSpeed(int objectID, float speed);

	// Get the configured move speed (defaults to 200.f if no entry).
	float GetMoveSpeed(int objectID) const;

	// Assign a click-to-move world target to an object.
	void SetMoveTarget(int objectID, const glm::vec2& target);

	// Remove click - to - move target and stop the object immediately.
	void ClearMoveTarget(int objectID);

	// Query if an object currently has an active click-to-move target.
	bool HasMoveTarget(int objectID) const;

	// Get the current click-to-move target; returns (0,0) if none.
	glm::vec2 GetMoveTarget(int objectID) const;

	// ----- NPC patrol -----

	// Set patrol waypoints for an object.
	void SetPatrolPath(int objectID, const std::vector<glm::vec2>& waypoints, bool loop = true);

	// Enable or disable patrol for an object.
	void EnablePatrol(int objectID, bool enable);

	// ----- Queries -----

	// True if the object has a non-zero velocity this frame.
	bool IsMoving(int objectID) const;

	// Get the last computed velocity for an object; zero if unknown.
	glm::vec2 GetVelocity(int objectID) const;

	// ----- Wiring to other systems -----

	// Provide access to world resolver used for wall trimming.
	void SetCollisionWorld(const collision::World* w) {
		world_ = w;
	}

	// Provide the NPC system to skip “lane NPCs” from passive updates.
	void SetNPCSystem(const NPCSystem* npcSys) {
		npcSystem_ = npcSys;
	}

private:
	// ----- Internal data types -----
	struct MovementData {
		// Controls / state
		float moveSpeed = 200.0f;

		// Click-to-move
		bool hasTarget = false;
		glm::vec2 moveTarget = { 0.f, 0.f };

		// Current motion
		glm::vec2 velocity = { 0.f, 0.f };

		// Facing logic
		bool hasFacingHint = false;
		glm::vec2 facingHint = { 0.f, 0.f };
		glm::vec2 lastFacing = { 0.f, 1.f };

		// Patrol
		bool patrolEnabled = false;
		bool patrolLoop = true;
		std::vector<glm::vec2> patrolPath{};
		size_t currentWaypoint = 0;
	};

	// ----- Helpers -----

	// Player-specific WASD / click-to-move resolver and world trimming.
	void UpdatePlayerMovement(float deltaTime, EntityManager& entityManager, InputManager& inputManager);

	// Generic click-to-move integrator for non-player objects.
	void UpdateClickToMove(int objectID, MovementData& data, float deltaTime, EntityManager& entityManager);

	// Patrol waypoint traversal with optional looping.
	void UpdateNPCPatrol(int objectID, MovementData& data, float deltaTime, EntityManager& entityManager);

	// Choose a sprite texture based on movement or intent.
	void UpdateSpriteDirection(int entityID, EntityManager& entityManager);

	// Move objects that rely only on their own velocity (bouncing off screen edges).
	void UpdateVelocityBasedMovement(float deltaTime, EntityManager& entityManager);

private:
	// Player
	int playerID_ = -1;

	// Runtime per-object state
	std::unordered_map<int, MovementData> movementData_;

	// External systems
	const collision::World* world_ = nullptr;
	const NPCSystem* npcSystem_ = nullptr;

	// References to other managers (set externally)
	EntityManager* entityManager_ = nullptr;
	InputManager* inputManager_ = nullptr;
};
