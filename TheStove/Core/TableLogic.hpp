/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         TableLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Implements the base class TableLogic, providing shared behavior
                    for all table-type objects. This includes item placement and
                    pickup, approach-point calculation for NPCs/players, and generic
                    per-table interaction logic. Specialized tables (worktable,
                    customer table, ingredient box, etc.) inherit and override this.

         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <vector>
#include <limits>
#include "GameObjectLogic.hpp"
#include "Math.hpp"

// Base logic for any table: kitchen table, work table, customer table, etc.
// - Can hold exactly one item (by GameObject ID).
// - Exposes approach points where the player should stand when interacting.
// - Provides virtual hooks so derived classes can restrict what items are allowed.
class TableLogic : public GameObjectLogic
{
public:
    explicit TableLogic(int ownerID);

    // No special Awake/Update yet – this is pure logic, driven by other systems.
    void Start(Scene& scene) override;
    void OnDestroy(Scene& scene) override;

    // -------- Item occupancy --------

    // Returns true if the table is currently holding an item.
    bool HasItem() const { return heldItemID_ != kInvalidID; }

    // ID of the GameObject currently on this table (or -1 if none).
    int GetHeldItemID() const { return heldItemID_; }

    // Can this table accept this item *right now*?
    // Base implementation: true if the table is empty and the item exists.
    // Derived classes (e.g. CustomerTable, WorkTable) can override to add rules.
    virtual bool CanAcceptItem(Scene& scene, int itemID) const;

    // Place an item onto this table.
    // - Returns true on success.
    // - Does NOT change ownership in any container, it just records the ID and
    //   optionally repositions the item to the table top.
    virtual bool PlaceItem(Scene& scene, int itemID);

    // Take the current item off the table.
    // - Returns the held item ID, or kInvalidID if empty.
    // - Does NOT reposition the item; caller is responsible for moving it.
    virtual int TakeItem(Scene& scene);

    // -------- Approach / destination points --------

    // These are LOCAL offsets relative to the table's position (2D: x,y).
    // For example: (0, -32) could mean "in front of the table".
    const std::vector<Math::Vector2D>& GetLocalApproachOffsets() const
    {
        return approachOffsets_;
    }

    // Add a new approach offset in local space.
    void AddApproachOffset(const Math::Vector2D& offset);

    // Replace any existing approach offsets with a single one
    void SetSingleApproachOffset(const Math::Vector2D& offset)
    {
        ClearApproachOffsets();
        AddApproachOffset(offset);
    }


    // Remove all approach offsets.
    void ClearApproachOffsets();

    // Get all approach points in WORLD space, computed from the owner GameObject.
    // If there is no owner, returns an empty vector.
    std::vector<Math::Vector2D> GetApproachPointsWorld(Scene& scene) const;

    // Choose the closest approach point (in world space) to a given position.
    // If there are no approach points or owner is missing, returns the input "from".
    Math::Vector2D GetClosestApproachPoint(Scene& scene,
        const Math::Vector2D& from) const;

protected:
    static constexpr int kInvalidID = -1;

    int heldItemID_;
    std::vector<Math::Vector2D> approachOffsets_;

    // Derived classes can react when an item is placed or taken.
    // For example:
    // - Work table: start processing timer when an ingredient is placed.
    // - Customer table: check if the dish is complete, notify customer, etc.
    virtual void OnItemPlaced(Scene& /*scene*/, GameObject& /*item*/) {}
    virtual void OnItemTaken(Scene& /*scene*/, GameObject& /*item*/) {}

    // Convenience to get the table GameObject (owner) with null-check already done.
    GameObject* GetOwnerChecked(Scene& scene) const;

    std::string GetName() const override { return "TableLogic"; }
};
