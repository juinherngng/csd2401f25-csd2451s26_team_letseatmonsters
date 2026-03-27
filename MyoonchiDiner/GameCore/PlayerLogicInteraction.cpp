/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogicInteraction.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements PlayerLogic click handling and table/item interaction behavior.
					- Resolves mouse clicks into movement or table interaction targets
					- Queues deferred actions while table commits or station locks are active
					- Handles ingredient box, customer table, trash can, and work table flows
					- Combines held items with table items for plating and dish assembly

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/InputManager.hpp"
#include "GameCore/CustomerOrderUILogic.hpp"
#include "GameCore/CustomerTableLogic.hpp"
#include "GameCore/IngredientBoxLogic.hpp"
#include "GameCore/IngredientLogic.hpp"
#include "GameCore/PlateLogic.hpp"
#include "GameCore/PlayerLogicShared.hpp"
#include "GameCore/SimpleNpcLogic.hpp"
#include "GameCore/TableLogic.hpp"
#include "GameCore/TrashCanLogic.hpp"
#include "GameCore/WorkTableLogic.hpp"

 /**
  * @brief Resolves the best table-like target beneath the mouse cursor.
  * @param scene Active scene containing tables, customers, and order bubbles.
   * @param mouseWorld Mouse position in world space.
   * @param outTableID Receives the resolved table object ID.
   * @param outTableLogic Receives the resolved table logic instance.
   * @return True when a valid table interaction target was found.
   */
bool PlayerLogic::TryResolveClickedTableTarget(Scene& scene,
	const glm::vec2& mouseWorld,
	int& outTableID,
	TableLogic*& outTableLogic) {
	// Choose the best logical table target under the cursor, including customer proxies and order bubbles.
	outTableID = -1;
	outTableLogic = nullptr;

	LogicManager& logicMgr = scene.GetLogicManager();
	float bestDistSq = std::numeric_limits<float>::max();

	auto considerTableTarget = [&](int tableID, GameObject* hitObject) {
		// Keep the nearest eligible target so overlapping visuals resolve predictably.
		if (tableID < 0 || !hitObject) {
			return;
		}

		TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(tableID);
		if (!tableLogic) {
			return;
		}

		const float distSq = PlayerLogicDetail::DistanceSqToObjectCenter(mouseWorld, hitObject);
		if (distSq < bestDistSq) {
			bestDistSq = distSq;
			outTableID = tableID;
			outTableLogic = tableLogic;
		}
		};

	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		// First pass: direct table visuals.
		if (!obj) {
			continue;
		}

		const int id = obj->GetID();
		if (!logicMgr.GetLogicForObject<TableLogic>(id)) {
			continue;
		}
		if (!PlayerLogicDetail::PointInsideObjectVisualRect(mouseWorld, obj)) {
			continue;
		}

		considerTableTarget(id, obj);
	}

	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		// Second pass: customer avatars and order bubbles that should redirect to their table.
		if (!obj) {
			continue;
		}

		const int id = obj->GetID();
		SimpleNpcLogic* npcLogic = logicMgr.GetLogicForObject<SimpleNpcLogic>(id);
		if (!npcLogic) {
			continue;
		}

		const int customerTableID = npcLogic->GetCustomerTableID();
		if (customerTableID < 0) {
			continue;
		}

		const bool hitCustomer = PlayerLogicDetail::PointInsideObjectVisualRect(mouseWorld, obj);

		bool hitBubble = false;
		if (CustomerOrderUILogic* uiLogic = logicMgr.GetLogicForObject<CustomerOrderUILogic>(id)) {
			hitBubble = uiLogic->HitTestBubble(scene, mouseWorld);
		}

		if (!hitCustomer && !hitBubble) {
			continue;
		}

		considerTableTarget(customerTableID, obj);
	}

	return outTableID >= 0 && outTableLogic != nullptr;
}

/**
 * @brief Returns whether the player is close enough to commit a table interaction.
 * @param scene Active scene being processed.
 * @param tableObjectID Object ID of the target table.
 * @return True when the player is within the tighter interaction commit radius.
 */
