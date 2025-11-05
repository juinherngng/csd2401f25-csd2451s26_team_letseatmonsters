#include "NPCSystem.hpp"
#include <iostream>

void NPCSystem::Update(float deltaTime, EntityManager& entityManager, CollisionManager& collisionManager, const collision::WalkArea& walkArea) {
    UpdateLaneNPCs(deltaTime, entityManager, collisionManager, walkArea);
    HandleNPCCollisions(entityManager);
    UpdateGenericNPCs(deltaTime, entityManager, walkArea);
}

void NPCSystem::SetNPCVelocity(int npcID, const glm::vec2& velocity) {
    npcVelocities_[npcID] = velocity;
}

glm::vec2 NPCSystem::GetNPCVelocity(int npcID) const {
    auto it = npcVelocities_.find(npcID);
    return (it != npcVelocities_.end()) ? it->second : glm::vec2(0.0f);
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
        if (!npc) continue;

        Math::Vector3D posM = Math::Vector3D(npc->GetPosition().x,
            npc->GetPosition().y,
            npc->GetPosition().z);
        Math::Vector2D velM(npcVelocities_[npcID].x, npcVelocities_[npcID].y);

        physics::MoveYLaneWithBounce(collisionManager.GetCollisionWorld(),
            npc, posM, velM, laneX, deltaTime);
        physics::ClampInsideWalk(walkArea, npc, posM);

        npc->SetPosition(glm::vec3(posM.x, posM.y, posM.z));
        npcVelocities_[npcID] = glm::vec2(velM.x, velM.y);
    }
}

void NPCSystem::HandleNPCCollisions(EntityManager& entityManager) {
    // Collect ALL NPCs (not just lane NPCs)
    std::vector<int> allNPCIDs;
    for (const auto& [id, _] : npcVelocities_) {
        allNPCIDs.push_back(id);
    }

    // Pairwise elastic bounce for all NPCs
    for (size_t i = 0; i < allNPCIDs.size(); ++i) {
        for (size_t j = i + 1; j < allNPCIDs.size(); ++j) {
            int id1 = allNPCIDs[i];
            int id2 = allNPCIDs[j];

            GameObject* npc1 = entityManager.GetByID(id1);
            GameObject* npc2 = entityManager.GetByID(id2);
            if (!npc1 || !npc2) continue;

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
}


void NPCSystem::UpdateGenericNPCs(float deltaTime,
    EntityManager& entityManager,
    const collision::WalkArea& walkArea) {
    for (const auto& [npcID, velocity] : npcVelocities_) {
        // Skip lane NPCs (already handled)
        if (laneNPCs_.find(npcID) != laneNPCs_.end()) {
            continue;
        }

        GameObject* npc = entityManager.GetByID(npcID);
        if (!npc) continue;

        Math::Vector3D posM(npc->GetPosition().x, npc->GetPosition().y, npc->GetPosition().z);

        // Simple Euler integration
        posM.x += velocity.x * deltaTime;
        posM.y += velocity.y * deltaTime;

        // Clamp inside walkable area
        physics::ClampInsideWalk(walkArea, npc, posM);

        npc->SetPosition(glm::vec3(posM.x, posM.y, posM.z));
    }
}

void NPCSystem::Clear() {
    npcVelocities_.clear();
    laneNPCs_.clear();
}
