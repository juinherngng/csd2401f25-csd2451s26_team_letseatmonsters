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

#include "Core/AudioManager.hpp"
#include "Core/LogicManager.hpp"
#include "Core/Quota.hpp"
#include "Core/SimpleNpcLogic.hpp"
#include "CustomerTableLogic.hpp"
#include "Graphics/GameObject.hpp"
#include "Graphics/SceneManager.hpp"

#include <algorithm>
#include <cmath>

CustomerTableLogic::CustomerTableLogic(int ownerID)
	: TableLogic(ownerID) {
}

void CustomerTableLogic::Start(Scene& scene) {
	TableLogic::Start(scene);

	seatedCustomerIDs_.fill(kInvalidID);
	servedFoodLocked_ = false;
	servedFoodItemID_ = kInvalidID;

	GameObject* owner = GetOwner(scene);
	if (!owner)
		return;

	const int id = owner->GetID();
	Scene::Defaults defs = scene.GetDefaults(id);

	seatCapacity_ = std::clamp(defs.customerSeatCapacity, 1, 2);

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
		customerSeatOffsets_[1] = Math::Vector2D(
			defs.customerSeatOffset2.x,
			defs.customerSeatOffset2.y
		);
	}
	else {
		// Auto-derive second seat if not authored
		if (std::abs(seat0.x) > 0.01f) {
			customerSeatOffsets_[1] = Math::Vector2D(-seat0.x, seat0.y);
		}
		else {
			customerSeatOffsets_[0] = Math::Vector2D(-40.0f, seat0.y);
			customerSeatOffsets_[1] = Math::Vector2D(40.0f, seat0.y);
		}
	}

	if (approachOffsets_.empty()) {
		SetSingleApproachOffset(Math::Vector2D(-seat0.x, -seat0.y));
	}
}

void CustomerTableLogic::OnDestroy(Scene& scene) {
	ClearAllCustomers();
	servedFoodLocked_ = false;
	servedFoodItemID_ = kInvalidID;
	TableLogic::OnDestroy(scene);
}

int CustomerTableLogic::GetSeatedCustomerCount() const {
	int count = 0;
	for (int id : seatedCustomerIDs_) {
		if (id != kInvalidID) {
			++count;
		}
	}
	return count;
}

int CustomerTableLogic::FindSeatIndexByCustomerID(int customerID) const {
	for (int i = 0; i < seatCapacity_; ++i) {
		if (seatedCustomerIDs_[i] == customerID) {
			return i;
		}
	}
	return -1;
}

int CustomerTableLogic::FindFirstFreeSeatIndex() const {
	for (int i = 0; i < seatCapacity_; ++i) {
		if (seatedCustomerIDs_[i] == kInvalidID) {
			return i;
		}
	}
	return -1;
}

