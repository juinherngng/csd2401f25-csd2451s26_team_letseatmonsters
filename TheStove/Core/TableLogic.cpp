/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         TableLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung

DESCRIPTION:     Implements the base class TableLogic, providing shared behavior
                 for all table-type objects. This includes item placement and
                 pickup, approach-point calculation for NPCs/players, and generic
                 per-table interaction logic. Specialized tables (worktable,
                 customer table, ingredient box, etc.) inherit and override this.


         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/TableLogic.hpp"
#include "../Core/LogicManager.hpp"
#include "../Core/PlateLogic.hpp"
#include "../Graphics/SceneManager.hpp"   // for Scene interface (GetGameObjectByID, etc.)
#include "../Graphics/GameObject.hpp"
#include <iostream>

// ------------------- Constructor / lifecycle -------------------

TableLogic::TableLogic(int ownerID) : GameObjectLogic(ownerID), heldItemID_(kInvalidID)
{
    //// Default: one approach point directly "in front" of the table.
    //// You can tweak this later or add more via AddApproachOffset.
    //// (Assuming +Y is "up" visually; adjust sign if needed.)
    //ClearApproachOffsets();

    //AddApproachOffset(Math::Vector2D(0.0f, -110.0f));
}

void TableLogic::Start(Scene& scene)
{
    // Ensure clean state if this script is reused.
    heldItemID_ = kInvalidID;

    // If we have an owner, check if there is a per-instance offset
    // stored in Scene::Defaults.vel. If it's non-zero, use it.
    GameObject* owner = GetOwner(scene);
    if (!owner)
        return;

    owner->SetMovableByPhysics(false);

    Scene::Defaults def = scene.GetDefaults(GetOwnerID());

    // Only override if level/spawner actually set a non-zero vel.
    if (def.approachOffset.x != 0.0f || def.approachOffset.y != 0.0f)
    {
        SetSingleApproachOffset(Math::Vector2D(def.approachOffset.x, def.approachOffset.y));
    }
}

void TableLogic::OnDestroy(Scene& scene)
{
    // The table is going away; we don't delete the held item here.
    // We just clear the reference so nothing keeps a stale ID.
    if (HasItem())
    {
        GameObject* item = scene.GetGameObjectByID(heldItemID_);
        if (item)
        {
            OnItemTaken(scene, *item);
        }
    }

    heldItemID_ = kInvalidID;
}

// ------------------- Owner helper -------------------

GameObject* TableLogic::GetOwnerChecked(Scene& scene) const
{
    GameObject* owner = GetOwner(scene);
    // In production code you might log an error if owner is null.
    return owner;
}

// ------------------- Item occupancy -------------------

bool TableLogic::CanAcceptItem(Scene& scene, int itemID) const
{
    if (HasItem())
        return false;

    if (itemID == kInvalidID)
        return false;

    GameObject* item = scene.GetGameObjectByID(itemID);
    if (!item)
        return false;

    // Base table: accept anything as long as it's a valid object and table is empty.
    // - WorkTableLogic can override this to only accept raw/processed ingredients.
    // - CustomerTableLogic can override to only accept completed dishes.
    return true;
}

bool TableLogic::PlaceItem(Scene& scene, int itemID)
{
    if (!CanAcceptItem(scene, itemID))
    {
        //std::cout << "[TableLogic] PlaceItem FAIL: ownerID=" << GetOwnerID()
        //    << " itemID=" << itemID << " (CanAcceptItem == false)\n";
        return false;
    }

    GameObject* owner = GetOwnerChecked(scene);
    GameObject* item = scene.GetGameObjectByID(itemID);
    if (!owner || !item)
    {
        //std::cout << "[TableLogic] PlaceItem FAIL: missing owner or item\n";
        return false;
    }

    // Record that this table now holds this item.
    heldItemID_ = itemID;

    // Optionally snap the item onto the table top.
    // For now we just align the item's X/Y to the table's position.
    Math::Vector3D tablePos = owner->GetPosition();
    Math::Vector3D newPos(tablePos.x, tablePos.y, tablePos.z);
    item->SetPosition(newPos);
    // If this item is a plate with an attached ingredient visual, move it too.
    if (auto* plate = scene.GetLogicManager().GetLogicForObject<PlateLogic>(itemID))
    {
        const int child = plate->GetFirstIngredientObjectID();

        // Only relevant for "partial plate" visuals (not after dish assembled)
        if (child >= 0 && !plate->HasPreparedDish())
        {
            if (GameObject* ingObj = scene.GetGameObjectByID(child))
            {
                ingObj->SetPosition(newPos);
            }
        }
    }
    //std::cout << "[TableLogic] PlaceItem OK: ownerID=" << GetOwnerID()
    //    << " now holds itemID=" << itemID
    //    << " at pos=(" << tablePos.x << ", " << tablePos.y << ")\n";

    OnItemPlaced(scene, *item);
    return true;
}