bool PlayerLogic::IsInTableCommitRange(Scene& scene, int tableObjectID) {
	// Use authored approach points when available so post-commit clicks feel consistent around stations.
	GameObject* player = GetOwner(scene);
	if (!player) {
		return false;
	}

	LogicManager& logicMgr = scene.GetLogicManager();
	TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(tableObjectID);
	if (!tableLogic) {
		return false;
	}

	if (tableLogic->GetLocalApproachOffsets().empty()) {
		return IsInTableInteractionRange(scene, tableObjectID);
	}

	const glm::vec2 playerPos = PlayerLogicDetail::ToVec2(player->GetPositionGLM());
	Math::Vector2D from(playerPos.x, playerPos.y);
	Math::Vector2D approach = tableLogic->GetClosestApproachPoint(scene, from);

	const float dx = playerPos.x - approach.x;
	const float dy = playerPos.y - approach.y;
	const float distSq = dx * dx + dy * dy;

	return distSq <= kInteractionCommitRadius * kInteractionCommitRadius;
}

/**
 * @brief Stores a deferred world-move action for later execution.
 * @param worldPos Deferred destination in world space.
 */
void PlayerLogic::QueueMoveAction(const glm::vec2& worldPos) {
	// Store a deferred move request to replay once the current interaction window closes.
	queuedAction_.type = QueuedActionType::MoveWorld;
	queuedAction_.worldPos = worldPos;
	queuedAction_.tableID = -1;
}

/**
 * @brief Stores a deferred table interaction action for later execution.
 * @param tableObjectID Object ID of the deferred table target.
 */
void PlayerLogic::QueueTableAction(int tableObjectID) {
	// Store a deferred table interaction for execution after the current commit resolves.
	queuedAction_.type = QueuedActionType::InteractTable;
	queuedAction_.tableID = tableObjectID;
}

/**
 * @brief Clears any deferred action currently stored by PlayerLogic.
 */
void PlayerLogic::ClearQueuedAction() {
	// Reset the deferred action slot back to its empty sentinel state.
	queuedAction_ = QueuedAction{};
}

/**
 * @brief Replays the currently queued deferred action when the player becomes free to act.
 * @param scene Active scene being processed.
 */
void PlayerLogic::ExecuteQueuedAction(Scene& scene) {
	// Replay the most recent deferred click once the player is free to act again.
	if (movementLocked_) {
		return;
	}

	const QueuedAction action = queuedAction_;
	ClearQueuedAction();
	if (action.type == QueuedActionType::None) {
		return;
	}

	if (action.type == QueuedActionType::MoveWorld) {
		// World clicks resume as ordinary click-to-move navigation.
		pendingTableID = -1;
		MoveTo(scene, action.worldPos);
		ShowClickMoveIndicator(scene, action.worldPos);
		return;
	}

	LogicManager& logicMgr = scene.GetLogicManager();
	TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(action.tableID);
	if (!tableLogic) {
		return;
	}

	if (IsInTableInteractionRange(scene, action.tableID)) {
		// If we are already close enough, commit the interaction immediately.
		CancelQueuedTableMove(scene);
		InteractWithTable(scene, action.tableID);
		return;
	}

	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	const glm::vec2 playerPos = PlayerLogicDetail::ToVec2(player->GetPositionGLM());
	Math::Vector2D from(playerPos.x, playerPos.y);
	Math::Vector2D approach = tableLogic->GetClosestApproachPoint(scene, from);
	const glm::vec2 target(approach.x, approach.y);

	pendingTableID = action.tableID;
	MoveTo(scene, target);
	ShowClickMoveIndicator(scene, target);
}

/**
 * @brief Converts the current mouse position into scene world coordinates.
 * @param scene Active scene used for screen-to-world conversion.
 * @param input Centralized input snapshot for the current frame.
 * @param mouseWorld Receives the resolved world-space mouse position.
 * @return True when a valid world-space mouse position could be produced.
 */
bool PlayerLogic::TryGetMouseWorld(Scene& scene, InputManager& input, glm::vec2& mouseWorld) const {
	// Respect replay-provided mouse positions so recorded input replays deterministically.
	if (input.IsReplayOverride()) {
		const glm::dvec2 replayMousePos = input.GetMousePosition();
		return scene.GetGraphicsEngine().GetMouseWorldInScene(mouseWorld, &replayMousePos);
	}

	return scene.GetGraphicsEngine().GetMouseWorldInScene(mouseWorld, nullptr);
}

