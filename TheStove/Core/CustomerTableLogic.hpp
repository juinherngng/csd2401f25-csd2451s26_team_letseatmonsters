#pragma once

#include "TableLogic.hpp"
#include "PlateLogic.hpp"

// CustomerTableLogic
//  - Table where customers can sit and be served dishes.
//  - Holds exactly one item (via TableLogic).
//  - Only accepts fully assembled dishes (logic stubbed for now).
//  - Tracks a single seated customer by GameObject ID.
//
// Customer AI later can:
//   - Call SeatCustomer / ClearCustomer.
//   - React to OnDishServed via overriding or querying the table state.
class CustomerTableLogic : public TableLogic
{
public:
    explicit CustomerTableLogic(int ownerID);

    void Start(Scene& scene) override;
    void OnDestroy(Scene& scene) override;

    // ---- Seating control ----

    bool HasSeatedCustomer() const { return seatedCustomerID_ != kInvalidID; }
    int  GetSeatedCustomerID() const { return seatedCustomerID_; }

    bool IsAvailableForSeating() const { return !HasSeatedCustomer(); }

    // Seat a customer at this table. Returns false if already occupied.
    bool SeatCustomer(int customerID);

    // Clear the currently seated customer (if any).
    void ClearCustomer();

    // Local offset from table position where the customer should stand/sit
    void SetCustomerSeatOffset(const Math::Vector2D& localOffset) {
        customerSeatOffset_ = localOffset;
    }

    Math::Vector2D GetCustomerSeatWorld(Scene& scene) const;

    // ---- TableLogic overrides ----

    // Only accept items that are "completed dishes" and when the table is empty.
    bool CanAcceptItem(Scene& scene, int itemID) const override;

    // High-level helper: given a plate logic that the player is placing on this table,
// decide if it is a valid dish to serve.
    bool CanServeFromPlate(const PlateLogic& plate) const;

    // Optional hook to be called when the plate is actually placed and accepted.
    void OnPlateServed(const PlateLogic& plate);


protected:
    void OnItemPlaced(Scene& scene, GameObject& item) override;
    void OnItemTaken(Scene& scene, GameObject& item) override;

    // Called when a completed dish is placed while a customer is seated.
    // Base implementation does nothing; a more specific subclass or external
    // system can override this or query the table state.
    virtual void OnDishServed(Scene& scene, GameObject& dish);

    // Base filter: is this item a completed dish?
    // For now this is a stub that always returns true; later you can wire this
    // to a DishLogic component or tag.
    //Only Completed Dish can be put on the customer table
    virtual bool IsCompletedDish(Scene& scene, const GameObject& item) const;

    int seatedCustomerID_;

    Math::Vector2D customerSeatOffset_{ 0.0f, 16.0f }; // e.g. in front of table

    std::string GetName() const override { return "CustomerTableLogic"; }
};