int TableLogic::TakeItem(Scene& scene)
{
    if (!HasItem())
    {
        //std::cout << "[TableLogic] TakeItem: ownerID=" << GetOwnerID()
        //    << " but table is empty\n";
        return kInvalidID;
    }

    int resultID = heldItemID_;
    heldItemID_ = kInvalidID;

    GameObject* item = scene.GetGameObjectByID(resultID);
    if (item)
    {
        Math::Vector3D pos = item->GetPosition();
        //std::cout << "[TableLogic] TakeItem: ownerID=" << GetOwnerID()
        //    << " returning itemID=" << resultID
        //    << " from pos=(" << pos.x << ", " << pos.y << ")\n";
        OnItemTaken(scene, *item);
    }

    return resultID;
}

// ------------------- Approach / destination points -------------------

void TableLogic::AddApproachOffset(const Math::Vector2D& offset)
{
    approachOffsets_.push_back(offset);
}

void TableLogic::ClearApproachOffsets()
{
    approachOffsets_.clear();
}

std::vector<Math::Vector2D> TableLogic::GetApproachPointsWorld(Scene& scene) const
{
    std::vector<Math::Vector2D> result;

    GameObject* owner = GetOwner(scene);
    if (!owner)
    {
        //std::cout << "[TableLogic] GetApproachPointsWorld: owner is null (ownerID="
        //    << GetOwnerID() << ")\n";
        return result;
    }

    Math::Vector3D tablePos3D = owner->GetPosition();
    Math::Vector2D tablePos2D(tablePos3D.x, tablePos3D.y);

    //std::cout << "[TableLogic] GetApproachPointsWorld ownerID=" << GetOwnerID()
    //    << " tablePos=(" << tablePos2D.x << ", " << tablePos2D.y << ")\n";

    result.reserve(approachOffsets_.size());
    for (std::size_t i = 0; i < approachOffsets_.size(); ++i)
    {
        const Math::Vector2D& localOffset = approachOffsets_[i];
        Math::Vector2D worldPoint = tablePos2D + localOffset;
        result.push_back(worldPoint);

        //std::cout << "  [TableLogic] offset[" << i << "] local=("
        //    << localOffset.x << ", " << localOffset.y << ") -> world=("
        //    << worldPoint.x << ", " << worldPoint.y << ")\n";
    }

    return result;
}

Math::Vector2D TableLogic::GetClosestApproachPoint(Scene& scene,
    const Math::Vector2D& from) const
{
    std::vector<Math::Vector2D> worldPoints = GetApproachPointsWorld(scene);
    if (worldPoints.empty())
    {
        //// Fallback: if no approach points, just return the input position.
        //std::cout << "[TableLogic] GetClosestApproachPoint ownerID=" << GetOwnerID()
        //    << " has NO approach points, returning from=("
        //    << from.x << ", " << from.y << ")\n";
        return from;
    }

    float bestDistSq = std::numeric_limits<float>::max();
    Math::Vector2D bestPoint = worldPoints[0];
    std::size_t bestIndex = 0;

    for (std::size_t i = 0; i < worldPoints.size(); ++i)
    {
        const Math::Vector2D& p = worldPoints[i];
        Math::Vector2D diff = p - from;
        float distSq = diff.x * diff.x + diff.y * diff.y;
        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            bestPoint = p;
            bestIndex = i;
        }
    }

    //std::cout << "[TableLogic] GetClosestApproachPoint ownerID=" << GetOwnerID()
    //    << " from=(" << from.x << ", " << from.y << ") -> index=" << bestIndex
    //    << " world=(" << bestPoint.x << ", " << bestPoint.y
    //    << ") dist=" << std::sqrt(bestDistSq) << "\n";

    return bestPoint;
}
