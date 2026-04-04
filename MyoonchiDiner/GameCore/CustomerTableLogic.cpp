/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerTableLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (90%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (10%)

 DESCRIPTION:       Implements behaviour for a dining table that can seat a
					customer. Handles table state (occupied/free), seat
					positions, communication with CustomerManagerLogic, and
					interactions such as placing dishes or checking whether a
					customer is served.


		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <cmath>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/LogicManager.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/CustomerTableLogic.hpp"
#include "GameCore/Quota.hpp"
#include "GameCore/SimpleNpcLogic.hpp"

CustomerTableLogic::CustomerTableLogic(int ownerID)
	: TableLogic(ownerID) {}

/**
 * @brief Initializes seating, served-food state, and authored seat offsets for the table.
 * @param scene Active scene containing the table object.
 */
void CustomerTableLogic::Start(Scene& scene) {
	// Let the base table logic initialize its shared item-slot state first.
	TableLogic::Start(scene);

	// Reset seat occupancy and served-food tracking every time the table starts.
	seatedCustomerIDs_.fill(kInvalidID);
	servedFoodLocked_ = false;
	servedFoodItemID_ = kInvalidID;

	// Abort cleanly if the table object no longer exists in the scene.
	GameObject* owner = GetOwner(scene);
	if (!owner)
		return;

	const int id = owner->GetID();
	Scene::Defaults defs = scene.GetDefaults(id);

	seatCapacity_ = std::clamp(defs.customerSeatCapacity, 1, 2);

	// Prefer explicit authored customer-seat offsets, then fall back to older approach offsets.
	Math::Vector2D seat0(0.0f, -90.0f);
	if (defs.hasCustomerSeatOffset) {
		seat0 = Math::Vector2D(defs.customerSeatOffset.x, defs.customerSeatOffset.y);
	}
	else if (defs.hasApproachOffset2) {
		seat0 = Math::Vector2D(defs.approachOffset2.x, defs.approachOffset2.y);
	}
	else if (defs.approachOffset.x != 0.0f || defs.approachOffset.y != 0.0f) {
		seat0 = Math::Vector2D(defs.approachOffset.x, defs.approachOffset.y);
	}

	customerSeatOffsets_[0] = seat0;

	if (defs.hasCustomerSeatOffset2) {
		// Use the authored second-seat offset when the level data provides one.
		customerSeatOffsets_[1] = Math::Vector2D(
			defs.customerSeatOffset2.x,
			defs.customerSeatOffset2.y
		);
	}
	else {
		// Auto-derive a mirrored second seat when the level author only supplied one seat offset.
		if (std::abs(seat0.x) > 0.01f) {
			customerSeatOffsets_[1] = Math::Vector2D(-seat0.x, seat0.y);
		}
		else {
			customerSeatOffsets_[0] = Math::Vector2D(-40.0f, seat0.y);
			customerSeatOffsets_[1] = Math::Vector2D(40.0f, seat0.y);
		}
	}

	// Provide a sensible interaction approach if the table has no authored approach offsets yet.
	if (approachOffsets_.empty()) {
		SetSingleApproachOffset(Math::Vector2D(-seat0.x, -seat0.y));
	}
}

/**
 * @brief Clears seating state and tears down shared table state during destruction.
 * @param scene Active scene containing the table object.
 */
void CustomerTableLogic::OnDestroy(Scene& scene) {
	// Drop any remembered seated customers before the base table cleanup runs.
	ClearAllCustomers();
	servedFoodLocked_ = false;
	servedFoodItemID_ = kInvalidID;
	TableLogic::OnDestroy(scene);
}

/**
 * @brief Counts how many seats are currently occupied.
 * @return Number of seated customers.
 */
int CustomerTableLogic::GetSeatedCustomerCount() const {
	// Count every occupied slot in the fixed-size seat array.
	int count = 0;
	for (int id : seatedCustomerIDs_) {
		if (id != kInvalidID) {
			++count;
		}
	}
	return count;
}

/**
 * @brief Finds the seat slot currently occupied by a specific customer.
 * @param customerID Runtime ID of the customer to search for.
 * @return Zero-based seat index, or `-1` when the customer is not seated here.
 */
int CustomerTableLogic::FindSeatIndexByCustomerID(int customerID) const {
	// Scan only the active seat-capacity range so disabled seats are ignored.
	for (int i = 0; i < seatCapacity_; ++i) {
		if (seatedCustomerIDs_[i] == customerID) {
			return i;
		}
	}
	return -1;
}

/**
 * @brief Finds the first unoccupied seat slot.
 * @return Zero-based seat index, or `-1` when no seats are free.
 */
int CustomerTableLogic::FindFirstFreeSeatIndex() const {
	// Return the first open slot so customers fill seats deterministically.
	for (int i = 0; i < seatCapacity_; ++i) {
		if (seatedCustomerIDs_[i] == kInvalidID) {
			return i;
		}
	}
	return -1;
}

/**
 * @brief Seats a customer at this table and optionally returns the resolved seat position.
 * @param scene Active scene containing the table object.
 * @param customerID Runtime ID of the customer to seat.
 * @param outSeatWorld Optional output for the resolved world-space seat position.
 * @return True if the customer was already seated or successfully assigned to a free seat.
 */
bool CustomerTableLogic::SeatCustomer(Scene& scene, int customerID, Math::Vector2D* outSeatWorld) {
	// Reject invalid IDs immediately so callers do not reserve phantom seats.
	if (customerID == kInvalidID) {
		return false;
	}

	// Reuse the existing seat assignment if this customer is already seated here.
	int existing = FindSeatIndexByCustomerID(customerID);
	if (existing >= 0) {
		if (outSeatWorld) {
			*outSeatWorld = GetCustomerSeatWorldByIndex(scene, existing);
		}
		return true;
	}

	// Claim the first available seat slot for a newly arriving customer.
	int freeSeat = FindFirstFreeSeatIndex();
	if (freeSeat < 0) {
		return false;
	}

	seatedCustomerIDs_[freeSeat] = customerID;

	if (outSeatWorld) {
		*outSeatWorld = GetCustomerSeatWorldByIndex(scene, freeSeat);
	}
	return true;
}

/**
 * @brief Clears a specific seated customer from this table.
 * @param customerID Runtime ID of the customer to remove.
 * @return True if the customer was occupying a seat on this table.
 */
bool CustomerTableLogic::ClearCustomer(int customerID) {
	// Only clear seats that currently belong to the requested customer.
	int seat = FindSeatIndexByCustomerID(customerID);
	if (seat < 0) {
		return false;
	}

	seatedCustomerIDs_[seat] = kInvalidID;

	// Release any food lock when the last customer leaves so the table cannot get stuck.
	if (!HasSeatedCustomer()) {
		servedFoodLocked_ = false;
		servedFoodItemID_ = kInvalidID;
	}

	return true;
}

/**
 * @brief Clears every seated customer slot on this table.
 */
void CustomerTableLogic::ClearAllCustomers() {
	// Reset the full seat array and any served-food lock in one step.
	seatedCustomerIDs_.fill(kInvalidID);
	servedFoodLocked_ = false;
	servedFoodItemID_ = kInvalidID;
}

/**
 * @brief Returns the world-space position for a specific seat slot.
 * @param scene Active scene containing the table object.
 * @param seatIndex Zero-based seat slot index.
 * @return World-space seat position for the requested slot.
 */
Math::Vector2D CustomerTableLogic::GetCustomerSeatWorldByIndex(Scene& scene, int seatIndex) const {
	// Guard against missing owners and invalid seat indices before reading offsets.
	GameObject* owner = GetOwner(scene);
	if (!owner || seatIndex < 0 || seatIndex >= seatCapacity_) {
		return Math::Vector2D(0.0f, 0.0f);
	}

	// Convert the authored local seat offset into world space using the table position.
	Math::Vector3D pos3 = owner->GetPosition();
	return Math::Vector2D(
		pos3.x + customerSeatOffsets_[seatIndex].x,
		pos3.y + customerSeatOffsets_[seatIndex].y
	);
}

/**
 * @brief Returns the world-space seat position assigned to a specific customer.
 * @param scene Active scene containing the table object.
 * @param customerID Runtime ID of the seated customer.
 * @return World-space seat position for that customer, or the first seat as a fallback.
 */
Math::Vector2D CustomerTableLogic::GetCustomerSeatWorld(Scene& scene, int customerID) const {
	// Fall back to the first seat when the requested customer is not currently seated here.
	int seat = FindSeatIndexByCustomerID(customerID);
	if (seat < 0) {
		return GetCustomerSeatWorldByIndex(scene, 0);
	}
	return GetCustomerSeatWorldByIndex(scene, seat);
}

/**
 * @brief Returns whether the specified item may be placed on this table.
 * @param scene Active scene containing the table and item.
 * @param itemID Runtime ID of the item being tested.
 * @return True if the item is a prepared plate that can serve at least one seated customer.
 */
bool CustomerTableLogic::CanAcceptItem(Scene& scene, int itemID) const {
	// Start with the base table checks before applying customer-table-specific rules.
	if (!TableLogic::CanAcceptItem(scene, itemID))
		return false;

	// Customer tables only accept service items while at least one diner is seated.
	if (!HasSeatedCustomer())
		return false;

	LogicManager& logicMgr = scene.GetLogicManager();
	auto* plate = logicMgr.GetLogicForObject<PlateLogic>(itemID);
	if (!plate || !plate->HasPreparedDish())
		return false;

	// Only allow the dish when at least one seated customer can still receive it.
	return FindBestCustomerForDish(scene, plate->GetDishType()) != kInvalidID;
}

/**
 * @brief Returns whether an item qualifies as a completed dish for this table.
 * @param scene Active scene containing the item logic.
 * @param item Item being tested.
 * @return True if the item owns a prepared plate logic.
 */
bool CustomerTableLogic::IsCompletedDish(Scene& scene, const GameObject& item) const {
	// Treat prepared plates as the canonical representation of completed dishes.
	LogicManager& logicMgr = scene.GetLogicManager();
	auto* plate = logicMgr.GetLogicForObject<PlateLogic>(item.GetID());
	if (!plate) return false;
	return plate->HasPreparedDish();
}

/**
 * @brief Finds the best seated customer candidate for a served dish.
 * @param scene Active scene containing the seated customers.
 * @param dishType Dish type being served.
 * @return Runtime ID of the best matching customer, or `kInvalidID` when none qualify.
 */
int CustomerTableLogic::FindBestCustomerForDish(Scene& scene, DishType dishType) const {
	LogicManager& logicMgr = scene.GetLogicManager();

	// Prefer a customer whose outstanding order exactly matches the served dish.
	for (int customerID : seatedCustomerIDs_) {
		if (customerID == kInvalidID) continue;
		auto* customer = logicMgr.GetLogicForObject<SimpleNpcLogic>(customerID);
		if (!customer) continue;
		if (!customer->IsWaitingForFood()) continue;
		if (customer->GetDesiredDishType() == dishType) {
			return customerID;
		}
	}

	// Fall back to any waiting customer so generic service still has a receiver.
	for (int customerID : seatedCustomerIDs_) {
		if (customerID == kInvalidID) continue;
		auto* customer = logicMgr.GetLogicForObject<SimpleNpcLogic>(customerID);
		if (!customer) continue;
		if (customer->IsWaitingForFood()) {
			return customerID;
		}
	}

	return kInvalidID;
}

/**
 * @brief Finds the first seated customer currently waiting to pay.
 * @param scene Active scene containing the seated customers.
 * @return Runtime ID of the first paying customer, or `kInvalidID` if none are paying.
 */
int CustomerTableLogic::FindFirstPayingCustomer(Scene& scene) const {
	// Scan the occupied seats in order so payment collection remains deterministic.
	LogicManager& logicMgr = scene.GetLogicManager();

	for (int customerID : seatedCustomerIDs_) {
		if (customerID == kInvalidID) continue;
		auto* customer = logicMgr.GetLogicForObject<SimpleNpcLogic>(customerID);
		if (!customer) continue;
		if (customer->IsPaying()) {
			return customerID;
		}
	}

	return kInvalidID;
}

/**
 * @brief Handles table-specific behavior after an item is placed here.
 * @param scene Active scene containing the table.
 * @param item Item that was placed on the table.
 */
void CustomerTableLogic::OnItemPlaced(Scene& scene, GameObject& item) {
	// Forward completed dishes to the serving flow when diners are currently seated.
	if (HasSeatedCustomer() && IsCompletedDish(scene, item)) {
		OnDishServed(scene, item);
	}
}

/**
 * @brief Handles table-specific behavior after an item is taken from here.
 * @param scene Active scene containing the table.
 * @param item Item that was taken from the table.
 */
void CustomerTableLogic::OnItemTaken(Scene& scene, GameObject& item) {
	// This table currently has no extra teardown when an item is removed.
	(void)scene;
	(void)item;
}

/**
 * @brief Routes a served dish to the best matching seated customer.
 * @param scene Active scene containing the table, plate, and customers.
 * @param dish Dish object that was placed on the table.
 */
void CustomerTableLogic::OnDishServed(Scene& scene, GameObject& dish) {
	// Abort when there is no seated customer to receive the dish.
	if (!HasSeatedCustomer()) {
		return;
	}

	LogicManager& logicMgr = scene.GetLogicManager();

	auto* plate = logicMgr.GetLogicForObject<PlateLogic>(dish.GetID());
	if (!plate || !plate->HasPreparedDish()) {
		return;
	}

	const DishType servedType = plate->GetDishType();
	const int targetCustomerID = FindBestCustomerForDish(scene, servedType);
	if (targetCustomerID == kInvalidID) {
		return;
	}

	auto* customerLogic = logicMgr.GetLogicForObject<SimpleNpcLogic>(targetCustomerID);
	if (!customerLogic || !customerLogic->IsWaitingForFood()) {
		return;
	}

	if (AudioManager* audioMgr = scene.GetAudioManager()) {
		// Play the standard serve SFX once the table finds a valid dish receiver.
		audioMgr->PlaySound("sfx_serve_dish", audioMgr->GetVfxVolume());
	}

	// Lock the served food in place only after the table confirms it has a valid receiver.
	servedFoodLocked_ = true;
	servedFoodItemID_ = dish.GetID();

	customerLogic->OnDishServed(scene, servedType);

	// Release the lock if the customer rejected the dish so the table cannot stay stuck forever.
	if (!customerLogic->HasDishServed()) {
		servedFoodLocked_ = false;
		servedFoodItemID_ = kInvalidID;
		return;
	}

	if (customerLogic->IsEating()) {
		// Clear the prepared state once the customer transitions into eating immediately.
		plate->ClearPreparedDish();
	}
}

/**
 * @brief Returns whether a given plate represents a valid customer dish.
 * @param plate Plate logic being considered for service.
 * @return True if the plate is prepared and the table has a seated customer.
 */
bool CustomerTableLogic::CanServeFromPlate(const PlateLogic& plate) const {
	// Customer tables only serve prepared dishes while at least one diner is seated.
	if (!HasSeatedCustomer())
		return false;

	if (!plate.HasPreparedDish())
		return false;

	return true;
}

/**
 * @brief Handles any follow-up state changes after a valid plate is served.
 * @param plate Plate logic that was served to the table.
 */
void CustomerTableLogic::OnPlateServed(const PlateLogic& plate) {
	// This hook is reserved for future extensions, so the base implementation is intentionally empty.
	(void)plate;
}

/**
 * @brief Attempts to collect payment from a customer seated at this table.
 * @param scene Active scene containing the table and customers.
 * @return True if payment was successfully taken.
 */
bool CustomerTableLogic::TryTakePayment(Scene& scene) {
	// Refuse payment collection when nobody is seated or no one has reached the paying state yet.
	if (!HasSeatedCustomer()) {
		return false;
	}

	const int payingCustomerID = FindFirstPayingCustomer(scene);
	if (payingCustomerID == kInvalidID) {
		return false;
	}

	LogicManager& logicMgr = scene.GetLogicManager();
	SimpleNpcLogic* customerLogic =
		logicMgr.GetLogicForObject<SimpleNpcLogic>(payingCustomerID);

	if (!customerLogic) {
		return false;
	}

	int payment = 0;

	// Award money only when the served dish was correct and the customer is still willing to pay.
	if (!customerLogic->WillPayZero()) {
		if (customerLogic->GetServedDishType() == customerLogic->GetDesiredDishType()) {
			payment = static_cast<int>(
				Economy::kCorrectDishPay *
				(1.0f + std::clamp(customerLogic->GetPatienceRatioAtServe(), 0.0f, 1.0f))
				);
		}
	}

	// Apply the earned payment and trigger the usual table-side feedback.
	Economy::AddMoney(scene, payment);

	if (GameObject* tableObj = GetOwner(scene)) {
		scene.TriggerCustomerPaymentFeedback(tableObj->GetID(), payment);
	}

	customerLogic->TakePayment(scene);
	return true;
}

/**
 * @brief Removes the served dish from this table and frees the served-food slot.
 * @param scene Active scene containing the served item.
 */
void CustomerTableLogic::ClearServedFood(Scene& scene) {
	// Clear the lock before taking the item so the base table logic can release it normally.
	servedFoodLocked_ = false;
	servedFoodItemID_ = kInvalidID;

	// Despawn the served item if the table was still holding one.
	const int itemID = TakeItem(scene);
	if (itemID != kInvalidID) {
		scene.RequestDespawn(itemID);
	}
}

/**
 * @brief Attempts to take the currently served item from the table.
 * @param scene Active scene containing the table.
 * @return Item ID if the take succeeds, or `kInvalidID` when the served food is locked.
 */
int CustomerTableLogic::TakeItem(Scene& scene) {
	// Prevent the player from taking back food that is already being served to a customer.
	if (servedFoodLocked_) {
		return kInvalidID;
	}

	return TableLogic::TakeItem(scene);
}
