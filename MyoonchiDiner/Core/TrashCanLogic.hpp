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

#include "TableLogic.hpp"

class TrashCanLogic : public TableLogic {
public:
	// Constructor takes ownerID and passes to base TableLogic constructor.
	explicit TrashCanLogic(int ownerID);

	// Trash can should accept items similar to a table, but it never "holds" them.
	bool CanAcceptItem(Scene& scene, int itemID) const override;

	// Place item -> delete it.
	bool PlaceItem(Scene& scene, int itemID) override;

	// Taking from trash should do nothing.
	int TakeItem(Scene& scene) override;

protected:
	// When an item is placed, destroy it immediately.
	void OnItemPlaced(Scene& scene, GameObject& item) override;

	// Trash can logic name for debugging.
	std::string GetName() const override {
		return "TrashCanLogic";
	}
};

