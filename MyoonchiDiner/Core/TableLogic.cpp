/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         TableLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Implements the base class TableLogic, providing shared behavior
					for all table-type objects. This includes item placement and
					pickup, approach-point calculation for NPCs/players, and generic
					per-table interaction logic. Specialized tables (worktable,
					customer table, ingredient box, etc.) inherit and override this.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "Graphics/GameObject.hpp"
#include "Graphics/SceneManager.hpp"   // for Scene interface (GetGameObjectByID, etc.)

#include "PlateLogic.hpp"
#include "TableLogic.hpp"


 // ------------------- Constructor / lifecycle -------------------

TableLogic::TableLogic(int ownerID) : GameObjectLogic(ownerID), heldItemID_(kInvalidID) {
	//// Default: one approach point directly "in front" of the table.
	//// You can tweak this later or add more via AddApproachOffset.
	//// (Assuming +Y is "up" visually; adjust sign if needed.)
	//ClearApproachOffsets();

	//AddApproachOffset(Math::Vector2D(0.0f, -110.0f));
}

void TableLogic::Start(Scene& scene) {
	// Ensure clean state if this script is reused.
	heldItemID_ = kInvalidID;

	// If we have an owner, check if there is a per-instance offset
	// stored in Scene::Defaults.vel. If it's non-zero, use it.
	GameObject* owner = GetOwner(scene);
	if (!owner)
		return;

	owner->SetMovableByPhysics(false);

	Scene::Defaults def = scene.GetDefaults(GetOwnerID());

	// Load authored approach points from JSON defaults.
	ClearApproachOffsets();
	if (def.approachOffset.x != 0.0f || def.approachOffset.y != 0.0f) {
		AddApproachOffset(Math::Vector2D(def.approachOffset.x, def.approachOffset.y));
	}

	if (def.hasApproachOffset2 &&
		(def.approachOffset2.x != 0.0f || def.approachOffset2.y != 0.0f)) {
		AddApproachOffset(Math::Vector2D(def.approachOffset2.x, def.approachOffset2.y));
	}
}

void TableLogic::OnDestroy(Scene& scene) {
	// The table is going away; we don't delete the held item here.
	// We just clear the reference so nothing keeps a stale ID.
	if (HasItem()) {
		GameObject* item = scene.GetGameObjectByID(heldItemID_);
		if (item) {
			OnItemTaken(scene, *item);
		}
	}

	heldItemID_ = kInvalidID;
}

// ------------------- Owner helper -------------------

GameObject* TableLogic::GetOwnerChecked(Scene& scene) const {
	GameObject* owner = GetOwner(scene);
	// In production code you might log an error if owner is null.
	return owner;
}

// ------------------- Item occupancy -------------------

bool TableLogic::CanAcceptItem(Scene& scene, int itemID) const {
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

bool TableLogic::PlaceItem(Scene& scene, int itemID) {
	if (!CanAcceptItem(scene, itemID)) {
		return false;
	}

	GameObject* owner = GetOwnerChecked(scene);
	GameObject* item = scene.GetGameObjectByID(itemID);
	if (!owner || !item) {
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
	if (auto* plate = scene.GetLogicManager().GetLogicForObject<PlateLogic>(itemID)) {
		const int child = plate->GetFirstIngredientObjectID();

		// Only relevant for "partial plate" visuals (not after dish assembled)
		if (child >= 0 && !plate->HasPreparedDish()) {
			if (GameObject* ingObj = scene.GetGameObjectByID(child)) {
				ingObj->SetPosition(newPos);
			}
		}
	}
	OnItemPlaced(scene, *item);
	return true;
}

int TableLogic::TakeItem(Scene& scene) {
	if (!HasItem()) {
		return kInvalidID;
	}

	int resultID = heldItemID_;
	heldItemID_ = kInvalidID;

	GameObject* item = scene.GetGameObjectByID(resultID);
	if (item) {
		OnItemTaken(scene, *item);
	}

	return resultID;
}

// ------------------- Approach / destination points -------------------

void TableLogic::AddApproachOffset(const Math::Vector2D& offset) {
	approachOffsets_.push_back(offset);
}

void TableLogic::ClearApproachOffsets() {
	approachOffsets_.clear();
}

std::vector<Math::Vector2D> TableLogic::GetApproachPointsWorld(Scene& scene) const {
	std::vector<Math::Vector2D> result;

	GameObject* owner = GetOwner(scene);
	if (!owner) {
		return result;
	}

	Math::Vector3D tablePos3D = owner->GetPosition();
	Math::Vector2D tablePos2D(tablePos3D.x, tablePos3D.y);

	result.reserve(approachOffsets_.size());
	for (std::size_t i = 0; i < approachOffsets_.size(); ++i) {
		const Math::Vector2D& localOffset = approachOffsets_[i];
		Math::Vector2D worldPoint = tablePos2D + localOffset;
		result.push_back(worldPoint);
	}

	return result;
}

Math::Vector2D TableLogic::GetClosestApproachPoint(Scene& scene,
	const Math::Vector2D& from) const {
	std::vector<Math::Vector2D> worldPoints = GetApproachPointsWorld(scene);
	if (worldPoints.empty()) {
		return from;
	}

	float bestDistSq = std::numeric_limits<float>::max();
	Math::Vector2D bestPoint = worldPoints[0];
	for (std::size_t i = 0; i < worldPoints.size(); ++i) {
		const Math::Vector2D& p = worldPoints[i];
		Math::Vector2D diff = p - from;
		float distSq = diff.x * diff.x + diff.y * diff.y;
		if (distSq < bestDistSq) {
			bestDistSq = distSq;
			bestPoint = p;
		}
	}

	return bestPoint;
}

