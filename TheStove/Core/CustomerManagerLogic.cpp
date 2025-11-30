/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerManagerlogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung

DESCRIPTION: Implements a simple scene-level system that manages all
             customers in the level. It assigns customers to available
             CustomerTableLogic tables, gives them target seating
             positions, and coordinates table–customer pairing at runtime.





         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */


#include "../Graphics/SceneManager.hpp"       // for Scene, GetAllObjectsRaw, GetLogicManager
#include "../Core/LogicManager.hpp"
#include "../Core/Math.hpp"
#include "../Core/SimpleNpcLogic.hpp"
#include "../Core/CustomerTableLogic.hpp"
#include "../Graphics/GameObject.hpp"
#include "CustomerManagerLogic.hpp"

#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

void CustomerManagerSystem::Reset()
{
    seatedOnce_ = false;
}

void CustomerManagerSystem::Update(float dt, Scene& scene)
{
    (void)dt;

    // Only do the seating logic once per scene run.
    if (seatedOnce_) {
        return;
    }

    LogicManager& logicMgr = scene.GetLogicManager();
    std::vector<GameObject*> all = scene.GetAllObjectsRaw();

    std::vector<std::pair<int, CustomerTableLogic*>> tables;
    std::vector<std::pair<int, SimpleNpcLogic*>> customers;

    tables.reserve(all.size());
    customers.reserve(all.size());

    // --------------------------------------------------------------------
    // Collect all customer tables and customer NPCs present in the scene.
    // --------------------------------------------------------------------
    for (GameObject* obj : all) {
        if (!obj) continue;

        const int id = obj->GetID();

        if (auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(id)) {
            tables.emplace_back(id, table);
        }

        if (auto* npc = logicMgr.GetLogicForObject<SimpleNpcLogic>(id)) {
            customers.emplace_back(id, npc);
        }
    }

    if (tables.empty() || customers.empty()) {
        // Nothing to do yet – maybe level is not fully spawned.
        // We simply try again next frame.
        return;
    }

    const std::size_t pairCount = std::min(tables.size(), customers.size());

    std::cout << "[CustomerManagerSystem] Found "
        << tables.size() << " customer tables and "
        << customers.size() << " customers. Pairing "
        << pairCount << " of them.\n";

    // --------------------------------------------------------------------
    // Pair them 1:1: for each (table, customer), seat and give seat target.
    // --------------------------------------------------------------------
    for (std::size_t i = 0; i < pairCount; ++i) {
        int tableObjID = tables[i].first;
        CustomerTableLogic* table = tables[i].second;

        int npcObjID = customers[i].first;
        SimpleNpcLogic* npc = customers[i].second;

        if (!table || !npc) {
            continue;
        }

        // Don’t re-seat a table that already has a customer.
        if (table->HasSeatedCustomer()) {
            continue;
        }

        // Seat the customer on this table.
        if (!table->SeatCustomer(npcObjID)) {
            // Table refused for some reason; skip.
            continue;
        }

        // Ask table where the seat is in world space.
        Math::Vector2D seatPos = table->GetCustomerSeatWorld(scene);

        // Tell the NPC to go to that seat; this sets hasCustomerTarget_ = true,
        // which makes SimpleNpcLogic use the "go to seat" path instead of
        // the vertical patrol fallback.
        npc->SetCustomerTableTarget(tableObjID, seatPos);

        std::cout << "[CustomerManagerSystem] Seated NPC " << npcObjID
            << " at table " << tableObjID
            << " seat=(" << seatPos.x << ", " << seatPos.y << ")\n";
    }

    // From now on we don’t need to run this again.
    seatedOnce_ = true;
}
