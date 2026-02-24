/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         NpcSystem.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu (65%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(35%)

 DESCRIPTION:       Implements NPCSystem. Updates lane-bound NPCs (Y-lane with bounce)
					and generic NPCs (free planar). Performs simple pairwise elastic
					collisions within each subgroup and clamps positions to the
					walkable area.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <cmath>
#include <iostream>
#include <vector>

#include "NPCSystem.hpp"

void NPCSystem::Update(float deltaTime, EntityManager& entityManager, CollisionManager& collisionManager, const collision::WalkArea& walkArea) {
	/*UpdateLaneNPCs(deltaTime, entityManager, collisionManager, walkArea);
	HandleNPCCollisions(entityManager);
	UpdateGenericNPCs(deltaTime, entityManager, walkArea);*/
	(void)deltaTime;
	(void)entityManager;
	(void)collisionManager;
	(void)walkArea;
}

void NPCSystem::SetNPCVelocity(int npcID, const glm::vec2& velocity) {
	npcVelocities_[npcID] = velocity;
}

glm::vec2 NPCSystem::GetNPCVelocity(int npcID) const {
	auto it = npcVelocities_.find(npcID);
	if (it != npcVelocities_.end()) {
		return it->second;
	}

	return glm::vec2(0.0f);
}

void NPCSystem::RegisterLaneNPC(int npcID, float laneX) {
	laneNPCs_[npcID] = laneX;
}

void NPCSystem::UpdateLaneNPCs(float deltaTime,
							   EntityManager& entityManager,
							   CollisionManager& collisionManager,
							   const collision::WalkArea& walkArea) {
	for (const auto& [npcID, laneX] : laneNPCs_) {
		GameObject* npc = entityManager.GetByID(npcID);
		if (npc == nullptr) {
			continue;
		}

		// Pull current position/velocity into Math:: types for physics helpers.
		Math::Vector3D posM(npc->GetPosition().x,
							npc->GetPosition().y,
							npc->GetPosition().z);

		Math::Vector2D velM(npcVelocities_[npcID].x,
							npcVelocities_[npcID].y);

		// Move along Y-lane with bounce against the world (helper in your physics module).
		physics::MoveYLaneWithBounce(collisionManager.GetCollisionWorld(),
									 npc, posM, velM, laneX, deltaTime);

		// Clamp within the walkable region.
		physics::ClampInsideWalk(walkArea, npc, posM);

		// Write back position and velocity.
		npc->SetPosition(glm::vec3(posM.x, posM.y, posM.z));
		npcVelocities_[npcID] = glm::vec2(velM.x, velM.y);
	}
}

void NPCSystem::HandleNPCCollisions(EntityManager& entityManager) {
	//  Lane goats collide with each other
	std::vector<int> laneNPCIDs;
	laneNPCIDs.reserve(laneNPCs_.size());
	for (const auto& [id, _laneX] : laneNPCs_) {
		laneNPCIDs.push_back(id);
	}

	// Pairwise elastic bounce for lane goats only
	for (size_t i = 0; i < laneNPCIDs.size(); ++i) {
		for (size_t j = i + 1; j < laneNPCIDs.size(); ++j) {
			int id1 = laneNPCIDs[i];
			int id2 = laneNPCIDs[j];

			GameObject* npc1 = entityManager.GetByID(id1);
			GameObject* npc2 = entityManager.GetByID(id2);
			if (npc1 == nullptr || npc2 == nullptr) {
				continue;
			}

			Math::Vector3D p1(npc1->GetPosition().x, npc1->GetPosition().y, npc1->GetPosition().z);
			Math::Vector3D p2(npc2->GetPosition().x, npc2->GetPosition().y, npc2->GetPosition().z);
			Math::Vector2D v1(npcVelocities_[id1].x, npcVelocities_[id1].y);
			Math::Vector2D v2(npcVelocities_[id2].x, npcVelocities_[id2].y);

			physics::ElasticBounceEqualMass(npc1, npc2, p1, p2, v1, v2);

			npc1->SetPosition(glm::vec3(p1.x, p1.y, p1.z));
			npc2->SetPosition(glm::vec3(p2.x, p2.y, p2.z));

			npcVelocities_[id1] = glm::vec2(v1.x, v1.y);
			npcVelocities_[id2] = glm::vec2(v2.x, v2.y);
		}
	}

	// Non-lane NPCs collide with each other
	std::vector<int> nonLaneNPCIDs;
	nonLaneNPCIDs.reserve(npcVelocities_.size());
	for (const auto& [id, _vel] : npcVelocities_) {
		if (laneNPCs_.find(id) != laneNPCs_.end()) {
			continue; // skip lane NPCs
		}
		nonLaneNPCIDs.push_back(id);
	}

	// Pairwise elastic bounce for non-lane NPCs only
	for (size_t i = 0; i < nonLaneNPCIDs.size(); ++i) {
		for (size_t j = i + 1; j < nonLaneNPCIDs.size(); ++j) {
			int id1 = nonLaneNPCIDs[i];
			int id2 = nonLaneNPCIDs[j];

			GameObject* npc1 = entityManager.GetByID(id1);
			GameObject* npc2 = entityManager.GetByID(id2);
			if (npc1 == nullptr || npc2 == nullptr) {
				continue;
			}

			Math::Vector3D p1(npc1->GetPosition().x, npc1->GetPosition().y, npc1->GetPosition().z);
			Math::Vector3D p2(npc2->GetPosition().x, npc2->GetPosition().y, npc2->GetPosition().z);
			Math::Vector2D v1(npcVelocities_[id1].x, npcVelocities_[id1].y);
			Math::Vector2D v2(npcVelocities_[id2].x, npcVelocities_[id2].y);

			physics::ElasticBounceEqualMass(npc1, npc2, p1, p2, v1, v2);

			npc1->SetPosition(glm::vec3(p1.x, p1.y, p1.z));
			npc2->SetPosition(glm::vec3(p2.x, p2.y, p2.z));

			npcVelocities_[id1] = glm::vec2(v1.x, v1.y);
			npcVelocities_[id2] = glm::vec2(v2.x, v2.y);
		}
	}

	// Intentionally no collisions between lane and non-lane NPCs.
}

void NPCSystem::UpdateGenericNPCs(float deltaTime,
								  EntityManager& entityManager,
								  const collision::WalkArea& walkArea) {
	for (const auto& [npcID, velocity] : npcVelocities_) {
		// Skip lane NPCs (already updated in UpdateLaneNPCs).
		if (laneNPCs_.find(npcID) != laneNPCs_.end()) {
			continue;
		}

		GameObject* npc = entityManager.GetByID(npcID);
		if (npc == nullptr) {
			continue;
		}

		Math::Vector3D posM(npc->GetPosition().x, npc->GetPosition().y, npc->GetPosition().z);

		// Simple Euler step.
		posM.x += velocity.x * deltaTime;
		posM.y += velocity.y * deltaTime;

		// Clamp inside the walkable area.
		physics::ClampInsideWalk(walkArea, npc, posM);

		npc->SetPosition(glm::vec3(posM.x, posM.y, posM.z));
	}
}

void NPCSystem::Clear() {
	npcVelocities_.clear();
	laneNPCs_.clear();
}
