/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu	(45%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(45%)
					Seah Wang Hua, wanghua.seah@digipen.edu (10%)

 DESCRIPTION:		Declares the PlayerLogic script that handles player movement, click-to-move
					navigation, arrival callbacks, item pick-up/drop behavior, and sprite facing
					updates.
					- Exposes the public player gameplay interface used by Scene and LogicManager
					- Declares helper methods for movement, interaction, hover, and carry handling
					- Tracks runtime state for queued actions, pathfinding, and carried items
					- Centralizes player-specific gameplay behavior in a single logic component

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>

#include "EngineCore/GameObjectLogic.hpp"
#include "EngineCore/Math.hpp"

 // Forward declarations to avoid circular dependencies
class LogicManager;
class TableLogic;
class PlayerLogic : public GameObjectLogic {
public:
	// Inherit constructor from GameObjectLogic
	using GameObjectLogic::GameObjectLogic; // inherit constructor

	/**
	 * @brief Initializes cached player state once the owning scene is ready.
	 * @param scene Active scene containing the player object and related systems.
	 */
	void Start(Scene& scene) override;

	/**
	 * @brief Updates movement, interaction, carried items, and player VFX for one frame.
	 * @param dt Frame delta time in seconds.
	 * @param scene Active scene containing the player and interactables.
	 * @param input Centralized input snapshot for the current frame.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Returns the runtime logic name used by the logic system and debugger.
	 * @return Stable script name for this logic component.
	 */
	std::string GetName() const override {
		// Return the stable logic identifier used by the runtime logic registry.
		return "PlayerLogic";
	}

	/**
	 * @brief Returns whether the player is currently carrying an item.
	 * @return True when a carried item ID is assigned.
	 */
	bool IsHolding() const {
		// Carry state is represented by whether a valid carried item ID is assigned.
		return carriedItemID >= 0;
	}

	/**
	 * @brief Returns the object ID of the currently carried item.
	 * @return Carried object ID, or `-1` when empty-handed.
	 */
	int GetCarriedItemID() const {
		// Expose the raw carried object ID for table, UI, and debug queries.
		return carriedItemID;
	}

	/**
	 * @brief Queues or performs interaction with a specific table object.
	 * @param scene Active scene containing the table.
	 * @param tableObjectID Object ID of the target table.
	 */
	void InteractWithTable(Scene& scene, int tableObjectID);

	/**
	 * @brief Sends the player toward a world-space destination using pathfinding when needed.
	 * @param scene Active scene used for path queries and collision checks.
	 * @param dest Target world position on the walkable plane.
	 */
	void MoveTo(Scene& scene, const glm::vec2& dest);

	/**
	 * @brief Starts direct movement toward a world-space destination without table interaction.
	 * @param dest Target world position.
	 */
	void MoveDirect(const glm::vec2& dest);

	/**
	 * @brief Returns whether the player currently has an active movement target.
	 * @return True when a destination is pending.
	 */
	bool HasDestination() const {
		// Movement remains active while a destination is still pending.
		return hasMoveTarget;
	}

	/**
	 * @brief Returns whether the player is currently idle at its destination.
	 * @return True when no movement target remains.
	 */
	bool HasArrived() const {
		// Arrival is the inverse of having a live movement target.
		return !hasMoveTarget;
	}

	/**
	 * @brief Attaches a world item to the player as the carried object.
	 * @param scene Active scene containing the item.
	 * @param itemID Object ID of the item to pick up.
	 */
	void PickUp(Scene& scene, int itemID);

	/**
	 * @brief Drops the currently carried item back into the scene.
	 * @param scene Active scene receiving the dropped item.
	 */
	void Drop(Scene& scene);

	/**
	 * @brief Clears movement and interaction state while gameplay is paused.
	 * @param scene Active scene used to clear indicators and queued actions.
	 */
	void EnterPauseState(Scene& scene);

private:
	/**
	 * @brief Applies the back-facing carry transparency rule to the player sprite.
	 * @param scene Active scene used to read the current player animation.
	 */
	void UpdateCarryBackTransparency(Scene& scene);

	enum class MoveMode {
		None,
		Direct,
		Pathfinding
	};

	MoveMode moveMode_{ MoveMode::None };

	// Movement state
	glm::vec2 moveTarget{ 0.f, 0.f };
	bool hasMoveTarget{ false };
	float moveSpeed{ 400.f }; // pixels/sec

	// Facing / sprite state
	enum class FacingDir {
		Front,
		Back,
		Left,
		Right
	};
	FacingDir facingDir{ FacingDir::Front };
	float playerBaseAlpha_{ 1.0f };

	// Interaction state
	int carriedItemID{ -1 };
	int pendingTableID = -1; // table we intend to interact with after moving

	// Offset where the carried item should appear relative to the player
	// X: positive = right, negative = left.
	// Y: smaller = up(since W subtracts from y), larger = down.
	glm::vec2 carryOffset{ 0.f, -32.f };
	glm::vec2 carryOffsetFront_{ 0.f, 26.f };
	glm::vec2 carryOffsetBack_{ 0.f, 19.f };
	glm::vec2 carryOffsetLeft_{ -24.f, 25.f };
	glm::vec2 carryOffsetRight_{ 24.f, 25.f };

	// Store original layer of the carried item (so we can restore on drop)
	std::string carriedItemOriginalLayer_;
	bool hasCarriedItemOriginalLayer_{ false };

	// Store original collider size of the carried item (so we can restore on drop)
	Math::Vector2D carriedItemOriginalColliderSize{ 0.f, 0.f };
	bool hasCarriedItemOriginalColliderSize{ false };

	/**
	 * @brief Processes mouse click input and queues the appropriate action.
	 * @param scene Active scene being processed.
	 * @param input Input manager for the current frame.
	 * @param dt Frame delta time in seconds.
	 */
	void HandleClickInput(Scene& scene, InputManager& input, float dt); // Unity: input + raycast

	/**
	 * @brief Advances player movement and path-following for one frame.
	 * @param dt Frame delta time in seconds.
	 * @param scene Active scene being processed.
	 */
	void UpdateMovement(float dt, Scene& scene); // Unity: NavMeshAgent movement

	/**
	 * @brief Finalizes movement once the player reaches its active target.
	 * @param scene Active scene being processed.
	 */
	void OnArrived(Scene& scene); // Unity: OnArrived() hook

	/**
	 * @brief Updates the player's animation and facing from a movement vector.
	 * @param scene Active scene being processed.
	 * @param player Owning player object.
	 * @param moveDir Raw movement direction used to determine facing.
	 */
	void UpdateSprite(Scene& scene, GameObject* player, const glm::vec2& moveDir);

	/**
	 * @brief Returns the carry offset that matches the current facing direction.
	 * @return Facing-dependent carry offset in scene units.
	 */
	glm::vec2 GetCarryOffsetForFacing() const;

	/**
	 * @brief Returns the render layer used for a carried item in the current facing.
	 * @param baseLayer Original authored layer of the item before it was carried.
	 * @return Layer name to use while the item is attached to the player.
	 */
	std::string GetCarryLayerForFacing(const std::string& baseLayer) const;

	/**
	 * @brief Returns the render layer used for carried child visuals such as plate contents.
	 * @param baseLayer Original authored layer of the child item before carrying.
	 * @return Layer name to use while the child visual is carried.
	 */
	std::string GetCarryChildLayerForFacing(const std::string& baseLayer) const;

	/**
	 * @brief Returns the numeric layer directly above a supplied base layer.
	 * @param baseLayer Original layer name to offset.
	 * @return Incremented layer name, or the original string when it is non-numeric.
	 */
	std::string GetChildLayerAbove(const std::string& baseLayer) const;

	/**
	 * @brief Applies the correct render layer to the currently carried item.
	 * @param scene Active scene being processed.
	 * @param itemID Object ID of the carried item.
	 */
	void ApplyCarryLayer(Scene& scene, int itemID);

	/**
	 * @brief Restores the original render layer for a released carried item.
	 * @param scene Active scene being processed.
	 * @param itemID Object ID of the released item.
	 */
	void RestoreCarriedItemLayer(Scene& scene, int itemID);

	/**
	 * @brief Keeps the carried item aligned with the player.
	 * @param scene Active scene being processed.
	 */
	void UpdateCarriedItemTransform(Scene& scene);

	/**
	 * @brief Refreshes hover outlines and other interactable visual cues.
	 * @param scene Active scene being processed.
	 * @param input Input manager for the current frame.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateInteractableVisualCues(Scene& scene, InputManager& input, float dt);

	/**
	 * @brief Returns whether a world-space point lies within an object's collider.
	 * @param obj Object being tested.
	 * @param worldPoint World-space point to test.
	 * @return True when the point lies inside the collider bounds.
	 */
	bool IsPointInsideObjectCollider(const GameObject* obj, const glm::vec2& worldPoint) const;

	/**
	 * @brief Spawns or restarts the click-to-move destination indicator.
	 * @param scene Active scene being processed.
	 * @param worldPoint World-space destination to indicate.
	 */
	void ShowClickMoveIndicator(Scene& scene, const glm::vec2& worldPoint);

	/**
	 * @brief Updates the lifetime and appearance of the click destination indicator.
	 * @param scene Active scene being processed.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateClickMoveIndicator(Scene& scene, float dt);

	/**
	 * @brief Removes the active click destination indicator immediately.
	 * @param scene Active scene being processed.
	 */
	void ClearClickMoveIndicator(Scene& scene);

	/**
	 * @brief Clears all transient hover and interactable visual cues.
	 * @param scene Active scene being processed.
	 */
	void ClearInteractableVisualCues(Scene& scene);

	/**
	 * @brief Resets mouse drag state.
	 */
	void ResetMouseDragState();

	/**
	 * @brief Attempts to get mouse world.
	 * @param scene Scene being processed.
	 * @param input Input manager for the current frame.
	 * @param mouseWorld Parameter for mouse world.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool TryGetMouseWorld(Scene& scene, InputManager& input, glm::vec2& mouseWorld) const;

	/**
	 * @brief Applies direct keyboard locomotion input for one frame.
	 * @param dt Frame delta time in seconds.
	 * @param scene Active scene being processed.
	 * @param input Input manager for the current frame.
	 * @param player Owning player object.
	 * @param playerPos Current player position.
	 */
	void HandleKeyboardMovement(float dt, Scene& scene, InputManager& input, GameObject* player, const glm::vec3& playerPos);

	/**
	 * @brief Updates footstep particle and audio feedback from player movement.
	 * @param dt Frame delta time in seconds.
	 * @param scene Active scene being processed.
	 * @param input Input manager for the current frame.
	 * @param player Owning player object.
	 * @param beforePos Player position at the start of the frame.
	 * @param afterPos Player position at the end of the frame.
	 */
	void UpdateFootstepTrailAndAudio(float dt, Scene& scene, InputManager& input, GameObject* player, const glm::vec3& beforePos, const glm::vec3& afterPos);

	/**
	 * @brief Clears the active movement target and related path state.
	 * @param scene Active scene being processed.
	 */
	void ClearMovementTarget(Scene& scene);

	/**
	 * @brief Finalizes the current movement operation and resets navigation state.
	 * @param scene Scene being processed.
	 * @param clearPendingTable True to clear any queued table interaction target.
	 */
	void FinishMovement(Scene& scene, bool clearPendingTable);

	/**
	 * @brief Advances past path waypoints that are already within the arrival radius.
	 * @param currentPos Current player world position.
	 * @param arriveRadiusSq Squared distance threshold used to treat a waypoint as reached.
	 * @return True when all remaining waypoints have been consumed.
	 */
	bool AdvancePathWaypoints(const glm::vec2& currentPos, float arriveRadiusSq);

	/**
	 * @brief Attempts to rebuild a path from the current player position to the final destination.
	 * @param scene Scene used for path queries.
	 * @param player Owning player object.
	 * @param currentPos Current player world position.
	 * @param arriveRadiusSq Squared distance threshold used for waypoint skipping.
	 * @param switchToPathMode True to switch the active move mode back to pathfinding.
	 * @return True when a replacement path was found and movement can continue.
	 */
	bool TryRepathToFinalTarget(Scene& scene, GameObject* player, const glm::vec2& currentPos, float arriveRadiusSq, bool switchToPathMode);

	/**
	 * @brief Checks whether the direct movement goal has been reached and handles arrival.
	 * @param scene Scene being processed.
	 * @param player Owning player object.
	 * @param currentPos Current player world position.
	 * @param arriveRadiusSq Squared distance threshold used for arrival.
	 * @return True when the player has finished the direct move.
	 */
	bool TryFinishDirectMovement(Scene& scene, GameObject* player, const glm::vec2& currentPos, float arriveRadiusSq);

	/**
	 * @brief Updates one frame of direct movement toward the current target.
	 * @param dt Frame delta time in seconds.
	 * @param scene Scene being processed.
	 * @param player Owning player object.
	 * @param playerPos3 Current player position in 3D space.
	 * @param currentPos Current player position in 2D space.
	 * @param arriveRadiusSq Squared distance threshold used for arrival.
	 * @return True when this function handled the movement update for the frame.
	 */
	bool TryUpdateDirectMovement(float dt, Scene& scene, GameObject* player, const glm::vec3& playerPos3, const glm::vec2& currentPos, float arriveRadiusSq);

	/**
	 * @brief Updates one frame of waypoint-based path movement.
	 * @param dt Frame delta time in seconds.
	 * @param scene Scene being processed.
	 * @param player Owning player object.
	 * @param playerPos3 Current player position in 3D space.
	 * @param currentPos Current player position in 2D space.
	 * @param arriveRadiusSq Squared distance threshold used for waypoint arrival.
	 * @return True when this function handled the movement update for the frame.
	 */
	bool TryUpdatePathMovement(float dt, Scene& scene, GameObject* player, const glm::vec3& playerPos3, const glm::vec2& currentPos, float arriveRadiusSq);

	/**
	 * @brief Processes held-mouse drag retargeting for click-and-drag movement.
	 * @param scene Scene being processed.
	 * @param dt Frame delta time in seconds.
	 * @param mouseWorld Mouse position in world space.
	 * @return True when a new drag destination was issued.
	 */
	bool TryProcessHeldDragRetarget(Scene& scene, float dt, const glm::vec2& mouseWorld);

	/**
	 * @brief Queues a follow-up click action while a table interaction is about to commit.
	 * @param scene Scene being processed.
	 * @param player Owning player object.
	 * @param mouseWorld Mouse position in world space.
	 * @param clickedTableID Table object ID under the cursor, if any.
	 * @param clickedTableLogic Table logic for the clicked table, if any.
	 * @param clickedTable True when the click resolved to a table target.
	 * @return True when the click was captured as a queued follow-up action.
	 */
	bool TryQueuePostCommitClick(Scene& scene, GameObject* player, const glm::vec2& mouseWorld, int clickedTableID, TableLogic* clickedTableLogic, bool clickedTable);

	/**
	 * @brief Handles a click that resolved to a table target.
	 * @param scene Scene being processed.
	 * @param player Owning player object.
	 * @param clickedTableID Object ID of the clicked table.
	 * @param clickedTableLogic Logic attached to the clicked table.
	 * @return True when the click was consumed as a table interaction request.
	 */
	bool TryHandleTableClick(Scene& scene, GameObject* player, int clickedTableID, TableLogic* clickedTableLogic);

	/**
	 * @brief Resolves a navigation-safe movement target for a table interaction.
	 * @param scene Scene being processed.
	 * @param tableObjectID Object ID of the target table.
	 * @param fromWorld Current player position used to rank candidate approach points.
	 * @param outTarget Receives the best snapped interaction target.
	 * @return True when a usable target was found.
	 */
	bool TryGetTableMoveTarget(Scene& scene, int tableObjectID, const glm::vec2& fromWorld, glm::vec2& outTarget);

	/**
	 * @brief Handles a plain move click on walkable world space.
	 * @param scene Scene being processed.
	 * @param mouseWorld Mouse position in world space.
	 */
	void HandleMoveClick(Scene& scene, const glm::vec2& mouseWorld);

	/**
	 * @brief Returns whether the player is currently close enough to interact with a table.
	 * @param scene Active scene being processed.
	 * @param tableObjectID Object ID of the target table.
	 * @return True when the player can immediately interact with the table.
	 */
	bool IsInTableInteractionRange(Scene& scene, int tableObjectID);

	/**
	 * @brief Returns whether a hypothetical player position can interact with a table.
	 * @param scene Scene being processed.
	 * @param tableObjectID Object ID of the target table.
	 * @param playerPos World-space player position to test.
	 * @param radius Allowed interaction radius for authored or snapped approach points.
	 * @return True when the tested position can commit the interaction.
	 */
	bool IsTableInRangeAtPosition(Scene& scene, int tableObjectID, const glm::vec2& playerPos, float radius);

	/**
	 * @brief Cancels any queued movement that is waiting to interact with a table.
	 * @param scene Active scene being processed.
	 */
	void CancelQueuedTableMove(Scene& scene);

	/**
	 * @brief Ensures a hover outline exists and matches the current source object transform.
	 * @param scene Active scene being processed.
	 * @param sourceObj Source object being outlined.
	 * @param sourceID Object ID of the source object.
	 */
	void EnsureHoverOutline(Scene& scene, GameObject* sourceObj, int sourceID);

	/**
	 * @brief Removes the hover outline associated with a source object.
	 * @param scene Active scene being processed.
	 * @param sourceID Object ID whose outline should be removed.
	 */
	void RemoveHoverOutline(Scene& scene, int sourceID);

	/**
	 * @brief Removes every hover outline currently tracked by the player.
	 * @param scene Active scene being processed.
	 */
	void ClearHoverOutlines(Scene& scene);

	// Particle footsteps
	float footstepDistanceAcc_ = 0.0f;
	bool wasMoving_ = false;
	float footstepEmitTimer_ = 0.0f;

	// Mouse drag state for click-and-drag movement
	bool mouseDragActive_ = false;
	glm::vec2 lastDragWorld_{ 0.0f, 0.0f };
	bool hasLastDragWorld_ = false;
	float dragRetargetTimer_ = 0.0f;
	std::unordered_set<int> highlightedInteractableIDs_;

	// Hover outline state: source object ID -> 5 overlay sprite IDs
	// [0..3] = cyan offsets (left/right/up/down), [4] = center mask
	std::unordered_map<int, std::array<int, 5>> hoverOutlineIDs_;

	// Click indicator state
	int clickIndicatorID_ = -1;
	float clickIndicatorTimeLeft_ = 0.0f;
	bool suppressMouseUntilRelease_ = false;

	// Trail effect state
	glm::vec3 lastTrailPos_{ 0.0f, 0.0f, 0.0f };
	bool hasLastTrailPos_ = false;
	float trailCarry_ = 0.0f;

	bool movementLocked_ = false;
	int lockedTableID_ = -1;

	/**
	 * @brief Enters a station-lock state tied to a specific workstation.
	 * @param scene Active scene being processed.
	 * @param tableID Object ID of the workstation that owns the lock.
	 */
	void BeginStationLock(Scene& scene, int tableID);

	/**
	 * @brief Clears the current station-lock state.
	 */
	void EndStationLock();

	/**
	 * @brief Refreshes the current station-lock state and releases it when appropriate.
	 * @param scene Active scene being processed.
	 */
	void UpdateStationLock(Scene& scene);

	/**
	 * @brief Returns whether the chopping animation should be enforced this frame.
	 * @param scene Active scene being processed.
	 * @return True when the locked station should keep the player in chop animation.
	 */
	bool ShouldPlayChopAnimation(Scene& scene) const;

	/**
	 * @brief Ensures the player is using the correct chopping animation clip.
	 * @param scene Active scene being processed.
	 * @param player Owning player object.
	 */
	void EnsureChopAnimation(Scene& scene, GameObject* player);

	std::vector<glm::vec2> pathPoints_;
	std::size_t pathIndex_ = 0;
	glm::vec2 finalTarget_{ 0.0f, 0.0f };

	float directPathCheckTimer_ = 0.0f;
	static constexpr float kDirectPathCheckInterval = 0.05f; // 20 times/sec
	int blockedMoveFrames_ = 0;

	enum class QueuedActionType {
		None,
		MoveWorld,
		InteractTable
	};

	struct QueuedAction {
		QueuedActionType type = QueuedActionType::None;
		glm::vec2 worldPos{ 0.0f, 0.0f };
		int tableID = -1;
	};

	static constexpr float kInteractionCommitRadius = 60.0f;

	QueuedAction queuedAction_{};

	/**
	 * @brief Resolves which table, if any, was clicked under the cursor.
	 * @param scene Active scene being processed.
	 * @param mouseWorld Mouse position in world space.
	 * @param outTableID Receives the clicked table object ID.
	 * @param outTableLogic Receives the clicked table logic pointer.
	 * @return True when a table target was found under the cursor.
	 */
	bool TryResolveClickedTableTarget(Scene& scene,
		const glm::vec2& mouseWorld,
		int& outTableID,
		TableLogic*& outTableLogic);

	/**
	 * @brief Returns whether the player is close enough to commit a queued table interaction.
	 * @param scene Active scene being processed.
	 * @param tableObjectID Object ID of the target table.
	 * @return True when the player is within interaction-commit range.
	 */
	bool IsInTableCommitRange(Scene& scene, int tableObjectID);

	/**
	 * @brief Stores a deferred move-to-world action.
	 * @param worldPos World-space destination to remember.
	 */
	void QueueMoveAction(const glm::vec2& worldPos);

	/**
	 * @brief Stores a deferred interact-with-table action.
	 * @param tableObjectID Object ID of the target table.
	 */
	void QueueTableAction(int tableObjectID);

	/**
	 * @brief Clears any deferred action currently stored by the player.
	 */
	void ClearQueuedAction();

	/**
	 * @brief Executes the current deferred action, if one is stored.
	 * @param scene Active scene being processed.
	 */
	void ExecuteQueuedAction(Scene& scene);

	/**
	 * @brief Handles ingredient box interaction, including plate-box special cases.
	 * @param scene Scene being processed.
	 * @param player Owning player object.
	 * @param logicMgr Logic manager used to query related scripts.
	 * @param tableObjectID Object ID of the interacted ingredient box.
	 * @return True when the interaction was handled by an ingredient box flow.
	 */
	bool TryHandleIngredientBoxInteraction(Scene& scene, GameObject* player, LogicManager& logicMgr, int tableObjectID);

	/**
	 * @brief Handles customer-table payment collection.
	 * @param scene Scene being processed.
	 * @param logicMgr Logic manager used to query related scripts.
	 * @param tableObjectID Object ID of the interacted customer table.
	 * @return True when the interaction was handled by a customer table flow.
	 */
	bool TryHandleCustomerTableInteraction(Scene& scene, LogicManager& logicMgr, int tableObjectID);

	/**
	 * @brief Handles trash-can disposal of the currently carried item.
	 * @param scene Scene being processed.
	 * @param logicMgr Logic manager used to query related scripts.
	 * @param tableObjectID Object ID of the interacted trash can.
	 * @return True when the interaction was handled by a trash-can flow.
	 */
	bool TryHandleTrashCanInteraction(Scene& scene, LogicManager& logicMgr, int tableObjectID);

	/**
	 * @brief Attempts to pick up an item currently resting on a table.
	 * @param scene Scene being processed.
	 * @param logicMgr Logic manager used to query related scripts.
	 * @param table Target table logic.
	 * @param tableObjectID Object ID of the interacted table.
	 * @return True when an item was successfully removed from the table and picked up.
	 */
	bool TryPickUpItemFromTable(Scene& scene, LogicManager& logicMgr, TableLogic& table, int tableObjectID);

	/**
	 * @brief Attempts to place the carried item onto an empty table.
	 * @param scene Scene being processed.
	 * @param player Owning player object.
	 * @param logicMgr Logic manager used to query related scripts.
	 * @param table Target table logic.
	 * @param tableObjectID Object ID of the interacted table.
	 * @return True when the carried item was placed successfully.
	 */
	bool TryPlaceHeldItemOnEmptyTable(Scene& scene, GameObject* player, LogicManager& logicMgr, TableLogic& table, int tableObjectID);

	/**
	 * @brief Attempts to combine the carried item with the item currently on a table.
	 * @param scene Scene being processed.
	 * @param player Owning player object.
	 * @param logicMgr Logic manager used to query related scripts.
	 * @param table Target table logic.
	 * @return True when the carried and table items were combined successfully.
	 */
	bool TryCombineHeldAndTableItems(Scene& scene, GameObject* player, LogicManager& logicMgr, TableLogic& table);
};
