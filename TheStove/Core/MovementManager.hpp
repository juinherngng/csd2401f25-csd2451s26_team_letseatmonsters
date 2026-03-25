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

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
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

	/**
	 * @brief Constructs a `MovementManager` instance.
	 */
	MovementManager() = default;

	/**
	 * @brief Destroys the `MovementManager` instance and releases owned resources.
	 */
	~MovementManager() override = default;

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

	// ----- Core update & lifecycle -----

	/**
	 * @brief Updates movement.
	 * @param deltaTime Frame delta time in seconds.
	 * @param entityManager Entity manager containing the active objects.
	 * @param inputManager Input manager for the current frame.
	 */
	void UpdateMovement(float deltaTime, EntityManager& entityManager, InputManager& inputManager);

	/**
	 * @brief Clears this object.
	 */
	void Clear();

	/**
	 * @brief Sets entity manager.
	 * @param entityMgr Parameter for entity mgr.
	 */
	void SetEntityManager(EntityManager* entityMgr);

	/**
	 * @brief Sets input manager.
	 * @param inputMgr Parameter for input mgr.
	 */
	void SetInputManager(InputManager* inputMgr);

	// ----- Player setup -----

	/**
	 * @brief Sets player id.
	 * @param playerID Identifier of the player object.
	 */
	void SetPlayerID(int playerID);

	/**
	 * @brief Returns player id.
	 * @return Requested value.
	 */
	int GetPlayerID() const {
		return playerID_;
	}

	// ----- Movement control (generic) -----

	/**
	 * @brief Sets move speed.
	 * @param objectID Identifier of the target object.
	 * @param speed Parameter for speed.
	 */
	void SetMoveSpeed(int objectID, float speed);

	/**
	 * @brief Returns move speed.
	 * @param objectID Identifier of the target object.
	 * @return Requested value.
	 */
	float GetMoveSpeed(int objectID) const;

	/**
	 * @brief Sets move target.
	 * @param objectID Identifier of the target object.
	 * @param target Parameter for target.
	 */
	void SetMoveTarget(int objectID, const glm::vec2& target);

	/**
	 * @brief Clears move target.
	 * @param objectID Identifier of the target object.
	 */
	void ClearMoveTarget(int objectID);

	/**
	 * @brief Returns whether move target.
	 * @param objectID Identifier of the target object.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasMoveTarget(int objectID) const;

	/**
	 * @brief Returns move target.
	 * @param objectID Identifier of the target object.
	 * @return Requested value.
	 */
	glm::vec2 GetMoveTarget(int objectID) const;

	// ----- NPC patrol -----

	/**
	 * @brief Sets patrol path.
	 * @param objectID Identifier of the target object.
	 * @param waypoints Parameter for waypoints.
	 * @param loop Parameter for loop.
	 */
	void SetPatrolPath(int objectID, const std::vector<glm::vec2>& waypoints, bool loop = true);

	/**
	 * @brief Enables patrol.
	 * @param objectID Identifier of the target object.
	 * @param enable Boolean flag controlling whether the feature is enabled.
	 */
	void EnablePatrol(int objectID, bool enable);

	// ----- Queries -----

	/**
	 * @brief Returns whether moving.
	 * @param objectID Identifier of the target object.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsMoving(int objectID) const;

	/**
	 * @brief Returns velocity.
	 * @param objectID Identifier of the target object.
	 * @return Requested value.
	 */
	glm::vec2 GetVelocity(int objectID) const;

	// ----- Wiring to other systems -----

	/**
	 * @brief Sets collision world.
	 * @param w Parameter for w.
	 */
	void SetCollisionWorld(const collision::World* w) {
		world_ = w;
	}

	/**
	 * @brief Sets npcsystem.
	 * @param npcSys Parameter for npc sys.
	 */
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

	/**
	 * @brief Updates player movement.
	 * @param deltaTime Frame delta time in seconds.
	 * @param entityManager Entity manager containing the active objects.
	 * @param inputManager Input manager for the current frame.
	 */
	void UpdatePlayerMovement(float deltaTime, EntityManager& entityManager, InputManager& inputManager);

	/**
	 * @brief Updates click to move.
	 * @param objectID Identifier of the target object.
	 * @param data Parameter for data.
	 * @param deltaTime Frame delta time in seconds.
	 * @param entityManager Entity manager containing the active objects.
	 */
	void UpdateClickToMove(int objectID, MovementData& data, float deltaTime, EntityManager& entityManager);

	/**
	 * @brief Updates npcpatrol.
	 * @param objectID Identifier of the target object.
	 * @param data Parameter for data.
	 * @param deltaTime Frame delta time in seconds.
	 * @param entityManager Entity manager containing the active objects.
	 */
	void UpdateNPCPatrol(int objectID, MovementData& data, float deltaTime, EntityManager& entityManager);

	/**
	 * @brief Updates sprite direction.
	 * @param entityID Parameter for entity id.
	 * @param entityManager Entity manager containing the active objects.
	 */
	void UpdateSpriteDirection(int entityID, EntityManager& entityManager);

	/**
	 * @brief Updates velocity based movement.
	 * @param deltaTime Frame delta time in seconds.
	 * @param entityManager Entity manager containing the active objects.
	 */
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

