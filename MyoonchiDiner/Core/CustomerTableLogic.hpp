/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerTableLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Declares the CustomerTableLogic class, representing a
					table that can seat a customer. Exposes API for checking
					occupancy, assigning customers, and retrieving seat
					transforms used by the AI.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

 // CustomerTableLogic
 //  - Table where customers can sit and be served dishes.
 //  - Holds exactly one item (via TableLogic).
 //  - Only accepts fully assembled dishes (logic stubbed for now).
 //  - Tracks a single seated customer by GameObject ID.
 //
 // Customer AI later can:
 //   - Call SeatCustomer / ClearCustomer.
 //   - React to OnDishServed via overriding or querying the table state.
#pragma once

#include "PlateLogic.hpp"
#include "TableLogic.hpp"
#include <array>
#include <algorithm>

class CustomerTableLogic : public TableLogic {
public:
	explicit CustomerTableLogic(int ownerID);

	void Start(Scene& scene) override;
	void OnDestroy(Scene& scene) override;

	// Block taking served food
	int TakeItem(Scene& scene) override;

	// ---- Seating control ----

	bool HasSeatedCustomer() const {
		return GetSeatedCustomerCount() > 0;
	}
	int GetSeatedCustomerCount() const;

	int GetSeatCapacity() const {
		return seatCapacity_;
	}

	bool IsAvailableForSeating() const {
		return GetSeatedCustomerCount() < seatCapacity_;
	}

	int GetSeatedCustomerID() const {
		for (int i = 0; i < seatCapacity_; ++i) {
			if (seatedCustomerIDs_[i] != kInvalidID) {
				return seatedCustomerIDs_[i];
			}
		}
		return kInvalidID;
	}

	const std::array<int, 2>& GetSeatedCustomerIDs() const {
		return seatedCustomerIDs_;
	}

	// Seat a customer at this table. Returns false if already occupied.
	bool SeatCustomer(Scene& scene, int customerID, Math::Vector2D* outSeatWorld = nullptr);

	// Clear the currently seated customer (if any).
	bool ClearCustomer(int customerID);
	void ClearAllCustomers();

	// Local offset from table position where the customer should stand/sit
	void SetCustomerSeatOffset(const Math::Vector2D& localOffset) {
		customerSeatOffsets_[0] = localOffset;
	}
	void SetCustomerSeatOffset2(const Math::Vector2D& localOffset) {
		customerSeatOffsets_[1] = localOffset;
	}
	void SetSeatCapacity(int capacity) {
		seatCapacity_ = std::clamp(capacity, 1, 2);
	}

	Math::Vector2D GetCustomerSeatWorld(Scene& scene, int customerID) const;
	Math::Vector2D GetCustomerSeatWorldByIndex(Scene& scene, int seatIndex) const;

	// ---- TableLogic overrides ----

	// Only accept items that are "completed dishes" and when the table is empty.
	bool CanAcceptItem(Scene& scene, int itemID) const override;

	// High-level helper: given a plate logic that the player is placing on this table,
	// decide if it is a valid dish to serve.
	bool CanServeFromPlate(const PlateLogic& plate) const;

	// Optional hook to be called when the plate is actually placed and accepted.
	void OnPlateServed(const PlateLogic& plate);

	//Try Take Payment
	bool TryTakePayment(Scene& scene);

	// Remove the served dish/plate from this table (despawn it and free the table slot).
	void ClearServedFood(Scene& scene);

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

	int FindSeatIndexByCustomerID(int customerID) const;
	int FindFirstFreeSeatIndex() const;
	int FindBestCustomerForDish(Scene& scene, DishType dishType) const;
	int FindFirstPayingCustomer(Scene& scene) const;

	int seatCapacity_ = 1;
	std::array<int, 2> seatedCustomerIDs_{ { kInvalidID, kInvalidID } };
	std::array<Math::Vector2D, 2> customerSeatOffsets_{ {
		Math::Vector2D(0.0f, -90.0f),
		Math::Vector2D(40.0f, -90.0f)
	} };

	std::string GetName() const override {
		return "CustomerTableLogic";
	}

	bool servedFoodLocked_ = false;
	int  servedFoodItemID_ = kInvalidID;
};

