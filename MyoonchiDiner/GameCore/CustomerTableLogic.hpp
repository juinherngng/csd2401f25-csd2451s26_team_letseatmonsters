/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerTableLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (85%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (15%)

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

#include <algorithm>
#include <array>

#include "GameCore/PlateLogic.hpp"
#include "GameCore/TableLogic.hpp"

class CustomerTableLogic : public TableLogic {
public:
	/**
	 * @brief Constructs a customer-table controller for the specified owner object.
	 * @param ownerID Runtime ID of the table GameObject that owns this logic.
	 */
	explicit CustomerTableLogic(int ownerID);

	/**
	 * @brief Initializes customer-table runtime state after the scene is ready.
	 * @param scene Active scene containing the table.
	 */
	void Start(Scene& scene) override;

	/**
	 * @brief Cleans up seating state when the table is being destroyed.
	 * @param scene Active scene containing the table and any seated customers.
	 */
	void OnDestroy(Scene& scene) override;

	/**
	 * @brief Attempts to take the currently served item from the table.
	 * @param scene Active scene containing the table.
	 * @return Item ID if the take succeeds, or an invalid ID when the served food is locked.
	 */
	int TakeItem(Scene& scene) override;

	/**
	 * @brief Returns whether at least one customer is currently seated here.
	 * @return True when any seat is occupied.
	 */
	bool HasSeatedCustomer() const {
		// Treat any occupied seat as proof that this table currently has a seated customer.
		return GetSeatedCustomerCount() > 0;
	}

	/**
	 * @brief Counts how many seats are currently occupied.
	 * @return Number of seated customers.
	 */
	int GetSeatedCustomerCount() const;

	/**
	 * @brief Returns how many seats this table can support.
	 * @return Current seat capacity, clamped between one and two seats.
	 */
	int GetSeatCapacity() const {
		// Expose the authored runtime seat count so spawning and interaction logic can query it.
		return seatCapacity_;
	}

	/**
	 * @brief Returns whether another customer may be seated here.
	 * @return True when at least one seat is currently free.
	 */
	bool IsAvailableForSeating() const {
		// Availability is simply the difference between current occupancy and seat capacity.
		return GetSeatedCustomerCount() < seatCapacity_;
	}

	/**
	 * @brief Returns the first occupied seat's customer ID.
	 * @return Seated customer ID, or `kInvalidID` when no seats are occupied.
	 */
	int GetSeatedCustomerID() const {
		// Return the first occupied slot so older single-seat call sites still work.
		for (int i = 0; i < seatCapacity_; ++i) {
			if (seatedCustomerIDs_[i] != kInvalidID) {
				return seatedCustomerIDs_[i];
			}
		}
		return kInvalidID;
	}

	/**
	 * @brief Returns the raw seated-customer slot array.
	 * @return Array of seated customer IDs for both seat slots.
	 */
	const std::array<int, 2>& GetSeatedCustomerIDs() const {
		// Expose the full slot array for systems that need to inspect both seats directly.
		return seatedCustomerIDs_;
	}

	/**
	 * @brief Seats a customer at this table and optionally returns the resolved seat position.
	 * @param scene Active scene containing the table.
	 * @param customerID Runtime ID of the customer to seat.
	 * @param outSeatWorld Optional output for the resolved world-space seat position.
	 * @return True if the customer was successfully assigned to a free seat.
	 */
	bool SeatCustomer(Scene& scene, int customerID, Math::Vector2D* outSeatWorld = nullptr);

	/**
	 * @brief Clears a specific seated customer from this table.
	 * @param customerID Runtime ID of the customer to remove.
	 * @return True if the customer was occupying a seat on this table.
	 */
	bool ClearCustomer(int customerID);

	/**
	 * @brief Clears every seated customer slot on this table.
	 */
	void ClearAllCustomers();

	/**
	 * @brief Sets the local seat offset for the primary seat.
	 * @param localOffset Authored local offset from the table position.
	 */
	void SetCustomerSeatOffset(const Math::Vector2D& localOffset) {
		// Update the first seat anchor used when the table is configured as a single-seat table.
		customerSeatOffsets_[0] = localOffset;
	}

	/**
	 * @brief Sets the local seat offset for the secondary seat.
	 * @param localOffset Authored local offset from the table position.
	 */
	void SetCustomerSeatOffset2(const Math::Vector2D& localOffset) {
		// Update the second seat anchor used by the optional two-seat layout.
		customerSeatOffsets_[1] = localOffset;
	}

	/**
	 * @brief Sets how many seats this table exposes.
	 * @param capacity Requested seat capacity, clamped to the supported range.
	 */
	void SetSeatCapacity(int capacity) {
		// Clamp to the currently supported seat-count range for this table implementation.
		seatCapacity_ = std::clamp(capacity, 1, 2);
	}

	/**
	 * @brief Returns the world-space seat position assigned to a specific customer.
	 * @param scene Active scene containing the table.
	 * @param customerID Runtime ID of the seated customer.
	 * @return World-space seat position for that customer.
	 */
	Math::Vector2D GetCustomerSeatWorld(Scene& scene, int customerID) const;

	/**
	 * @brief Returns the world-space seat position for a specific seat slot.
	 * @param scene Active scene containing the table.
	 * @param seatIndex Zero-based seat slot index.
	 * @return World-space seat position for the requested seat.
	 */
	Math::Vector2D GetCustomerSeatWorldByIndex(Scene& scene, int seatIndex) const;

	/**
	 * @brief Returns whether the specified item may be placed on this table.
	 * @param scene Active scene containing the table and item.
	 * @param itemID Runtime ID of the item being tested.
	 * @return True if the item is a completed dish and the table can accept it.
	 */
	bool CanAcceptItem(Scene& scene, int itemID) const override;

	/**
	 * @brief Returns whether a given plate represents a valid customer dish.
	 * @param plate Plate logic being considered for service.
	 * @return True if the plate can be served to this table.
	 */
	bool CanServeFromPlate(const PlateLogic& plate) const;

	/**
	 * @brief Handles any follow-up state changes after a valid plate is served.
	 * @param plate Plate logic that was served to the table.
	 */
	void OnPlateServed(const PlateLogic& plate);

	/**
	 * @brief Attempts to collect payment from a customer seated at this table.
	 * @param scene Active scene containing the table and seated customers.
	 * @return True if payment was successfully taken.
	 */
	bool TryTakePayment(Scene& scene);

	/**
	 * @brief Removes the served dish from this table and frees the served-food slot.
	 * @param scene Active scene containing the served item.
	 */
	void ClearServedFood(Scene& scene);

protected:
	/**
	 * @brief Handles table-specific behavior after an item is placed here.
	 * @param scene Active scene containing the table.
	 * @param item Item that was placed on the table.
	 */
	void OnItemPlaced(Scene& scene, GameObject& item) override;

	/**
	 * @brief Handles table-specific behavior after an item is taken from here.
	 * @param scene Active scene containing the table.
	 * @param item Item that was taken from the table.
	 */
	void OnItemTaken(Scene& scene, GameObject& item) override;

	/**
	 * @brief Hook that runs when a completed dish is placed while a customer is seated.
	 * @param scene Active scene containing the table.
	 * @param dish Dish object that was just served.
	 */
	virtual void OnDishServed(Scene& scene, GameObject& dish);

	/**
	 * @brief Returns whether an item qualifies as a completed dish for this table.
	 * @param scene Active scene containing the table and item.
	 * @param item Item being tested.
	 * @return True if the item should be treated as a completed dish.
	 */
	virtual bool IsCompletedDish(Scene& scene, const GameObject& item) const;

	/**
	 * @brief Finds the seat slot index currently occupied by a given customer.
	 * @param customerID Runtime ID of the customer to search for.
	 * @return Zero-based seat index, or `-1` when the customer is not seated here.
	 */
	int FindSeatIndexByCustomerID(int customerID) const;

	/**
	 * @brief Finds the first currently unoccupied seat slot.
	 * @return Zero-based seat index, or `-1` when no seats are free.
	 */
	int FindFirstFreeSeatIndex() const;

	/**
	 * @brief Finds the best seated customer candidate for a served dish.
	 * @param scene Active scene containing the table and customers.
	 * @param dishType Dish type that was served.
	 * @return Runtime ID of the best matching customer, or an invalid ID if none match.
	 */
	int FindBestCustomerForDish(Scene& scene, DishType dishType) const;

	/**
	 * @brief Finds the first seated customer currently waiting to pay.
	 * @param scene Active scene containing the table and customers.
	 * @return Runtime ID of the first paying customer, or an invalid ID if none are paying.
	 */
	int FindFirstPayingCustomer(Scene& scene) const;

	int seatCapacity_ = 1;
	std::array<int, 2> seatedCustomerIDs_{ { kInvalidID, kInvalidID } };
	std::array<Math::Vector2D, 2> customerSeatOffsets_{ {
		Math::Vector2D(0.0f, -90.0f),
		Math::Vector2D(40.0f, -90.0f)
	} };

	/**
	 * @brief Returns the stable runtime logic name used by the engine.
	 * @return Name string for this logic component.
	 */
	std::string GetName() const override {
		// Keep the logic name stable for registration, debugging, and serialization helpers.
		return "CustomerTableLogic";
	}

	bool servedFoodLocked_ = false;
	int  servedFoodItemID_ = kInvalidID;
};
