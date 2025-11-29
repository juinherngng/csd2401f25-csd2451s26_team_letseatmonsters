#include "CustomerTableLogic.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Core/LogicManager.hpp"   
#include "../Core/SimpleNpcLogic.hpp" 

CustomerTableLogic::CustomerTableLogic(int ownerID)
    : TableLogic(ownerID)
    , seatedCustomerID_(kInvalidID)
{
    ClearApproachOffsets();

    AddApproachOffset(Math::Vector2D(0.0f, -140.0f));
}

void CustomerTableLogic::Start(Scene& scene)
{
    TableLogic::Start(scene);
    seatedCustomerID_ = kInvalidID;
}

void CustomerTableLogic::OnDestroy(Scene& scene)
{
    // If needed, external systems can query that the table is now free.
    seatedCustomerID_ = kInvalidID;
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

bool CustomerTableLogic::IsCompletedDish(Scene& /*scene*/, const GameObject& /*item*/) const
{
    // Stub implementation.
    //
    // Later you can change this to:
    //   - Check a DishLogic attached to the GameObject.
    //   - Or inspect some "DishType" / "isEaten" flags.
    //   - Or check a "Dish" tag.

    // PSEUDO: adapt this to however you fetch PlateLogic for a GameObject.
    // PlateLogic* plate = scene.GetLogicForObject<PlateLogic>(item.GetID());
    // if (!plate) return false;
    //
    // return CanServeFromPlate(*plate);
    return true;
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
    // Base implementation: do nothing.
    //
    // Example of how a more specific version could work in the future:
    //
    //   void RestaurantTableLogic::OnDishServed(Scene& scene, GameObject& dish) override {
    //       auto* customer = scene.GetLogic<CustomerLogic>(GetSeatedCustomerID());
    //       if (customer) {
    //           customer->OnDishServed(dish);
    //       }
    //   }
    (void)dish; // We’re not reading the plate contents yet.

    if (!HasSeatedCustomer())
        return;

    LogicManager& logicMgr = scene.GetLogicManager();

    // Find the customer logic attached to the seated NPC.
    SimpleNpcLogic* customerLogic =
        logicMgr.GetLogicForObject<SimpleNpcLogic>(seatedCustomerID_);

    if (!customerLogic) {
        std::cout << "[CustomerTableLogic] OnDishServed but no SimpleNpcLogic on customerID="
            << seatedCustomerID_ << "\n";
        return;
    }

    // For now: treat ANY completed dish placed here as the salad we wanted.
    // If you have real dish detection, plug it in here instead of hardcoding Salad.
    DishType servedType = DishType::VegDish; // TODO: map from plate/dish object.

    std::cout << "[CustomerTableLogic] Notifying customer " << seatedCustomerID_
        << " that dish type=" << static_cast<int>(servedType) << " is served.\n";

    customerLogic->OnDishServed(scene, servedType);
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

