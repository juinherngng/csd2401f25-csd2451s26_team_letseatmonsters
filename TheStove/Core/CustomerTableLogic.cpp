/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerTableLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Implements behaviour for a dining table that can seat a
                    customer. Handles table state (occupied/free), seat
                    positions, communication with CustomerManagerLogic, and
                    interactions such as placing dishes or checking whether a
                    customer is served.


         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "CustomerTableLogic.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Core/LogicManager.hpp"   
#include "../Core/SimpleNpcLogic.hpp" 
#include "../Core/Quota.hpp"
#include <cmath>

CustomerTableLogic::CustomerTableLogic(int ownerID)
    : TableLogic(ownerID)
    , seatedCustomerID_(kInvalidID)
{
    //ClearApproachOffsets();

    //AddApproachOffset(Math::Vector2D(0.0f, -140.0f));
}

void CustomerTableLogic::Start(Scene& scene)
{
    // Base TableLogic will:
    //  - reset heldItemID_
    //  - (currently) set the player approach offset from defs.approachOffset
    TableLogic::Start(scene);

    seatedCustomerID_ = kInvalidID;

    servedFoodLocked_ = false;
    servedFoodItemID_ = kInvalidID;

    GameObject* owner = GetOwner(scene);
    if (!owner)
        return;

    const int id = owner->GetID();
    Scene::Defaults defs = scene.GetDefaults(id);

    // We interpret defs.approachOffset as the CUSTOMER seat offset.
    if (defs.approachOffset.x != 0.0f || defs.approachOffset.y != 0.0f)
    {
        // Customer sits at the authored offset (e.g. 90, 0)
        customerSeatOffset_ = Math::Vector2D(defs.approachOffset.x,
            defs.approachOffset.y);

        // PLAYER approach is mirrored across the table center (e.g. -90, 0)
        Math::Vector2D playerOffset(-defs.approachOffset.x,
            -defs.approachOffset.y);

        // Override the base TableLogic approach point for this table
        SetSingleApproachOffset(playerOffset);

        std::cout << "[CustomerTableLogic] ownerID=" << GetOwnerID()
            << " customerSeatOffset=(" << customerSeatOffset_.x << ", " << customerSeatOffset_.y << ")"
            << " playerApproachOffset=(" << playerOffset.x << ", " << playerOffset.y << ")\n";
    }
    else
    {
        // Fallback if nothing authored: simple defaults
        customerSeatOffset_ = Math::Vector2D(0.0f, -90.0f);   // customer just "above"
        SetSingleApproachOffset(Math::Vector2D(0.0f, 90.0f)); // player "below"
    }
}

void CustomerTableLogic::OnDestroy(Scene& scene)
{
    // If needed, external systems can query that the table is now free.
    seatedCustomerID_ = kInvalidID;
    servedFoodLocked_ = false;
    servedFoodItemID_ = kInvalidID;
    TableLogic::OnDestroy(scene);
}

bool CustomerTableLogic::SeatCustomer(int customerID)
{
    if (HasSeatedCustomer())
        return false;

    seatedCustomerID_ = customerID;
    return true;
}

void CustomerTableLogic::ClearCustomer()
{
    seatedCustomerID_ = kInvalidID;
}

bool CustomerTableLogic::CanAcceptItem(Scene& scene, int itemID) const
{
    // First, respect base rules (table must be empty, item must exist).
    if (!TableLogic::CanAcceptItem(scene, itemID))
        return false;

    // Require a seated customer to serve.
    if (!HasSeatedCustomer())
        return false;

    GameObject* item = scene.GetGameObjectByID(itemID);
    if (!item)
        return false;

    // Only accept completed dishes.
    return IsCompletedDish(scene, *item);
}

bool CustomerTableLogic::IsCompletedDish(Scene& scene, const GameObject& item) const
{
    LogicManager& logicMgr = scene.GetLogicManager();
    auto* plate = logicMgr.GetLogicForObject<PlateLogic>(item.GetID());
    if (!plate) return false;
    return plate->HasPreparedDish();
}

void CustomerTableLogic::OnItemPlaced(Scene& scene, GameObject& item)
{
    // When a dish is placed and there is a seated customer,
    // treat this as "dish served".
    if (HasSeatedCustomer() && IsCompletedDish(scene, item))
    {
        OnDishServed(scene, item);
    }
}

void CustomerTableLogic::OnItemTaken(Scene& /*scene*/, GameObject& /*item*/)
{
    // For now, do nothing special when the player takes back the dish.
    // You could add logic here later (e.g., cancel serving).
}