/**
 * @brief Processes held-button drag retargeting for click-and-drag movement.
 * @param scene Active scene being processed.
 * @param dt Frame delta time in seconds.
 * @param mouseWorld Current mouse position in world space.
 * @return True when a new drag destination was issued.
 */
bool PlayerLogic::TryProcessHeldDragRetarget(Scene& scene, float dt, const glm::vec2& mouseWorld) {
	// Throttle drag retargets so pathfinding is not rebuilt on every tiny cursor movement.
	if (!mouseDragActive_) {
		mouseDragActive_ = true;
		hasLastDragWorld_ = false;
		dragRetargetTimer_ = 0.0f;
	}

	dragRetargetTimer_ -= dt;
	const float minDragRetargetDistSq = PlayerLogicDetail::kDragRetargetDistance * PlayerLogicDetail::kDragRetargetDistance;
	const glm::vec2 delta = mouseWorld - lastDragWorld_;
	const bool movedEnough = !hasLastDragWorld_ ||
		PlayerLogicDetail::DistanceSquared(delta, glm::vec2(0.0f, 0.0f)) >= minDragRetargetDistSq;
	if (!movedEnough || dragRetargetTimer_ > 0.0f) {
		return false;
	}

	// Convert the drag into a fresh move request and replace any queued interaction.
	ClearQueuedAction();
	pendingTableID = -1;
	MoveTo(scene, mouseWorld);
	ShowClickMoveIndicator(scene, mouseWorld);
	lastDragWorld_ = mouseWorld;
	hasLastDragWorld_ = true;
	dragRetargetTimer_ = PlayerLogicDetail::kDragRetargetInterval;
	return true;
}

/**
 * @brief Queues a follow-up click while a pending table interaction is about to commit.
 * @param scene Active scene being processed.
 * @param player Owning player object.
 * @param mouseWorld Mouse position in world space.
 * @param clickedTableID Table object ID under the cursor, if any.
 * @param clickedTableLogic Table logic for the clicked table, if any.
 * @param clickedTable True when the cursor resolved to a table target.
 * @return True when the click was captured as a follow-up action.
 */
bool PlayerLogic::TryQueuePostCommitClick(Scene& scene, GameObject* player, const glm::vec2& mouseWorld, int clickedTableID, TableLogic* clickedTableLogic, bool clickedTable) {
	// Queue follow-up input while the player is already close enough to finish a table commit.
	if (clickedTable) {
		if (clickedTableID == pendingTableID) {
			ClearQueuedAction();
			return true;
		}

		QueueTableAction(clickedTableID);
		// Show feedback at the eventual interaction approach point, not the raw click location.
		const glm::vec2 playerPos = PlayerLogicDetail::ToVec2(player->GetPositionGLM());
		Math::Vector2D from(playerPos.x, playerPos.y);
		Math::Vector2D approach = clickedTableLogic->GetClosestApproachPoint(scene, from);
		ShowClickMoveIndicator(scene, glm::vec2(approach.x, approach.y));
		return true;
	}

	QueueMoveAction(mouseWorld);
	ShowClickMoveIndicator(scene, mouseWorld);
	return true;
}

/**
 * @brief Handles a click that resolved directly to a table target.
 * @param scene Active scene being processed.
 * @param player Owning player object.
 * @param clickedTableID Object ID of the clicked table.
 * @param clickedTableLogic Logic attached to the clicked table.
 * @return True when the click was consumed as a table interaction request.
 */
bool PlayerLogic::TryHandleTableClick(Scene& scene, GameObject* player, int clickedTableID, TableLogic* clickedTableLogic) {
	// Either interact immediately or move to the closest authored approach point for the table.
	ClearQueuedAction();

	if (IsInTableInteractionRange(scene, clickedTableID)) {
		CancelQueuedTableMove(scene);
		InteractWithTable(scene, clickedTableID);
		return true;
	}

	pendingTableID = clickedTableID;
	const glm::vec2 playerPos = PlayerLogicDetail::ToVec2(player->GetPositionGLM());
	Math::Vector2D from(playerPos.x, playerPos.y);
	Math::Vector2D approach = clickedTableLogic->GetClosestApproachPoint(scene, from);
	const glm::vec2 target(approach.x, approach.y);
	MoveTo(scene, target);
	ShowClickMoveIndicator(scene, target);
	return true;
}

