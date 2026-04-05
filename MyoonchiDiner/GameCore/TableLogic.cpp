/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         TableLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (70%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (30%)

 DESCRIPTION:       Implements the base class TableLogic, providing shared behavior
					for all table-type objects. This includes item placement and
					pickup, approach-point calculation for NPCs/players, and generic
					per-table interaction logic. Specialized tables (worktable,
					customer table, ingredient box, etc.) inherit and override this.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/SceneManager.hpp" // for Scene interface (GetGameObjectByID, etc.)
#include "GameCore/PlateLogic.hpp"
#include "GameCore/TableLogic.hpp"

namespace {
	/**
	 * @brief Resolves the visual tabletop anchor used when dropping an item onto a table.
	 * @param scene Active scene used to inspect authored texture metadata.
	 * @param owner Owning table object.
	 * @return World-space placement position for the table's held item.
	 */
	Math::Vector3D ResolveTableItemPlacementPos(Scene& scene, GameObject& owner) {
		Math::Vector3D tablePos = owner.GetPosition();
		const std::string& texturePath = scene.GetObjectTexturePath(owner.GetID());

		// Table_Leg has its visible tabletop slightly above the sprite origin in both kitchen layouts.
		if (texturePath.find("Table_Leg.png") != std::string::npos) {
			tablePos.y -= 10.0f;
		}

		return tablePos;
	}
}

/**
 * @brief Constructs table logic with no held item and no authored approach points.
 * @param ownerID Runtime object ID that owns this logic component.
 */
TableLogic::TableLogic(int ownerID) : GameObjectLogic(ownerID), heldItemID_(kInvalidID) {
	// Approach offsets are loaded from scene defaults during Start(), not hard-coded here.
}

/**
 * @brief Initializes the table and loads any authored approach offsets.
 * @param scene Active scene containing the table object.
 */
void TableLogic::Start(Scene& scene) {
	// Ensure clean state if this script is reused.
	heldItemID_ = kInvalidID;

	// Read per-instance approach offsets from the serialized scene defaults.
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

/**
 * @brief Clears held-item bookkeeping before the table object is destroyed.
 * @param scene Active scene containing the table object.
 */
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

/**
 * @brief Returns the owning table object if it still exists in the scene.
 * @param scene Active scene containing the table object.
 * @return Owning game object, or `nullptr` when it cannot be found.
 */
GameObject* TableLogic::GetOwnerChecked(Scene& scene) const {
	GameObject* owner = GetOwner(scene);
	// Centralize owner lookup so callers do not each duplicate the null check.
	return owner;
}

/**
 * @brief Returns whether this table can currently accept the specified item.
 * @param scene Active scene containing the table and candidate item.
 * @param itemID Runtime ID of the item being tested.
 * @return True when the table is effectively empty and the item exists.
 */
bool TableLogic::CanAcceptItem(Scene& scene, int itemID) const {
	// Treat stale held-item references as occupied until RefreshHeldItemState() or PlaceItem() corrects them.
	if (heldItemID_ != kInvalidID && scene.GetGameObjectByID(heldItemID_) != nullptr)
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

/**
 * @brief Clears stale occupancy when the tracked held item no longer exists.
 * @param scene Active scene containing the table and held item.
 */
void TableLogic::RefreshHeldItemState(Scene& scene) {
	// External despawns should not leave the table permanently marked as occupied.
	if (heldItemID_ != kInvalidID && scene.GetGameObjectByID(heldItemID_) == nullptr) {
		heldItemID_ = kInvalidID;
	}
}

/**
 * @brief Places an item onto the table and snaps it to the authored tabletop anchor.
 * @param scene Active scene containing the table and candidate item.
 * @param itemID Runtime ID of the item being placed.
 * @return True when the item was accepted and positioned successfully.
 */
bool TableLogic::PlaceItem(Scene& scene, int itemID) {
	if (heldItemID_ != kInvalidID && scene.GetGameObjectByID(heldItemID_) == nullptr) {
		// Recover from stale occupancy when the previous item was despawned externally.
		heldItemID_ = kInvalidID;
	}

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

	// Snap the item to the visual tabletop, not just the raw object origin.
	Math::Vector3D newPos = GetItemPlacementPosition(scene);
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

/**
 * @brief Removes and returns the current held item from the table.
 * @param scene Active scene containing the table and held item.
 * @return Held item ID, or `kInvalidID` when the table is empty.
 */
int TableLogic::TakeItem(Scene& scene) {
	if (!HasItem()) {
		return kInvalidID;
	}

	if (!scene.GetGameObjectByID(heldItemID_)) {
		// Stale references should not permanently lock the table.
		heldItemID_ = kInvalidID;
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

/**
 * @brief Returns the world-space anchor used to place items on top of the table.
 * @param scene Active scene containing the table object.
 * @return Placement position for a held item.
 */
Math::Vector3D TableLogic::GetItemPlacementPosition(Scene& scene) const {
	GameObject* owner = GetOwnerChecked(scene);
	if (!owner) {
		return Math::Vector3D(0.0f, 0.0f, 0.0f);
	}

	return ResolveTableItemPlacementPos(scene, *owner);
}

/**
 * @brief Appends a new local-space approach offset for the table.
 * @param offset Local-space offset to add.
 */
void TableLogic::AddApproachOffset(const Math::Vector2D& offset) {
	// Preserve insertion order so authored approach priorities stay predictable.
	approachOffsets_.push_back(offset);
}

/**
 * @brief Removes all authored approach offsets from the table.
 */
void TableLogic::ClearApproachOffsets() {
	// Clear the list completely so the next setup pass can rebuild it from scratch.
	approachOffsets_.clear();
}

/**
 * @brief Returns every approach offset converted into world-space positions.
 * @param scene Active scene containing the table object.
 * @return World-space approach points for the table.
 */
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

/**
 * @brief Returns the closest available approach point to a supplied world position.
 * @param scene Active scene containing the table object.
 * @param from World-space position used as the distance reference.
 * @return Nearest approach point, or `from` when none are available.
 */
Math::Vector2D TableLogic::GetClosestApproachPoint(Scene& scene,
	const Math::Vector2D& from) const {
	std::vector<Math::Vector2D> worldPoints = GetApproachPointsWorld(scene);
	if (worldPoints.empty()) {
		return from;
	}

	float bestDistSq = std::numeric_limits<float>::max();
	Math::Vector2D bestPoint = worldPoints[0];
	for (std::size_t i = 0; i < worldPoints.size(); ++i) {
		// Measure squared distance to avoid an unnecessary square root inside the search loop.
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
