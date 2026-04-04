/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         TrashCanLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Implements TrashCanLogic.
					Any item placed on the trash can is immediately despawned.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineCore/Logger.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/TrashCanLogic.hpp"

 /**
  * @brief Constructs trash-can logic for the owning scene object.
  * @param ownerID Runtime object ID that owns this logic component.
  */
TrashCanLogic::TrashCanLogic(int ownerID)
	: TableLogic(ownerID) {}

/**
 * @brief Returns whether the trash can can accept the specified item.
 * @param scene Active scene containing the trash can and candidate item.
 * @param itemID Runtime ID of the item being tested.
 * @return True when the base table validation allows the item.
 */
bool TrashCanLogic::CanAcceptItem(Scene& scene, int itemID) const {
	// Trash can never stores anything, so it is always effectively empty.
	// We still validate the item exists (reuse base rules).
	// (Base rules: table must be empty, itemID valid, item exists)
	// heldItemID_ will stay invalid, so HasItem() is always false here.
	return TableLogic::CanAcceptItem(scene, itemID);
}

/**
 * @brief Discards an item by moving it onto the trash can and despawning it.
 * @param scene Active scene containing the trash can and candidate item.
 * @param itemID Runtime ID of the item being discarded.
 * @return True when the item was accepted and queued for despawn.
 */
bool TrashCanLogic::PlaceItem(Scene& scene, int itemID) {
	// Validate like a normal table would.
	if (!CanAcceptItem(scene, itemID))
		return false;

	GameObject* owner = GetOwnerChecked(scene);
	GameObject* item = scene.GetGameObjectByID(itemID);
	if (!owner || !item)
		return false;

	// Optional: snap item to trash can position for a frame (visual consistency).
	Math::Vector3D canPos = owner->GetPosition();
	item->SetPosition(Math::Vector3D(canPos.x, canPos.y, canPos.z));

	// IMPORTANT:
	// Do NOT set heldItemID_ (trash can should remain empty).
	// Just trigger hook + despawn.
	OnItemPlaced(scene, *item);

	// Despawn the placed item.
	scene.RequestDespawn(itemID);

	return true; // return true so the player drops it successfully
}

/**
 * @brief Returns no item because trash cans never retain discarded objects.
 * @param scene Active scene containing the trash can.
 * @return Always returns `kInvalidID`.
 */
int TrashCanLogic::TakeItem(Scene& /*scene*/) {
	// Trash can never has an item to take.
	return kInvalidID;
}

/**
 * @brief Logs that an item was discarded by this trash can.
 * @param scene Unused active scene containing the trash can.
 * @param item Item being discarded.
 */
void TrashCanLogic::OnItemPlaced(Scene& /*scene*/, GameObject& item) {
	// Emit a concise trace so discard interactions are easy to confirm in logs.
	TS_LOG_DEBUG("[TrashCanLogic] Trashed item " << item.GetID()
		<< " (owner trash can=" << GetOwnerID() << ")");
}