/**
 * @brief Handles a click that should simply move the player to a world-space point.
 * @param scene Active scene being processed.
 * @param mouseWorld Clicked world-space destination.
 */
void PlayerLogic::HandleMoveClick(Scene& scene, const glm::vec2& mouseWorld) {
	// Plain clicks clear pending table intent and become ordinary path requests.
	ClearQueuedAction();
	pendingTableID = -1;
	MoveTo(scene, mouseWorld);
	ShowClickMoveIndicator(scene, mouseWorld);
}

/**
 * @brief Returns whether the player is close enough to interact with a table.
 * @param scene Active scene being processed.
 * @param tableObjectID Object ID of the target table.
 * @return True when the player can interact immediately.
 */
bool PlayerLogic::IsInTableInteractionRange(Scene& scene, int tableObjectID) {
	// Support both authored approach points and a body-overlap fallback for loose table geometry.
	GameObject* player = GetOwner(scene);
	GameObject* tableObj = scene.GetGameObjectByID(tableObjectID);
	if (!player || !tableObj) {
		return false;
	}

	LogicManager& logicMgr = scene.GetLogicManager();
	TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(tableObjectID);

	const glm::vec3 playerPos3 = player->GetPositionGLM();
	const glm::vec2 playerPos(playerPos3.x, playerPos3.y);

	if (tableLogic) {
		// Prefer the closest approach point when the table provides explicit interaction anchors.
		Math::Vector2D from(playerPos.x, playerPos.y);
		Math::Vector2D approach = tableLogic->GetClosestApproachPoint(scene, from);

		const float dx = playerPos.x - approach.x;
		const float dy = playerPos.y - approach.y;
		const float distSq = dx * dx + dy * dy;
		if (distSq <= PlayerLogicDetail::kPlayerInteractRadius * PlayerLogicDetail::kPlayerInteractRadius) {
			return true;
		}
	}

	glm::vec2 playerCenter, playerHalf;
	glm::vec2 tableCenter, tableHalf;
	if (PlayerLogicDetail::GetObjectRect(player, playerCenter, playerHalf) &&
		PlayerLogicDetail::GetObjectRect(tableObj, tableCenter, tableHalf)) {
		// Fallback: allow interaction when the player collider is effectively touching the table body.
		constexpr float kTouchPadding = 14.0f;
		glm::vec2 expandedHalf(
			tableHalf.x + playerHalf.x + kTouchPadding,
			tableHalf.y + playerHalf.y + kTouchPadding);

		const float distSqToTableBody =
			PlayerLogicDetail::DistanceSqPointToExpandedRect(playerCenter, tableCenter, expandedHalf);
		if (distSqToTableBody <= 0.0001f) {
			return true;
		}
	}

	return false;
}

/**
 * @brief Converts the current mouse button state into movement or interaction actions.
 * @param scene Active scene being processed.
 * @param input Centralized input snapshot for the current frame.
 * @param dt Frame delta time in seconds.
 */
