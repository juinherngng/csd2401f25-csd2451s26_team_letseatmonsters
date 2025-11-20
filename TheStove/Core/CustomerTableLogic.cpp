#include "CustomerTableLogic.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Graphics/GameObject.hpp"

CustomerTableLogic::CustomerTableLogic(int ownerID)
    : TableLogic(ownerID)
    , seatedCustomerID_(kInvalidID)
{
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

void CustomerTableLogic::OnDishServed(Scene& /*scene*/, GameObject& /*dish*/)
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
