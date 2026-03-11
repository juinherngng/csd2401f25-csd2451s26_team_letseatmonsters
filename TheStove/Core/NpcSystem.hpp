/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         NpcSystem.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu	(35%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(65%)

 DESCRIPTION:       Declares NPCSystem, which updates two kinds of NPCs:
					- lane NPCs constrained to Y-lane movement with bounce,
					- generic NPCs with free planar movement.
					Handles simple pairwise elastic collisions within each group
					and clamps all NPCs inside the walkable area.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <glm/glm.hpp>
#include <unordered_map>

#include "../Graphics/EntityManager.hpp"

#include "CollisionManager.hpp"
#include "Physics.hpp"

 /**
  * @class NPCSystem
  * @brief Orchestrates lane-bound and generic NPC updates, including
  *        movement, simple collision response, and walk-area clamping.
  */
class NPCSystem {
public:
	NPCSystem() = default;
	~NPCSystem() = default;

	// Per-frame update for all NPCs.
	void Update(float deltaTime,
		EntityManager& entityManager,
		CollisionManager& collisionManager,
		const collision::WalkArea& walkArea);

	// Set or update an NPC's current velocity.
	void SetNPCVelocity(int npcID, const glm::vec2& velocity);

	// Get an NPC's velocity; returns zero vector if unknown.
	glm::vec2 GetNPCVelocity(int npcID) const;

	// Register an NPC as "lane NPC" with a fixed X lane coordinate.
	void RegisterLaneNPC(int npcID, float laneX);

	// Clear all stored NPC state (velocities and lane registrations).
	void Clear();

	bool IsLaneNPC(int npcID) const {
		return laneNPCs_.find(npcID) != laneNPCs_.end();
	}

private:
	// Update Y-lane NPCs with bounce, clamp inside walk area, and write back state.
	void UpdateLaneNPCs(float deltaTime,
		EntityManager& entityManager,
		CollisionManager& collisionManagerWorld,
		const collision::WalkArea& walkArea);

	// Pairwise elastic collisions within lane group and within generic group.
	void HandleNPCCollisions(EntityManager& entityManager);

	// Euler-integrate generic (non-lane) NPCs and clamp to walk area.
	void UpdateGenericNPCs(float deltaTime,
		EntityManager& entityManager,
		const collision::WalkArea& walkArea);

private:
	// Per-NPC velocity (both lane and non-lane).
	std::unordered_map<int, glm::vec2> npcVelocities_;

	// Lane NPCs: npcID -> lane X position.
	std::unordered_map<int, float> laneNPCs_;
};
