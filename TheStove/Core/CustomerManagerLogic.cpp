/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerManagerlogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu   (95%)
 CO-AUTHORS:        Ng Juin Herng, juinherng.ng@digipen.edu (5%)

 DESCRIPTION:       Implements a simple scene-level system that manages all
                    customers in the level. It assigns customers to available
                    CustomerTableLogic tables, gives them target seating
                    positions, and coordinates table–customer pairing at runtime.

         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */


#include "../Graphics/SceneManager.hpp"
#include "../Core/AudioManager.hpp"
#include "../Core/LogicManager.hpp"
#include "../Core/Math.hpp"
#include "../Core/SimpleNpcLogic.hpp"
#include "../Core/CustomerTableLogic.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Core/CustomerOrderUILogic.hpp"
#include "../Core/CustomerManagerLogic.hpp"

#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>
#include <string>

void CustomerManagerSystem::Reset()
{
    activeCustomers_.clear();
    customerTableIDs_.clear();

    cachedTables_ = false;

    customerTemplateID_ = -1;
    cachedTemplate_ = false;

    spawnTimer_ = 180.0f;
}

void CustomerManagerSystem::CacheTables(Scene& scene) {
    customerTableIDs_.clear();

    LogicManager& logicMgr = scene.GetLogicManager();
    for (GameObject* obj : scene.GetAllObjectsRaw()) {
        if (!obj) continue;
        const int id = obj->GetID();
        if (logicMgr.GetLogicForObject<CustomerTableLogic>(id)) {
            customerTableIDs_.push_back(id);
        }
    }

    cachedTables_ = true;
    std::cout << "[CustomerManager] Cached " << customerTableIDs_.size() << " customer tables\n";
}

void CustomerManagerSystem::CacheTemplate(Scene& scene)
{
    customerTemplateID_ = -1;

    for (GameObject* obj : scene.GetAllObjectsRaw()) {
        if (!obj) continue;

        const int id = obj->GetID();
        Scene::Defaults d = scene.GetDefaults(id);

        if (d.tag == "customer_template") {
            customerTemplateID_ = id;
            break;
        }
    }

    cachedTemplate_ = true;

    if (customerTemplateID_ < 0) {
        std::cout << "[CustomerManager] WARNING: No customer_template found in JSON.\n";
    }
    else {
        std::cout << "[CustomerManager] Cached customer template id=" << customerTemplateID_ << "\n";
    }
}


void CustomerManagerSystem::CleanupDeadCustomers(Scene& scene) {
    activeCustomers_.erase(
        std::remove_if(activeCustomers_.begin(), activeCustomers_.end(),
            [&](int id) {
                return scene.GetGameObjectByID(id) == nullptr;
            }),
        activeCustomers_.end()
    );
}

bool CustomerManagerSystem::TrySpawnOne(Scene& scene)
{
    if (customerTemplateID_ < 0)
        return false;

    LogicManager& logicMgr = scene.GetLogicManager();

    // Find an empty customer table
    CustomerTableLogic* chosenTable = nullptr;
    int chosenTableID = -1;

    for (int tableID : customerTableIDs_) {
        auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(tableID);
        if (!table) continue;
        if (table->IsAvailableForSeating()) {
            chosenTable = table;
            chosenTableID = tableID;
            break;
        }
    }
    if (!chosenTable) return false;

    // Read profile from template
    Scene::Defaults prof = scene.GetDefaults(customerTemplateID_);

    // Spawn position (you’re using exit gate right now; later make a dedicated entrance)
    Math::Vector2D spawn2 = scene.GetExitGateWorldPos();
    glm::vec3 spawnPos{ spawn2.x, spawn2.y, 0.0f };

    GameObject* npc = scene.SpawnStaticSprite(
        prof.texture,
        spawnPos,
        prof.size,
        prof.layer
    );
    if (!npc) return false;

    const int npcID = npc->GetID();

    // Apply collider/profile settings
    npc->SetColliderSize(Math::Vector2D(prof.colSize.x, prof.colSize.y));
    npc->SetColliderOffset(Math::Vector2D(prof.colOff.x, prof.colOff.y));
    scene.SetNPCVelocity(npcID, prof.vel.x, prof.vel.y);
    scene.SetObjectTexturePath(npcID, prof.texture);

    // Give it logic
    if (auto* npcLogic = logicMgr.AddLogic<SimpleNpcLogic>(npcID)) {
        npcLogic->Awake(scene);
        npcLogic->Start(scene);
    }

    //attach UI logic to the customer
    if (auto* ui = logicMgr.AddLogic<CustomerOrderUILogic>(npcID)) {
        ui->Start(scene);
    }

    // Seat + assign target
    chosenTable->SeatCustomer(npcID);
    Math::Vector2D seatWorld = chosenTable->GetCustomerSeatWorld(scene);

    if (auto* npcLogic = logicMgr.GetLogicForObject<SimpleNpcLogic>(npcID)) {
        npcLogic->SetCustomerTableTarget(chosenTableID, seatWorld);
    }

    scene.ClampToWalkArea(npc);

    activeCustomers_.push_back(npcID);

    // Play customer entering sound effect (release mode only)
#ifndef _DEBUG
    if (AudioManager* audioMgr = scene.GetAudioManager()) {
        audioMgr->PlaySound("sfx_customer_entering", audioMgr->GetVfxVolume() * 0.3f, false);
    }
#endif

    std::cout << "[CustomerManager] Spawned customer " << npcID
        << " -> table " << chosenTableID << "\n";

    return true;
}


void CustomerManagerSystem::Update(float dt, Scene& scene)
{
    if (!scene.IsSimulationActive())
        return;

    if (!cachedTables_) CacheTables(scene);
    if (!cachedTemplate_) CacheTemplate(scene);

    CleanupDeadCustomers(scene);

    // Hard cap by number of tables
    const int tableCap = static_cast<int>(customerTableIDs_.size());
    const int targetCount = std::min(maxCustomers_, tableCap);

    spawnTimer_ += dt;

    while ((int)activeCustomers_.size() < targetCount && spawnTimer_ >= spawnCooldown_) {
        spawnTimer_ = 0.0f;

        if (!TrySpawnOne(scene)) {
            break; // no empty table or no template
        }
    }
}