void PlayerLogic::HandleClickInput(Scene& scene, InputManager& input, float dt) {
	// Convert the current mouse state into either interaction requests or movement targets.
	if (suppressMouseUntilRelease_) {
		const bool lmbHeld = input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
		const bool lmbJustPressed = input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);

		if (!lmbHeld && !lmbJustPressed) {
			suppressMouseUntilRelease_ = false;
		}

		ResetMouseDragState();
		return;
	}

	const bool lmbJustPressed = input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);
	const bool lmbHeld = input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);

	if (!lmbHeld) {
		// End drag mode as soon as the button is released.
		ResetMouseDragState();
	}

	if (!lmbJustPressed && !lmbHeld) {
		return;
	}

	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	glm::vec2 mouseWorld{};
	if (!TryGetMouseWorld(scene, input, mouseWorld)) {
		return;
	}

	if (movementLocked_) {
		// Ignore gameplay clicks while a workstation owns the player.
		ResetMouseDragState();
		return;
	}

	if (lmbHeld && !lmbJustPressed && pendingTableID >= 0 && IsInTableCommitRange(scene, pendingTableID)) {
		return;
	}

	if (lmbHeld && !lmbJustPressed) {
		// Held input becomes drag-retarget logic after the initial click has been consumed.
		(void)TryProcessHeldDragRetarget(scene, dt, mouseWorld);
		return;
	}

	mouseDragActive_ = true;
	lastDragWorld_ = mouseWorld;
	hasLastDragWorld_ = true;
	dragRetargetTimer_ = 0.0f;

	int clickedTableID = -1;
	TableLogic* clickedTableLogic = nullptr;
	const bool clickedTable = TryResolveClickedTableTarget(scene, mouseWorld, clickedTableID, clickedTableLogic);

	if (pendingTableID >= 0 && IsInTableCommitRange(scene, pendingTableID)) {
		// Preserve the imminent table commit and queue the new click for after it resolves.
		(void)TryQueuePostCommitClick(scene, player, mouseWorld, clickedTableID, clickedTableLogic, clickedTable);
		return;
	}

	if (clickedTable && clickedTableLogic) {
		// Table clicks always take priority over walkable ground clicks.
		(void)TryHandleTableClick(scene, player, clickedTableID, clickedTableLogic);
		return;
	}

	HandleMoveClick(scene, mouseWorld);
}

/**
 * @brief Routes a committed table interaction to the correct gameplay flow.
 * @param scene Active scene being processed.
 * @param tableObjectID Object ID of the interacted table.
 */
void PlayerLogic::InteractWithTable(Scene& scene, int tableObjectID) {
	// Dispatch the interaction through the table-type-specific handlers in priority order.
	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	scene.PlayInteractAudio(tableObjectID);

	LogicManager& logicMgr = scene.GetLogicManager();
	TableLogic* table = logicMgr.GetLogicForObject<TableLogic>(tableObjectID);
	if (!table) {
		return;
	}

	if (TryHandleIngredientBoxInteraction(scene, player, logicMgr, tableObjectID)) {
		// Ingredient boxes have custom spawn/pickup rules, including the plate-box conversion flow.
		return;
	}

	if (TryHandleCustomerTableInteraction(scene, logicMgr, tableObjectID)) {
		return;
	}

	if (TryHandleTrashCanInteraction(scene, logicMgr, tableObjectID)) {
		return;
	}

	const bool playerHolding = (carriedItemID >= 0);
	const bool tableHasItem = table->HasItem();
	if (!playerHolding && tableHasItem) {
		(void)TryPickUpItemFromTable(scene, logicMgr, *table, tableObjectID);
		return;
	}

	if (playerHolding && !tableHasItem) {
		(void)TryPlaceHeldItemOnEmptyTable(scene, player, logicMgr, *table, tableObjectID);
		return;
	}

	if (playerHolding && tableHasItem) {
		// When both sides have items, attempt a recipe or plating combination instead of swapping blindly.
		(void)TryCombineHeldAndTableItems(scene, player, logicMgr, *table);
	}
}

/**
 * @brief Handles interaction with an ingredient box, including plate-box special cases.
 * @param scene Active scene being processed.
 * @param player Owning player object.
 * @param logicMgr Logic manager used to query related scripts.
 * @param tableObjectID Object ID of the interacted ingredient box.
 * @return True when an ingredient-box flow handled the interaction.
 */