bool CustomerTableLogic::SeatCustomer(Scene& scene, int customerID, Math::Vector2D* outSeatWorld) {
	if (customerID == kInvalidID) {
		return false;
	}

	int existing = FindSeatIndexByCustomerID(customerID);
	if (existing >= 0) {
		if (outSeatWorld) {
			*outSeatWorld = GetCustomerSeatWorldByIndex(scene, existing);
		}
		return true;
	}

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

bool CustomerTableLogic::ClearCustomer(int customerID) {
	int seat = FindSeatIndexByCustomerID(customerID);
	if (seat < 0) {
		return false;
	}

	seatedCustomerIDs_[seat] = kInvalidID;

	// Safety: if nobody is seated anymore, never keep the table food-locked.
	if (!HasSeatedCustomer()) {
		servedFoodLocked_ = false;
		servedFoodItemID_ = kInvalidID;
	}

	return true;
}

void CustomerTableLogic::ClearAllCustomers() {
	seatedCustomerIDs_.fill(kInvalidID);
	servedFoodLocked_ = false;
	servedFoodItemID_ = kInvalidID;
}

Math::Vector2D CustomerTableLogic::GetCustomerSeatWorldByIndex(Scene& scene, int seatIndex) const {
	GameObject* owner = GetOwner(scene);
	if (!owner || seatIndex < 0 || seatIndex >= seatCapacity_) {
		return Math::Vector2D(0.0f, 0.0f);
	}

	Math::Vector3D pos3 = owner->GetPosition();
	return Math::Vector2D(
		pos3.x + customerSeatOffsets_[seatIndex].x,
		pos3.y + customerSeatOffsets_[seatIndex].y
	);
}

Math::Vector2D CustomerTableLogic::GetCustomerSeatWorld(Scene& scene, int customerID) const {
	int seat = FindSeatIndexByCustomerID(customerID);
	if (seat < 0) {
		return GetCustomerSeatWorldByIndex(scene, 0);
	}
	return GetCustomerSeatWorldByIndex(scene, seat);
}

bool CustomerTableLogic::CanAcceptItem(Scene& scene, int itemID) const {
	if (!TableLogic::CanAcceptItem(scene, itemID))
		return false;

	if (!HasSeatedCustomer())
		return false;

	LogicManager& logicMgr = scene.GetLogicManager();
	auto* plate = logicMgr.GetLogicForObject<PlateLogic>(itemID);
	if (!plate || !plate->HasPreparedDish())
		return false;

	// Do not allow placing food if nobody at this table can still receive it.
	return FindBestCustomerForDish(scene, plate->GetDishType()) != kInvalidID;
}

bool CustomerTableLogic::IsCompletedDish(Scene& scene, const GameObject& item) const {
	LogicManager& logicMgr = scene.GetLogicManager();
	auto* plate = logicMgr.GetLogicForObject<PlateLogic>(item.GetID());
	if (!plate) return false;
	return plate->HasPreparedDish();
}

int CustomerTableLogic::FindBestCustomerForDish(Scene& scene, DishType dishType) const {
	LogicManager& logicMgr = scene.GetLogicManager();

	// First pass: exact matching order
	for (int customerID : seatedCustomerIDs_) {
		if (customerID == kInvalidID) continue;
		auto* customer = logicMgr.GetLogicForObject<SimpleNpcLogic>(customerID);
		if (!customer) continue;
		if (!customer->IsWaitingForFood()) continue;
		if (customer->GetDesiredDishType() == dishType) {
			return customerID;
		}
	}

	// Second pass: any waiting customer
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

int CustomerTableLogic::FindFirstPayingCustomer(Scene& scene) const {
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

void CustomerTableLogic::OnItemPlaced(Scene& scene, GameObject& item) {
	if (HasSeatedCustomer() && IsCompletedDish(scene, item)) {
		OnDishServed(scene, item);
	}
}

void CustomerTableLogic::OnItemTaken(Scene& scene, GameObject& item) {
	(void)scene;
	(void)item;
}

void CustomerTableLogic::OnDishServed(Scene& scene, GameObject& dish) {
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
		audioMgr->PlaySound("sfx_serve_dish", audioMgr->GetVfxVolume());
	}

	// Lock only after we know this table has a valid receiver.
	servedFoodLocked_ = true;
	servedFoodItemID_ = dish.GetID();

	customerLogic->OnDishServed(scene, servedType);

	// Safety: if for any reason the customer did not actually accept the dish,
	// do not leave the table locked forever.
	if (!customerLogic->HasDishServed()) {
		servedFoodLocked_ = false;
		servedFoodItemID_ = kInvalidID;
		return;
	}

	if (customerLogic->IsEating()) {
		plate->ClearPreparedDish();
	}
}

bool CustomerTableLogic::CanServeFromPlate(const PlateLogic& plate) const {
	if (!HasSeatedCustomer())
		return false;

	if (!plate.HasPreparedDish())
		return false;

	return true;
}

void CustomerTableLogic::OnPlateServed(const PlateLogic& plate) {
	(void)plate;
}

bool CustomerTableLogic::TryTakePayment(Scene& scene) {
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

	if (!customerLogic->WillPayZero()) {
		if (customerLogic->GetServedDishType() == customerLogic->GetDesiredDishType()) {
			payment = static_cast<int>(
				Economy::kCorrectDishPay *
				(1.0f + std::clamp(customerLogic->GetPatienceRatioAtServe(), 0.0f, 1.0f))
				);
		}
	}

	Economy::AddMoney(scene, payment);

	if (GameObject* tableObj = GetOwner(scene)) {
		scene.TriggerCustomerPaymentFeedback(tableObj->GetID(), payment);
	}

	customerLogic->TakePayment(scene);
	return true;
}

void CustomerTableLogic::ClearServedFood(Scene& scene) {
	servedFoodLocked_ = false;
	servedFoodItemID_ = kInvalidID;

	const int itemID = TakeItem(scene);
	if (itemID != kInvalidID) {
		scene.RequestDespawn(itemID);
	}
}

int CustomerTableLogic::TakeItem(Scene& scene) {
	if (servedFoodLocked_) {
		return kInvalidID;
	}

	return TableLogic::TakeItem(scene);
}