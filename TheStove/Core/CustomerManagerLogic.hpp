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
class CustomerManagerSystem {
public:
    CustomerManagerSystem() = default;

    /// Called each frame from Scene::Update. Seats customers once.
    void Update(float dt, Scene& scene);

    /// Reset internal state when the scene is cleared.
    void Reset();

private:
    bool seatedOnce_ = false;
};