bool PlayerLogic::TryHandleIngredientBoxInteraction(Scene& scene, GameObject* player, LogicManager& logicMgr, int tableObjectID) {
	// Ingredient boxes either spawn a new item directly or convert processed ingredients into plated items.
	(void)player;
	IngredientBoxLogic* box = logicMgr.GetLogicForObject<IngredientBoxLogic>(tableObjectID);
	if (!box) {
		return false;
	}

	const bool isPlateBox = (scene.GetObjectTag(tableObjectID) == "plate_box");
	if (isPlateBox && carriedItemID >= 0) {
		// Plate boxes consume a processed ingredient and replace it with a newly spawned plate item.
		IngredientLogic* heldIngredient = logicMgr.GetLogicForObject<IngredientLogic>(carriedItemID);
		if (!heldIngredient || !heldIngredient->IsProcessed()) {
			return true;
		}

		const int ingredientObjID = carriedItemID;
		GameObject* ingredientObj = scene.GetGameObjectByID(ingredientObjID);
		if (!ingredientObj) {
			return true;
		}

		const int newPlateID = box->SpawnIngredient(scene);
		if (newPlateID < 0) {
			return true;
		}

		PlateLogic* newPlate = logicMgr.GetLogicForObject<PlateLogic>(newPlateID);
		GameObject* plateObj = scene.GetGameObjectByID(newPlateID);
		if (!newPlate || !plateObj) {
			if (scene.GetGameObjectByID(newPlateID)) {
				scene.DespawnByID(newPlateID);
			}
			return true;
		}

		bool consumedNow = false;
		if (!newPlate->TryAddIngredient(*heldIngredient, consumedNow)) {
			// Roll back the spawned plate if the ingredient cannot actually be plated.
			if (scene.GetGameObjectByID(newPlateID)) {
				scene.DespawnByID(newPlateID);
			}
			return true;
		}

		plateObj->SetPosition(ingredientObj->GetPositionGLM());
		ingredientObj->SetPosition(plateObj->GetPositionGLM());
		newPlate->SetFirstIngredientObjectID(ingredientObjID);
		ingredientObj->SetColliderSize(Math::Vector2D(0.f, 0.f));
		ingredientObj->SetMovableByPhysics(false);
		ingredientObj->SetRenderSortOrder(1);

		carriedItemID = -1;
		carriedItemOriginalLayer_.clear();
		hasCarriedItemOriginalLayer_ = false;
		hasCarriedItemOriginalColliderSize = false;
		PickUp(scene, newPlateID);

#ifndef _DEBUG
		if (AudioManager* audioMgr = scene.GetAudioManager()) {
			glm::vec3 playerPos = player->GetPositionGLM();
			audioMgr->PlaySound3D("sfx_put_down", playerPos.x, playerPos.y, playerPos.z,
				audioMgr->GetVfxVolume() * 0.8f);
		}
#endif
		return true;
	}

	if (carriedItemID >= 0) {
		// Ignore normal ingredient spawns while the player is already carrying something.
		return true;
	}

	const int newItemID = box->SpawnIngredient(scene);
	if (newItemID >= 0) {
		PickUp(scene, newItemID);
	}

	return true;
}

/**
 * @brief Handles payment collection from a customer table.
 * @param scene Active scene being processed.
 * @param logicMgr Logic manager used to query related scripts.
 * @param tableObjectID Object ID of the interacted customer table.
 * @return True when the interaction was handled by a customer table flow.
 */
bool PlayerLogic::TryHandleCustomerTableInteraction(Scene& scene, LogicManager& logicMgr, int tableObjectID) {
	// Customer tables only expose the payment collection interaction from the player side.
	CustomerTableLogic* ctable = logicMgr.GetLogicForObject<CustomerTableLogic>(tableObjectID);
	if (!ctable) {
		return false;
	}

	return ctable->TryTakePayment(scene);
}

/**
 * @brief Handles disposing of the carried item in a trash can.
 * @param scene Active scene being processed.
 * @param logicMgr Logic manager used to query related scripts.
 * @param tableObjectID Object ID of the interacted trash can.
 * @return True when the interaction was handled by a trash-can flow.
 */
bool PlayerLogic::TryHandleTrashCanInteraction(Scene& scene, LogicManager& logicMgr, int tableObjectID) {
	// Trash cans consume the held item and restore any temporary carry-time state before dropping ownership.
	TrashCanLogic* trash = logicMgr.GetLogicForObject<TrashCanLogic>(tableObjectID);
	if (!trash) {
		return false;
	}

	if (carriedItemID >= 0) {
		const int itemToTrash = carriedItemID;
		if (trash->PlaceItem(scene, itemToTrash)) {
			if (hasCarriedItemOriginalColliderSize) {
				if (GameObject* item = scene.GetGameObjectByID(itemToTrash)) {
					item->SetColliderSize(carriedItemOriginalColliderSize);
				}
				hasCarriedItemOriginalColliderSize = false;
			}

			RestoreCarriedItemLayer(scene, itemToTrash);
			carriedItemID = -1;
		}
	}

	return true;
}