void CustomerTableLogic::OnDishServed(Scene& scene, GameObject& dish)
{
    if (!HasSeatedCustomer()) return;

    servedFoodLocked_ = true;
    servedFoodItemID_ = dish.GetID();

    LogicManager& logicMgr = scene.GetLogicManager();

    auto* plate = logicMgr.GetLogicForObject<PlateLogic>(dish.GetID());
    auto* customerLogic = logicMgr.GetLogicForObject<SimpleNpcLogic>(seatedCustomerID_);

    if (!plate || !customerLogic) return;
    if (!plate->HasPreparedDish()) return;

    DishType servedType = plate->GetDishType();

    std::cout << "[CustomerTable] Serve dishType=" << (int)servedType
        << " desired=" << (int)customerLogic->GetDesiredDishType() << "\n";

    customerLogic->OnDishServed(scene, servedType);

    // Optional: if accepted (customer started eating), clear plate so it can be reused
    if (customerLogic->IsEating()) {
        plate->ClearPreparedDish();

        // also reset plate sprite if you want
        // scene.SetObjectTexturePath(dish.GetID(), "../assets/Plate.png");
    }
}


bool CustomerTableLogic::CanServeFromPlate(const PlateLogic& plate) const
{
    // Must have a seated customer.
    if (!HasSeatedCustomer())
        return false;

    // Plate must contain a prepared dish.
    if (!plate.HasPreparedDish())
        return false;

    return true;
}

void CustomerTableLogic::OnPlateServed(const PlateLogic& plate)
{
    // Base implementation: do nothing.
    // Later:
    //  - Notify the customer logic that dish type = plate.GetDishType().
    //  - Start "eating" behaviour, patience reset, etc.
    (void)plate;
}

Math::Vector2D CustomerTableLogic::GetCustomerSeatWorld(Scene& scene) const
{
    GameObject* owner = GetOwner(scene);
    if (!owner) {
        return Math::Vector2D(0.0f, 0.0f);
    }

    Math::Vector3D pos3 = owner->GetPosition();
    return Math::Vector2D(pos3.x + customerSeatOffset_.x,
        pos3.y + customerSeatOffset_.y);
}

bool CustomerTableLogic::TryTakePayment(Scene& scene)
{
    if (!HasSeatedCustomer()) {
        return false;
    }

    LogicManager& logicMgr = scene.GetLogicManager();
    SimpleNpcLogic* customerLogic =
        logicMgr.GetLogicForObject<SimpleNpcLogic>(seatedCustomerID_);

    if (!customerLogic) {
        std::cout << "[CustomerTableLogic] TryTakePayment: no SimpleNpcLogic on customerID="
            << seatedCustomerID_ << "\n";
        return false;
    }

    if (!customerLogic->IsPaying()) {
        return false; // only take payment in Paying state
    }

    // correct dish => pay kCorrectDishPay
    // wrong dish OR patience timeout => pay 0
    // -------------------------------------------------------
    int payment = 0;

    // If customer logic says "pay $0", override everything
    if (!customerLogic->WillPayZero())
    {
        // Only pay full amount if they were served the correct dish
        if (customerLogic->GetServedDishType() == customerLogic->GetDesiredDishType())
        {
            payment = static_cast<int>(Economy::kCorrectDishPay * (1.0f + std::clamp(customerLogic->GetPatienceRatioAtServe(), 0.0f, 1.0f)));
        }
    }

    Economy::AddMoney(scene, payment);


    std::cout << "[CustomerTableLogic] Payment amount=" << payment
        << " totalMoney=" << Economy::gPlayerMoney
        << " quota=" << Economy::kQuota << "\n";

    // Player successfully takes payment -> customer becomes Leaving
    customerLogic->TakePayment(scene);

    std::cout << "[CustomerTableLogic] Payment taken! customer=" << seatedCustomerID_ << "\n";

    return true;
}

void CustomerTableLogic::ClearServedFood(Scene& scene)
{

    // Unlock first
    servedFoodLocked_ = false;
    servedFoodItemID_ = kInvalidID;

    // TakeItem() clears heldItemID_ immediately and calls OnItemTaken(...)
    const int itemID = TakeItem(scene);
    if (itemID != kInvalidID)
    {
        scene.RequestDespawn(itemID);   // dish/plate disappears
        // std::cout << "[CustomerTableLogic] Cleared served food item " << itemID << "\n";
    }
}

int CustomerTableLogic::TakeItem(Scene& scene)
{
    // If food has been served on this table, don't allow taking it
    if (servedFoodLocked_) {
        return kInvalidID;
    }

    return TableLogic::TakeItem(scene);
}
