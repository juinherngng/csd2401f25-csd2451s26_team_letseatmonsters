/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerManagerLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung

DESCRIPTION: Declares the CustomerManagerLogic system, which is
             responsible for pairing customers with tables, assigning
             seating targets, and maintaining runtime customer–table
             relationships.


         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

class Scene;

/**
 * @brief Scene-level system that seats NPC customers at customer tables.
 *
 * On the first Update, it:
 *  - Finds all GameObjects with CustomerTableLogic
 *  - Finds all GameObjects with SimpleNpcLogic (treated as customers)
 *  - Pairs them 1:1 in order, calls SeatCustomer() on the table,
 *    and SetCustomerTableTarget() on the NPC so they walk to the seat.
 */

class Scene;

class CustomerManagerSystem {
public:
    CustomerManagerSystem() = default;

    /// Called each frame from Scene::Update. Seats customers once.
    void Update(float dt, Scene& scene);

    /// Reset internal state when the scene is cleared.
    void Reset();

    void SetMaxCustomers(int n) { maxCustomers_ = n; }

private:
    int maxCustomers_ = 2;              // start with 2
    float spawnCooldown_ = 0.5f;        // small delay between spawns
    float spawnTimer_ = 999.0f;         // big so it spawns immediately at start

    std::vector<int> activeCustomers_;  // ids of customers alive
    std::vector<int> customerTableIDs_; // ids of customer tables we discovered
    bool cachedTables_ = false;

    int customerTemplateID_ = -1;
    bool cachedTemplate_ = false;

    void CacheTables(Scene& scene);
    void CacheTemplate(Scene& scene);
    void CleanupDeadCustomers(Scene& scene);
    bool TrySpawnOne(Scene& scene);
};
