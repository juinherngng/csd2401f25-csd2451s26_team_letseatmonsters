/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         TableLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (60%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (40%)

 DESCRIPTION:       Implements the base class TableLogic, providing shared behavior
					for all table-type objects. This includes item placement and
					pickup, approach-point calculation for NPCs/players, and generic
					per-table interaction logic. Specialized tables (worktable,
					customer table, ingredient box, etc.) inherit and override this.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <limits>
#include <vector>

#include "EngineCore/GameObjectLogic.hpp"
#include "EngineCore/Math.hpp"

/**
 * @brief Shared base logic for all table-like gameplay objects.
 * @details
 * Tracks a single held item, exposes authored approach points for pathing and
 * interaction, and provides overridable hooks that specialized table types can
 * use to enforce item rules or react to placement changes.
 */
class TableLogic : public GameObjectLogic {
public:
	/**
	 * @brief Constructs table logic for the owning scene object.
	 * @param ownerID Runtime object ID that owns this logic component.
	 */
	explicit TableLogic(int ownerID);

	/**
	 * @brief Initializes table state and loads authored approach offsets.
	 * @param scene Active scene containing the table object.
	 */
	void Start(Scene& scene) override;

	/**
	 * @brief Releases held-item bookkeeping before the table is destroyed.
	 * @param scene Active scene containing the table object.
	 */
	void OnDestroy(Scene& scene) override;

	/**
	 * @brief Returns whether the table is currently holding an item.
	 * @return True when a valid held-item ID is recorded.
	 */
	bool HasItem() const {
		// Tables are considered occupied whenever they track a live item ID.
		return heldItemID_ != kInvalidID;
	}

	/**
	 * @brief Returns the runtime ID of the item currently on this table.
	 * @return Held item ID, or `kInvalidID` when the table is empty.
	 */
	int GetHeldItemID() const {
		// Expose the recorded occupancy so interaction code can query the table quickly.
		return heldItemID_;
	}

	/**
	 * @brief Returns whether the table can accept a specific item right now.
	 * @param scene Active scene containing the table and candidate item.
	 * @param itemID Runtime ID of the item being tested.
	 * @return True when the base table rules allow the item to be placed.
	 */
	virtual bool CanAcceptItem(Scene& scene, int itemID) const;

	/**
	 * @brief Refreshes cached occupancy state before other systems query the table.
	 * @param scene Active scene containing the table and held item.
	 */
	virtual void RefreshHeldItemState(Scene& scene);

	/**
	 * @brief Places an item onto the table and snaps it to the tabletop anchor.
	 * @param scene Active scene containing the table and item.
	 * @param itemID Runtime ID of the item to place.
	 * @return True when the item was accepted and recorded successfully.
	 */
	virtual bool PlaceItem(Scene& scene, int itemID);

	/**
	 * @brief Removes and returns the current held item without repositioning it.
	 * @param scene Active scene containing the table and held item.
	 * @return Held item ID, or `kInvalidID` when the table is empty.
	 */
	virtual int TakeItem(Scene& scene);

	/**
	 * @brief Returns the authored local-space approach offsets for this table.
	 * @return Immutable list of approach offsets relative to the table origin.
	 */
	const std::vector<Math::Vector2D>& GetLocalApproachOffsets() const {
		// Return the raw authored offsets so callers can inspect or reuse them directly.
		return approachOffsets_;
	}

	/**
	 * @brief Adds a new approach offset in the table's local space.
	 * @param offset Local-space offset to append.
	 */
	void AddApproachOffset(const Math::Vector2D& offset);

	/**
	 * @brief Replaces all existing approach offsets with a single authored offset.
	 * @param offset Local-space offset that should become the only approach point.
	 */
	void SetSingleApproachOffset(const Math::Vector2D& offset) {
		// Reset the list first so the caller gets exactly one approach point.
		ClearApproachOffsets();
		AddApproachOffset(offset);
	}


	/**
	 * @brief Removes all authored approach offsets from the table.
	 */
	void ClearApproachOffsets();

	/**
	 * @brief Returns all approach points converted into world space.
	 * @param scene Active scene containing the table object.
	 * @return World-space approach points derived from the table transform.
	 */
	std::vector<Math::Vector2D> GetApproachPointsWorld(Scene& scene) const;

	/**
	 * @brief Returns the nearest world-space approach point to a given position.
	 * @param scene Active scene containing the table object.
	 * @param from World-space position used as the distance reference.
	 * @return Closest approach point, or `from` when none are available.
	 */
	Math::Vector2D GetClosestApproachPoint(Scene& scene,
		const Math::Vector2D& from) const;

protected:
	static constexpr int kInvalidID = -1;

	int heldItemID_;
	std::vector<Math::Vector2D> approachOffsets_;

	/**
	 * @brief Optional hook invoked after an item is placed onto the table.
	 * @param scene Active scene containing the table.
	 * @param item Item that was just placed.
	 */
	virtual void OnItemPlaced(Scene& /*scene*/, GameObject& /*item*/) {}

	/**
	 * @brief Optional hook invoked after an item is removed from the table.
	 * @param scene Active scene containing the table.
	 * @param item Item that was just taken.
	 */
	virtual void OnItemTaken(Scene& /*scene*/, GameObject& /*item*/) {}

	/**
	 * @brief Returns the owning table object from the current scene.
	 * @param scene Active scene containing the table object.
	 * @return Owning game object, or `nullptr` when it no longer exists.
	 */
	GameObject* GetOwnerChecked(Scene& scene) const;

	/**
	 * @brief Returns the world-space tabletop anchor used for held-item placement.
	 * @param scene Active scene containing the table object.
	 * @return Item placement position for the table.
	 */
	Math::Vector3D GetItemPlacementPosition(Scene& scene) const;

	std::string GetName() const override {
		// Keep the runtime registration name stable for derived table behaviors.
		return "TableLogic";
	}
};
