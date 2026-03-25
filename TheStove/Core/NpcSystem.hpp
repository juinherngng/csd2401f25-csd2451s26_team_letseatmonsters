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

		 All content  2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <glm/glm.hpp>
#include <unordered_map>

#include "../Graphics/EntityManager.hpp"

#include "CollisionManager.hpp"
#include "Physics.hpp"

 // Forward declare Scene to avoid circular dependency.
class NPCSystem {
public:

	/**
	 * @brief Constructs a `NPCSystem` instance.
	 */
	NPCSystem() = default;

	/**
	 * @brief Destroys the `NPCSystem` instance and releases owned resources.
	 */
	~NPCSystem() = default;

	/**
	 * @brief Updates this object.
	 * @param deltaTime Frame delta time in seconds.
	 * @param entityManager Entity manager containing the active objects.
	 * @param collisionManager Collision manager used for collision queries.
	 * @param walkArea Parameter for walk area.
	 */
	void Update(float deltaTime,
		EntityManager& entityManager,
		CollisionManager& collisionManager,
		const collision::WalkArea& walkArea);

	/**
	 * @brief Sets npcvelocity.
	 * @param npcID Identifier of the NPC object.
	 * @param velocity Parameter for velocity.
	 */
	void SetNPCVelocity(int npcID, const glm::vec2& velocity);

	/**
	 * @brief Returns npcvelocity.
	 * @param npcID Identifier of the NPC object.
	 * @return Requested value.
	 */
	glm::vec2 GetNPCVelocity(int npcID) const;

	/**
	 * @brief Registers lane npc.
	 * @param npcID Identifier of the NPC object.
	 * @param laneX Parameter for lane x.
	 */
	void RegisterLaneNPC(int npcID, float laneX);

	/**
	 * @brief Clears this object.
	 */
	void Clear();

	/**
	 * @brief Returns whether lane npc.
	 * @param npcID Identifier of the NPC object.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsLaneNPC(int npcID) const {
		return laneNPCs_.find(npcID) != laneNPCs_.end();
	}

private:

	/**
	 * @brief Updates lane npcs.
	 * @param deltaTime Frame delta time in seconds.
	 * @param entityManager Entity manager containing the active objects.
	 * @param collisionManagerWorld Parameter for collision manager world.
	 * @param walkArea Parameter for walk area.
	 */
	void UpdateLaneNPCs(float deltaTime,
		EntityManager& entityManager,
		CollisionManager& collisionManagerWorld,
		const collision::WalkArea& walkArea);

	/**
	 * @brief Handles npccollisions.
	 * @param entityManager Entity manager containing the active objects.
	 */
	void HandleNPCCollisions(EntityManager& entityManager);

	/**
	 * @brief Updates generic npcs.
	 * @param deltaTime Frame delta time in seconds.
	 * @param entityManager Entity manager containing the active objects.
	 * @param walkArea Parameter for walk area.
	 */
	void UpdateGenericNPCs(float deltaTime,
		EntityManager& entityManager,
		const collision::WalkArea& walkArea);

private:
	// Per-NPC velocity (both lane and non-lane).
	std::unordered_map<int, glm::vec2> npcVelocities_;

	// Lane NPCs: npcID -> lane X position.
	std::unordered_map<int, float> laneNPCs_;
};
