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

#include "Graphics/GameObject.hpp"
#include "Graphics/SceneManager.hpp"
#include "Core/Logger.hpp"

#include "TrashCanLogic.hpp"

TrashCanLogic::TrashCanLogic(int ownerID)
	: TableLogic(ownerID) {
}

bool TrashCanLogic::CanAcceptItem(Scene& scene, int itemID) const {
	// Trash can never stores anything, so it is always effectively empty.
	// We still validate the item exists (reuse base rules).
	// (Base rules: table must be empty, itemID valid, item exists)
	// heldItemID_ will stay invalid, so HasItem() is always false here.
	return TableLogic::CanAcceptItem(scene, itemID);
}

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

int TrashCanLogic::TakeItem(Scene& /*scene*/) {
	// Trash can never has an item to take.
	return kInvalidID;
}

void TrashCanLogic::OnItemPlaced(Scene& /*scene*/, GameObject& item) {
	TS_LOG_DEBUG("[TrashCanLogic] Trashed item " << item.GetID()
		<< " (owner trash can=" << GetOwnerID() << ")");
}