/**
 * @brief Attempts to pick up an item resting on a table.
 * @param scene Active scene being processed.
 * @param logicMgr Logic manager used to query related scripts.
 * @param table Target table logic.
 * @param tableObjectID Object ID of the interacted table.
 * @return True when an item was successfully taken from the table.
 */
bool PlayerLogic::TryPickUpItemFromTable(Scene& scene, LogicManager& logicMgr, TableLogic& table, int tableObjectID) {
	// Work tables may temporarily refuse pickup while processing is in progress.
	if (WorkTableLogic* wt = logicMgr.GetLogicForObject<WorkTableLogic>(tableObjectID)) {
		if (!wt->CanTakeHeldItem(scene)) {
			return false;
		}
	}

	const int itemID = table.TakeItem(scene);
	if (itemID >= 0) {
		PickUp(scene, itemID);
		return true;
	}

	return false;
}

/**
 * @brief Attempts to place the carried item onto an empty table.
 * @param scene Active scene being processed.
 * @param player Owning player object.
 * @param logicMgr Logic manager used to query related scripts.
 * @param table Target table logic.
 * @param tableObjectID Object ID of the interacted table.
 * @return True when the carried item was placed successfully.
 */
bool PlayerLogic::TryPlaceHeldItemOnEmptyTable(Scene& scene, GameObject* player, LogicManager& logicMgr, TableLogic& table, int tableObjectID) {
	// Empty-table placement restores collider/layer state that was suppressed while carrying.
	(void)player;
	if (!table.CanAcceptItem(scene, carriedItemID) || !table.PlaceItem(scene, carriedItemID)) {
		return false;
	}

	if (hasCarriedItemOriginalColliderSize) {
		if (GameObject* item = scene.GetGameObjectByID(carriedItemID)) {
			item->SetColliderSize(carriedItemOriginalColliderSize);
		}
		hasCarriedItemOriginalColliderSize = false;
	}

	RestoreCarriedItemLayer(scene, carriedItemID);
	carriedItemID = -1;

#ifndef _DEBUG
	if (AudioManager* audioMgr = scene.GetAudioManager()) {
		glm::vec3 playerPos = player->GetPositionGLM();
		audioMgr->PlaySound3D("sfx_put_down", playerPos.x, playerPos.y, playerPos.z,
			audioMgr->GetVfxVolume() * 0.8f);
	}
#endif

	if (WorkTableLogic* wt = logicMgr.GetLogicForObject<WorkTableLogic>(tableObjectID)) {
		// Some stations immediately lock the player once processing starts.
		if (wt->LocksPlayerMovementWhileProcessing() && wt->IsProcessing()) {
			BeginStationLock(scene, tableObjectID);
		}
	}

	return true;
}

/**
 * @brief Attempts to combine the carried item with the item currently on a table.
 * @param scene Active scene being processed.
 * @param player Owning player object.
 * @param logicMgr Logic manager used to query related scripts.
 * @param table Target table logic.
 * @return True when the combination succeeded.
 */
