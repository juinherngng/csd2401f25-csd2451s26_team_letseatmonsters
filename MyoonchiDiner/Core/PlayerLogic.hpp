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

	// Lifecycle overrides
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

	// Carry state helpers
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
	// (For example: you can call this when the player presses a key near a table.)
	/**
	 * @brief Queues or performs interaction with a specific table object.
	 * @param scene Active scene containing the table.
	 * @param tableObjectID Object ID of the target table.
	 */
	void InteractWithTable(Scene& scene, int tableObjectID);

	// Unity: Move(Vector3 dest)
	/**
	 * @brief Sends the player toward a world-space destination using pathfinding when needed.
	 * @param scene Active scene used for path queries and collision checks.
	 * @param dest Target world position on the walkable plane.
	 */
	void MoveTo(Scene& scene, const glm::vec2& dest);

	// Direct free movement (use this for plain floor clicks)
	/**
	 * @brief Starts direct movement toward a world-space destination without table interaction.
	 * @param dest Target world position.
	 */
	void MoveDirect(const glm::vec2& dest);

	// Unity: bool ReachedDestination()
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

	// Unity: PickUp(GameObject item)
	/**
	 * @brief Attaches a world item to the player as the carried object.
	 * @param scene Active scene containing the item.
	 * @param itemID Object ID of the item to pick up.
	 */
	void PickUp(Scene& scene, int itemID);

	// Unity: Drop()
	/**
	 * @brief Drops the currently carried item back into the scene.
	 * @param scene Active scene receiving the dropped item.
	 */
	void Drop(Scene& scene);

	// Force-clear movement/click state while pause overlay is active or before resume.
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

	// Internal helpers
	void HandleClickInput(Scene& scene, InputManager& input, float dt); // Unity: input + raycast
	void UpdateMovement(float dt, Scene& scene);						// Unity: NavMeshAgent movement
	void OnArrived(Scene& scene);										// Unity: OnArrived() hook
	void UpdateSprite(Scene& scene, GameObject* player, const glm::vec2& moveDir);
	glm::vec2 GetCarryOffsetForFacing() const;

	// Layer management for carried item (to render above player)
	std::string GetCarryLayerForFacing(const std::string& baseLayer) const;
	std::string GetCarryChildLayerForFacing(const std::string& baseLayer) const;
	std::string GetChildLayerAbove(const std::string& baseLayer) const;
	void ApplyCarryLayer(Scene& scene, int itemID);
	void RestoreCarriedItemLayer(Scene& scene, int itemID);

	// Interaction helpers
	void UpdateCarriedItemTransform(Scene& scene);
	void UpdateInteractableVisualCues(Scene& scene, InputManager& input, float dt);
	bool IsPointInsideObjectCollider(const GameObject* obj, const glm::vec2& worldPoint) const;
	void ShowClickMoveIndicator(Scene& scene, const glm::vec2& worldPoint);
	void UpdateClickMoveIndicator(Scene& scene, float dt);
	void ClearClickMoveIndicator(Scene& scene);
	void ClearInteractableVisualCues(Scene& scene);
	void ResetMouseDragState();
	bool TryGetMouseWorld(Scene& scene, InputManager& input, glm::vec2& mouseWorld) const;
	void HandleKeyboardMovement(float dt, Scene& scene, InputManager& input, GameObject* player, const glm::vec3& playerPos);
	void UpdateFootstepTrailAndAudio(float dt, Scene& scene, InputManager& input, GameObject* player, const glm::vec3& beforePos, const glm::vec3& afterPos);
	void ClearMovementTarget(Scene& scene);
	bool IsInTableInteractionRange(Scene& scene, int tableObjectID);
	void CancelQueuedTableMove(Scene& scene);

	// Hover outline helpers
	void EnsureHoverOutline(Scene& scene, GameObject* sourceObj, int sourceID);
	void RemoveHoverOutline(Scene& scene, int sourceID);
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

	void BeginStationLock(Scene& scene, int tableID);
	void EndStationLock();
	void UpdateStationLock(Scene& scene);
	bool ShouldPlayChopAnimation(Scene& scene) const;
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

	bool TryResolveClickedTableTarget(Scene& scene,
		const glm::vec2& mouseWorld,
		int& outTableID,
		TableLogic*& outTableLogic);

	bool IsInTableCommitRange(Scene& scene, int tableObjectID);

	void QueueMoveAction(const glm::vec2& worldPos);
	void QueueTableAction(int tableObjectID);
	void ClearQueuedAction();
	void ExecuteQueuedAction(Scene& scene);
};
