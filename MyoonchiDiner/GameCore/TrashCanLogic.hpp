/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         TrashCanLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Trash can behaviour:
					- Acts like a table (has approach points via TableLogic)
					- When an item is placed, it is destroyed (despawned)
					- Trash can never stores an item (always "empty")

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "GameCore/TableLogic.hpp"

class TrashCanLogic : public TableLogic {
public:
	/**
	 * @brief Constructs trash-can logic for the owning scene object.
	 * @param ownerID Runtime object ID that owns this logic component.
	 */
	explicit TrashCanLogic(int ownerID);

	/**
	 * @brief Returns whether the trash can can accept the specified item.
	 * @param scene Active scene containing the trash can and candidate item.
	 * @param itemID Runtime ID of the item being tested.
	 * @return True when the item passes the base table validation checks.
	 */
	bool CanAcceptItem(Scene& scene, int itemID) const override;

	/**
	 * @brief Accepts an item and immediately queues it for destruction.
	 * @param scene Active scene containing the trash can and candidate item.
	 * @param itemID Runtime ID of the item being discarded.
	 * @return True when the item was accepted and queued for despawn.
	 */
	bool PlaceItem(Scene& scene, int itemID) override;

	/**
	 * @brief Returns no item because trash cans never retain placed objects.
	 * @param scene Active scene containing the trash can.
	 * @return Always returns `kInvalidID`.
	 */
	int TakeItem(Scene& scene) override;

protected:
	/**
	 * @brief Handles the side effects of an item being placed into the trash.
	 * @param scene Active scene containing the trash can.
	 * @param item Item being discarded.
	 */
	void OnItemPlaced(Scene& scene, GameObject& item) override;

	/**
	 * @brief Returns the runtime logic name used for debugging and registration.
	 * @return Stable logic name string.
	 */
	std::string GetName() const override {
		// Keep the logic name stable so logs and logic inspection remain readable.
		return "TrashCanLogic";
	}
};