bool PlayerLogic::TryCombineHeldAndTableItems(Scene& scene, GameObject* player, LogicManager& logicMgr, TableLogic& table) {
	// Support both directions: ingredient onto plate, or held plate onto table ingredient.
	(void)player;
	const int tableItemID = table.GetHeldItemID();
	PlateLogic* heldPlate = logicMgr.GetLogicForObject<PlateLogic>(carriedItemID);
	IngredientLogic* heldIngredient = logicMgr.GetLogicForObject<IngredientLogic>(carriedItemID);
	PlateLogic* tablePlate = logicMgr.GetLogicForObject<PlateLogic>(tableItemID);
	IngredientLogic* tableIngredient = logicMgr.GetLogicForObject<IngredientLogic>(tableItemID);

	if (heldPlate && tableIngredient) {
		// Merge a table ingredient into the held plate, then upgrade to a dish if the recipe completes.
		if (!tableIngredient->IsProcessed() || !heldPlate->CanAcceptIngredientType(tableIngredient->GetType())) {
			return false;
		}

		const int ingredientObjID = tableItemID;
		const int ingredientCountBefore = heldPlate->GetIngredientCount();
		const int removedID = table.TakeItem(scene);
		if (removedID < 0) {
			return false;
		}

		bool consumedNow = false;
		if (!heldPlate->TryAddIngredient(*tableIngredient, consumedNow)) {
			// Restore the table state when the ingredient cannot actually be added to the plate.
			table.PlaceItem(scene, removedID);
			return false;
		}

		if (ingredientCountBefore == 0) {
			GameObject* plateObj = scene.GetGameObjectByID(carriedItemID);
			GameObject* ingredientObj = scene.GetGameObjectByID(ingredientObjID);
			if (plateObj && ingredientObj) {
				ingredientObj->SetPosition(plateObj->GetPositionGLM());
				heldPlate->SetFirstIngredientObjectID(ingredientObjID);
				ingredientObj->SetColliderSize(Math::Vector2D(0.f, 0.f));
				ingredientObj->SetMovableByPhysics(false);
			}
		}

		DishType dishType;
		std::vector<IngredientType> consumedTypes;
		if (heldPlate->TryAssembleDish(dishType, consumedTypes)) {
			heldPlate->ApplyDishVisual(scene);

			const int firstObjID = heldPlate->GetFirstIngredientObjectID();
			if (firstObjID >= 0) {
				scene.DespawnByID(firstObjID);
				heldPlate->SetFirstIngredientObjectID(-1);
			}
			if (ingredientObjID >= 0 && ingredientObjID != firstObjID) {
				scene.DespawnByID(ingredientObjID);
			}
		}

#ifndef _DEBUG
		if (AudioManager* audioMgr = scene.GetAudioManager()) {
			glm::vec3 playerPos = player->GetPositionGLM();
			audioMgr->PlaySound3D("sfx_put_down", playerPos.x, playerPos.y, playerPos.z,
				audioMgr->GetVfxVolume() * 0.8f);
		}
#endif
		return true;
	}

	if (tablePlate && heldIngredient) {
		// Mirror the same recipe flow when the plate is on the table and the ingredient is in hand.
		const int ingredientObjID = heldIngredient->GetOwnerID();
		const int ingredientCountBefore = tablePlate->GetIngredientCount();

		bool consumedNow = false;
		if (!tablePlate->TryAddIngredient(*heldIngredient, consumedNow)) {
			return false;
		}

		if (ingredientCountBefore == 0) {
			GameObject* plateObj = scene.GetGameObjectByID(tableItemID);
			GameObject* ingredientObj = scene.GetGameObjectByID(ingredientObjID);
			if (plateObj && ingredientObj) {
				ingredientObj->SetPosition(plateObj->GetPositionGLM());
				tablePlate->SetFirstIngredientObjectID(ingredientObjID);
				ingredientObj->SetColliderSize(Math::Vector2D(0.f, 0.f));
				ingredientObj->SetMovableByPhysics(false);
			}
		}

		RestoreCarriedItemLayer(scene, carriedItemID);

		DishType dishType;
		std::vector<IngredientType> consumedTypes;
		if (tablePlate->TryAssembleDish(dishType, consumedTypes)) {
			tablePlate->ApplyDishVisual(scene);

			int firstObjID = tablePlate->GetFirstIngredientObjectID();
			if (firstObjID >= 0) {
				scene.DespawnByID(firstObjID);
				tablePlate->SetFirstIngredientObjectID(-1);
			}
			if (ingredientObjID >= 0 && ingredientObjID != firstObjID) {
				scene.DespawnByID(ingredientObjID);
			}

			hasCarriedItemOriginalColliderSize = false;
		}

		carriedItemID = -1;

#ifndef _DEBUG
		if (AudioManager* audioMgr = scene.GetAudioManager()) {
			glm::vec3 playerPos = player->GetPositionGLM();
			audioMgr->PlaySound3D("sfx_put_down", playerPos.x, playerPos.y, playerPos.z,
				audioMgr->GetVfxVolume() * 0.8f);
		}
#endif
		return true;
	}

	return false;
}
