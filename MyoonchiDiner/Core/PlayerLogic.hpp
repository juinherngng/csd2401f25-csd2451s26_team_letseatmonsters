/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (60%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu   (40%)

 DESCRIPTION:		Declares the PlayerLogic script that handles player movement, click-to-move
					navigation, arrival callbacks, item pick-up/drop behaviour, and sprite facing
					updates. Provides the public interface used by the Scene and LogicManager.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "Core/GameObjectLogic.hpp"

#include "Math.hpp"

#include <array>
#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>

 // Forward declarations to avoid circular dependencies
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
		return "PlayerLogic";
	}

	/**
	 * @brief Returns whether the player is currently carrying an item.
	 * @return True when a carried item ID is assigned.
	 */
	bool IsHolding() const {
		return carriedItemID >= 0;
	}

	/**
	 * @brief Returns the object ID of the currently carried item.
	 * @return Carried object ID, or `-1` when empty-handed.
	 */
	int  GetCarriedItemID() const {
		return carriedItemID;
	}

	// High-level interaction
	// Called when we want the player to interact with a particular table GameObject.
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
		return hasMoveTarget;
	}

	/**
	 * @brief Returns whether the player is currently idle at its destination.
	 * @return True when no movement target remains.
	 */
	bool HasArrived()   const {
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
	enum class MoveMode {
		None,
		Direct,
		Pathfinding
	};

	MoveMode moveMode_{ MoveMode::None };

	// Movement state
	glm::vec2 moveTarget{ 0.f, 0.f };
	bool  hasMoveTarget{ false };
	float moveSpeed{ 370.f };  // pixels/sec

	// Facing / sprite state
	enum class FacingDir {
		Front, Back, Left, Right
	};
	FacingDir facingDir{ FacingDir::Front };

	// Interaction state
	int carriedItemID{ -1 };
	int pendingTableID = -1;   // table we intend to interact with after moving

	// Offset where the carried item should appear relative to the player
	// X: positive = right, negative = left.
	// Y: smaller = up(since W subtracts from y), larger = down.
	glm::vec2 carryOffset{ 0.f, -32.f };
	glm::vec2 carryOffsetFront_{ 0.f, 26.f };
	glm::vec2 carryOffsetBack_{ 0.f, -42.f };
	glm::vec2 carryOffsetLeft_{ -24.f, 25.f };
	glm::vec2 carryOffsetRight_{ 24.f, 25.f };

	// Store original layer of the carried item (so we can restore on drop)
	std::string carriedItemOriginalLayer_;
	bool hasCarriedItemOriginalLayer_{ false };

	// Store original collider size of the carried item (so we can restore on drop)
	Math::Vector2D carriedItemOriginalColliderSize{ 0.f, 0.f };
	bool hasCarriedItemOriginalColliderSize{ false };

	/**
	 * @brief Handles click input.
	 * @param scene Scene being processed.
	 * @param input Input manager for the current frame.
	 * @param dt Frame delta time in seconds.
	 */
	void HandleClickInput(Scene& scene, InputManager& input, float dt); // Unity: input + raycast

	/**
	 * @brief Updates movement.
	 * @param dt Frame delta time in seconds.
	 * @param scene Scene being processed.
	 */
	void UpdateMovement(float dt, Scene& scene);						// Unity: NavMeshAgent movement

	/**
	 * @brief Performs on arrived.
	 * @param scene Scene being processed.
	 */
	void OnArrived(Scene& scene);										// Unity: OnArrived() hook

	/**
	 * @brief Updates sprite.
	 * @param scene Scene being processed.
	 * @param player Parameter for player.
	 * @param moveDir Parameter for move dir.
	 */
	void UpdateSprite(Scene& scene, GameObject* player, const glm::vec2& moveDir);

	/**
	 * @brief Returns carry offset for facing.
	 * @return Requested value.
	 */
	glm::vec2 GetCarryOffsetForFacing() const;

	/**
	 * @brief Returns carry layer for facing.
	 * @param baseLayer Parameter for base layer.
	 * @return Requested value.
	 */
	std::string GetCarryLayerForFacing(const std::string& baseLayer) const;

	/**
	 * @brief Returns carry child layer for facing.
	 * @param baseLayer Parameter for base layer.
	 * @return Requested value.
	 */
	std::string GetCarryChildLayerForFacing(const std::string& baseLayer) const;

	/**
	 * @brief Returns child layer above.
	 * @param baseLayer Parameter for base layer.
	 * @return Requested value.
	 */
	std::string GetChildLayerAbove(const std::string& baseLayer) const;

	/**
	 * @brief Applies carry layer.
	 * @param scene Scene being processed.
	 * @param itemID Parameter for item id.
	 */
	void ApplyCarryLayer(Scene& scene, int itemID);

	/**
	 * @brief Restores carried item layer.
	 * @param scene Scene being processed.
	 * @param itemID Parameter for item id.
	 */
	void RestoreCarriedItemLayer(Scene& scene, int itemID);

	/**
	 * @brief Updates carried item transform.
	 * @param scene Scene being processed.
	 */
	void UpdateCarriedItemTransform(Scene& scene);

	/**
	 * @brief Updates interactable visual cues.
	 * @param scene Scene being processed.
	 * @param input Input manager for the current frame.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateInteractableVisualCues(Scene& scene, InputManager& input, float dt);

	/**
	 * @brief Returns whether point inside object collider.
	 * @param obj Parameter for obj.
	 * @param worldPoint Parameter for world point.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsPointInsideObjectCollider(const GameObject* obj, const glm::vec2& worldPoint) const;

	/**
	 * @brief Performs show click move indicator.
	 * @param scene Scene being processed.
	 * @param worldPoint Parameter for world point.
	 */
	void ShowClickMoveIndicator(Scene& scene, const glm::vec2& worldPoint);

	/**
	 * @brief Updates click move indicator.
	 * @param scene Scene being processed.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateClickMoveIndicator(Scene& scene, float dt);

	/**
	 * @brief Clears click move indicator.
	 * @param scene Scene being processed.
	 */
	void ClearClickMoveIndicator(Scene& scene);

	/**
	 * @brief Clears interactable visual cues.
	 * @param scene Scene being processed.
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
	 * @brief Handles keyboard movement.
	 * @param dt Frame delta time in seconds.
	 * @param scene Scene being processed.
	 * @param input Input manager for the current frame.
	 * @param player Parameter for player.
	 * @param playerPos Parameter for player pos.
	 */
	void HandleKeyboardMovement(float dt, Scene& scene, InputManager& input, GameObject* player, const glm::vec3& playerPos);

	/**
	 * @brief Updates footstep trail and audio.
	 * @param dt Frame delta time in seconds.
	 * @param scene Scene being processed.
	 * @param input Input manager for the current frame.
	 * @param player Parameter for player.
	 * @param beforePos Parameter for before pos.
	 * @param afterPos Parameter for after pos.
	 */
	void UpdateFootstepTrailAndAudio(float dt, Scene& scene, InputManager& input, GameObject* player, const glm::vec3& beforePos, const glm::vec3& afterPos);

	/**
	 * @brief Clears movement target.
	 * @param scene Scene being processed.
	 */
	void ClearMovementTarget(Scene& scene);

	/**
	 * @brief Returns whether in table interaction range.
	 * @param scene Scene being processed.
	 * @param tableObjectID Parameter for table object id.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsInTableInteractionRange(Scene& scene, int tableObjectID);

	/**
	 * @brief Performs cancel queued table move.
	 * @param scene Scene being processed.
	 */
	void CancelQueuedTableMove(Scene& scene);

	/**
	 * @brief Performs ensure hover outline.
	 * @param scene Scene being processed.
	 * @param sourceObj Parameter for source obj.
	 * @param sourceID Parameter for source id.
	 */
	void EnsureHoverOutline(Scene& scene, GameObject* sourceObj, int sourceID);

	/**
	 * @brief Removes hover outline.
	 * @param scene Scene being processed.
	 * @param sourceID Parameter for source id.
	 */
	void RemoveHoverOutline(Scene& scene, int sourceID);

	/**
	 * @brief Clears hover outlines.
	 * @param scene Scene being processed.
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

	// PlayerLogic.hpp
	bool movementLocked_ = false;
	int  lockedTableID_ = -1;

	/**
	 * @brief Begins station lock.
	 * @param scene Scene being processed.
	 * @param tableID Parameter for table id.
	 */
	void BeginStationLock(Scene& scene, int tableID);

	/**
	 * @brief Ends station lock.
	 */
	void EndStationLock();

	/**
	 * @brief Updates station lock.
	 * @param scene Scene being processed.
	 */
	void UpdateStationLock(Scene& scene);

	/**
	 * @brief Returns whether play chop animation.
	 * @param scene Scene being processed.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool ShouldPlayChopAnimation(Scene& scene) const;

	/**
	 * @brief Performs ensure chop animation.
	 * @param scene Scene being processed.
	 * @param player Parameter for player.
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

	static constexpr float kInteractionCommitRadius = 30.0f;

	QueuedAction queuedAction_{};

	/**
	 * @brief Attempts to resolve clicked table target.
	 * @param scene Scene being processed.
	 * @param mouseWorld Parameter for mouse world.
	 * @param outTableID Output value for out table id.
	 * @param outTableLogic Output value for out table logic.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool TryResolveClickedTableTarget(Scene& scene,
		const glm::vec2& mouseWorld,
		int& outTableID,
		TableLogic*& outTableLogic);

	/**
	 * @brief Returns whether in table commit range.
	 * @param scene Scene being processed.
	 * @param tableObjectID Parameter for table object id.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsInTableCommitRange(Scene& scene, int tableObjectID);

	/**
	 * @brief Performs queue move action.
	 * @param worldPos Parameter for world pos.
	 */
	void QueueMoveAction(const glm::vec2& worldPos);

	/**
	 * @brief Performs queue table action.
	 * @param tableObjectID Parameter for table object id.
	 */
	void QueueTableAction(int tableObjectID);

	/**
	 * @brief Clears queued action.
	 */
	void ClearQueuedAction();

	/**
	 * @brief Performs execute queued action.
	 * @param scene Scene being processed.
	 */
	void ExecuteQueuedAction(Scene& scene);
};
