/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (50%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu	  (30%)
					Seah Wang Hua, wanghua.seah@digipen.edu (20%)

 DESCRIPTION:		Implements player control logic, including movement, sprite updates,
					mouse click handling, item pickup/drop, and scene clamping behavior as well
					hover logic (outlining interactables on mouse hover).

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "Core/AudioManager.hpp"
#include "Core/CustomerTableLogic.hpp"
#include "Core/DebugUI.hpp"
#include "Core/EngineRng.hpp"
#include "Core/IngredientBoxLogic.hpp"
#include "Core/IngredientLogic.hpp"
#include "Core/InputControls.hpp"
#include "Core/InputManager.hpp"
#include "Core/Logger.hpp"
#include "Core/PlateLogic.hpp"
#include "Core/Quota.hpp"
#include "Core/TableLogic.hpp"
#include "Core/TrashCanLogic.hpp"
#include "Core/WorkTableLogic.hpp"
#include "Graphics/ResourceManager.hpp"
#include "Graphics/SceneManager.hpp"

#include "CustomerOrderUILogic.hpp"
#include "PlayerLogic.hpp"
#include "SimpleNpcLogic.hpp"

#include <algorithm>
#include <limits>

namespace {

	/**
	 * @brief Returns whether pointinsideobjectvisualrect.
	 * @param point Parameter for point.
	 * @param obj Parameter for obj.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool PointInsideObjectVisualRect(const glm::vec2& point, GameObject* obj) {
		if (!obj) return false;

		const Math::Vector2D colSize = obj->GetColliderSize();
		const Math::Vector2D colOffset = obj->GetColliderOffset();
		const glm::vec3 scale = obj->GetScaleGLM();

		const float width = (colSize.x > 0.0f) ? colSize.x : scale.x;
		const float height = (colSize.y > 0.0f) ? colSize.y : scale.y;

		if (width <= 0.0f || height <= 0.0f) {
			return false;
		}

		const glm::vec3 pos = obj->GetPositionGLM();
		const glm::vec2 center(pos.x + colOffset.x, pos.y + colOffset.y);

		const float halfW = width * 0.5f;
		const float halfH = height * 0.5f;

		return
			point.x >= center.x - halfW && point.x <= center.x + halfW &&
			point.y >= center.y - halfH && point.y <= center.y + halfH;
	}

	/**
	 * @brief Performs distance sq to object center.
	 * @param point Parameter for point.
	 * @param obj Parameter for obj.
	 * @return Result produced by this operation.
	 */
	float DistanceSqToObjectCenter(const glm::vec2& point, GameObject* obj) {
		if (!obj) return std::numeric_limits<float>::max();

		const Math::Vector2D colOffset = obj->GetColliderOffset();
		const glm::vec3 pos = obj->GetPositionGLM();
		const glm::vec2 center(pos.x + colOffset.x, pos.y + colOffset.y);

		const glm::vec2 d = point - center;
		return d.x * d.x + d.y * d.y;
	}

	/**
	 * @brief Returns object rect.
	 * @param obj Parameter for obj.
	 * @param outCenter Output value for out center.
	 * @param outHalfExtents Output value for out half extents.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool GetObjectRect(GameObject* obj, glm::vec2& outCenter, glm::vec2& outHalfExtents) {
		if (!obj) return false;

		const Math::Vector2D colSize = obj->GetColliderSize();
		const Math::Vector2D colOffset = obj->GetColliderOffset();
		const glm::vec3 scale = obj->GetScaleGLM();

		const float width = (colSize.x > 0.0f) ? colSize.x : scale.x;
		const float height = (colSize.y > 0.0f) ? colSize.y : scale.y;

		if (width <= 0.0f || height <= 0.0f) {
			return false;
		}

		const glm::vec3 pos = obj->GetPositionGLM();
		outCenter = glm::vec2(pos.x + colOffset.x, pos.y + colOffset.y);
		outHalfExtents = glm::vec2(width * 0.5f, height * 0.5f);
		return true;
	}

	/**
	 * @brief Performs distance sq point to expanded rect.
	 * @param point Parameter for point.
	 * @param rectCenter Parameter for rect center.
	 * @param rectHalfExtents Parameter for rect half extents.
	 * @return Result produced by this operation.
	 */
	float DistanceSqPointToExpandedRect(
		const glm::vec2& point,
		const glm::vec2& rectCenter,
		const glm::vec2& rectHalfExtents) {
		const float dx = std::max(std::abs(point.x - rectCenter.x) - rectHalfExtents.x, 0.0f);
		const float dy = std::max(std::abs(point.y - rectCenter.y) - rectHalfExtents.y, 0.0f);
		return dx * dx + dy * dy;
	}
}

// Constants and helper functions for PlayerLogic, in an anonymous namespace to limit scope to this file.
namespace {
	// Interaction and movement parameters
	constexpr float kPlayerInteractRadius = 67.0f;
	constexpr float kMoveRetargetDeadzone = 6.0f;
	constexpr float kDragRetargetDistance = 20.0f;
	constexpr float kDragRetargetInterval = 0.06f;
	constexpr float kArriveRadius = 4.0f;
	constexpr int kBlockedFramesBeforeCancel = 6;

	constexpr float kKeyboardMoveSpeed = 200.0f;
	constexpr float kTrailJitterEpsilon = 0.25f;
	constexpr float kFootstepInterval = 0.3f;
	constexpr float kClickIndicatorLifetime = 0.45f;
	constexpr float kClickIndicatorBaseSize = 37.0f;
	// constexpr float kClickIndicatorPopSize = 36.0f;

	// Clamp large frame spikes (for example right after pause/resume)
	// so movement cannot jump/teleport in a single update.
	constexpr float kMaxPlayerUpdateDt = 1.0f / 30.0f;

	/**
	 * @brief Performs distance squared.
	 * @param a Parameter for a.
	 * @param b Parameter for b.
	 * @return Result produced by this operation.
	 */
	float DistanceSquared(const glm::vec2& a, const glm::vec2& b) {
		const glm::vec2 delta = a - b;
		return delta.x * delta.x + delta.y * delta.y;
	}

	/**
	 * @brief Performs to vec2.
	 * @param value Parameter for value.
	 * @return Result produced by this operation.
	 */
	glm::vec2 ToVec2(const glm::vec3& value) {
		return { value.x, value.y };
	}

	/**
	 * @brief Normalizes or zero.
	 * @param v Parameter for v.
	 * @return Result produced by this operation.
	 */
	glm::vec2 NormalizeOrZero(const glm::vec2& v) {
		const float len = std::sqrt(v.x * v.x + v.y * v.y);
		if (len <= 0.0001f) {
			return glm::vec2(0.0f, 0.0f);
		}

		return glm::vec2(v.x / len, v.y / len);
	}

	// Result struct for FindClickedTable, containing the ID and logic pointer of the clicked table (or defaults if none)
	struct ClickedTableResult {
		int tableID = -1;
		TableLogic* tableLogic = nullptr;
	};

	/**
	 * @brief Finds clicked table.
	 * @param scene Scene being processed.
	 * @param mouseWorld Parameter for mouse world.
	 * @return Result produced by this operation.
	 */
	ClickedTableResult FindClickedTable(Scene& scene, const glm::vec2& mouseWorld) {
		ClickedTableResult result{};
		float bestDistSq = std::numeric_limits<float>::max();

		// Iterate through all game objects and check for tables under the mouse cursor
		LogicManager& logicMgr = scene.GetLogicManager();
		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) {
				continue;
			}

			const int id = obj->GetID();
			TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(id);
			if (!tableLogic) {
				continue;
			}

			auto colSize = obj->GetColliderSize();
			auto colOffset = obj->GetColliderOffset();
			glm::vec3 objPos = obj->GetPositionGLM();
			glm::vec2 center(objPos.x + colOffset.x, objPos.y + colOffset.y);

			const float halfW = colSize.x * 0.5f;
			const float halfH = colSize.y * 0.5f;

			const bool inside =
				(mouseWorld.x >= center.x - halfW && mouseWorld.x <= center.x + halfW) &&
				(mouseWorld.y >= center.y - halfH && mouseWorld.y <= center.y + halfH);

			if (!inside) {
				continue;
			}

			const float distSq = DistanceSquared(mouseWorld, center);
			if (distSq < bestDistSq) {
				bestDistSq = distSq;
				result.tableID = id;
				result.tableLogic = tableLogic;
			}
		}

		return result;
	}
}

/**
 * @brief Performs start.
 * @param scene Scene being processed.
 * @return Result produced by this operation.
 */
void PlayerLogic::Start(Scene& scene) {
	(void)scene;
	hasMoveTarget = false;
	carriedItemID = -1;
	pendingTableID = -1;
	facingDir = FacingDir::Front;
	moveMode_ = MoveMode::None;
	mouseDragActive_ = false;
	hasLastDragWorld_ = false;
	dragRetargetTimer_ = 0.0f;
	highlightedInteractableIDs_.clear();
	hoverOutlineIDs_.clear();
	clickIndicatorID_ = -1;
	clickIndicatorTimeLeft_ = 0.0f;
	suppressMouseUntilRelease_ = false;

	pathPoints_.clear();
	pathIndex_ = 0;
	finalTarget_ = glm::vec2(0.0f, 0.0f);
	directPathCheckTimer_ = 0.0f;
}

/**
 * @brief Attempts to resolve clicked table target.
 * @param scene Scene being processed.
 * @param mouseWorld Parameter for mouse world.
 * @param outTableID Output value for out table id.
 * @param outTableLogic Output value for out table logic.
 * @return Result produced by this operation.
 */
bool PlayerLogic::TryResolveClickedTableTarget(Scene& scene,
	const glm::vec2& mouseWorld,
	int& outTableID,
	TableLogic*& outTableLogic) {
	outTableID = -1;
	outTableLogic = nullptr;

	LogicManager& logicMgr = scene.GetLogicManager();
	float bestDistSq = std::numeric_limits<float>::max();

	auto considerTableTarget = [&](int tableID, GameObject* hitObject) {
		if (tableID < 0 || !hitObject) return;

		TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(tableID);
		if (!tableLogic) return;

		const float distSq = DistanceSqToObjectCenter(mouseWorld, hitObject);
		if (distSq < bestDistSq) {
			bestDistSq = distSq;
			outTableID = tableID;
			outTableLogic = tableLogic;
		}
		};

	// Direct table hit
	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		if (!obj) continue;

		const int id = obj->GetID();
		if (!logicMgr.GetLogicForObject<TableLogic>(id)) continue;
		if (!PointInsideObjectVisualRect(mouseWorld, obj)) continue;

		considerTableTarget(id, obj);
	}

	// Customer / customer bubble hit -> redirect to customer table
	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		if (!obj) continue;

		const int id = obj->GetID();
		SimpleNpcLogic* npcLogic = logicMgr.GetLogicForObject<SimpleNpcLogic>(id);
		if (!npcLogic) continue;

		const int customerTableID = npcLogic->GetCustomerTableID();
		if (customerTableID < 0) continue;

		const bool hitCustomer = PointInsideObjectVisualRect(mouseWorld, obj);

		bool hitBubble = false;
		if (CustomerOrderUILogic* uiLogic = logicMgr.GetLogicForObject<CustomerOrderUILogic>(id)) {
			hitBubble = uiLogic->HitTestBubble(scene, mouseWorld);
		}

		if (!hitCustomer && !hitBubble) continue;

		considerTableTarget(customerTableID, obj);
	}

	return outTableID >= 0 && outTableLogic != nullptr;
}

/**
 * @brief Returns whether in table commit range.
 * @param scene Scene being processed.
 * @param tableObjectID Parameter for table object id.
 * @return True when the operation succeeds or the condition is met.
 */
bool PlayerLogic::IsInTableCommitRange(Scene& scene, int tableObjectID) {
	GameObject* player = GetOwner(scene);
	if (!player) {
		return false;
	}

	LogicManager& logicMgr = scene.GetLogicManager();
	TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(tableObjectID);
	if (!tableLogic) {
		return false;
	}

	// If no authored approach point exists, fall back to normal interaction range.
	if (tableLogic->GetLocalApproachOffsets().empty()) {
		return IsInTableInteractionRange(scene, tableObjectID);
	}

	const glm::vec2 playerPos = ToVec2(player->GetPositionGLM());
	Math::Vector2D from(playerPos.x, playerPos.y);
	Math::Vector2D approach = tableLogic->GetClosestApproachPoint(scene, from);

	const float dx = playerPos.x - approach.x;
	const float dy = playerPos.y - approach.y;
	const float distSq = dx * dx + dy * dy;

	return distSq <= kInteractionCommitRadius * kInteractionCommitRadius;
}

/**
 * @brief Performs queue move action.
 * @param worldPos Parameter for world pos.
 * @return Result produced by this operation.
 */
void PlayerLogic::QueueMoveAction(const glm::vec2& worldPos) {
	queuedAction_.type = QueuedActionType::MoveWorld;
	queuedAction_.worldPos = worldPos;
	queuedAction_.tableID = -1;
}

/**
 * @brief Performs queue table action.
 * @param tableObjectID Parameter for table object id.
 * @return Result produced by this operation.
 */
void PlayerLogic::QueueTableAction(int tableObjectID) {
	queuedAction_.type = QueuedActionType::InteractTable;
	queuedAction_.tableID = tableObjectID;
}

/**
 * @brief Clears queued action.
 * @return Result produced by this operation.
 */
void PlayerLogic::ClearQueuedAction() {
	queuedAction_ = QueuedAction{};
}

/**
 * @brief Performs execute queued action.
 * @param scene Scene being processed.
 * @return Result produced by this operation.
 */
void PlayerLogic::ExecuteQueuedAction(Scene& scene) {
	if (movementLocked_) {
		return;
	}

	const QueuedAction action = queuedAction_;
	ClearQueuedAction();

	if (action.type == QueuedActionType::None) {
		return;
	}

	if (action.type == QueuedActionType::MoveWorld) {
		pendingTableID = -1;
		MoveTo(scene, action.worldPos);
		ShowClickMoveIndicator(scene, action.worldPos);
		return;
	}

	if (action.type == QueuedActionType::InteractTable) {
		LogicManager& logicMgr = scene.GetLogicManager();
		TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(action.tableID);
		if (!tableLogic) {
			return;
		}

		if (IsInTableInteractionRange(scene, action.tableID)) {
			CancelQueuedTableMove(scene);
			InteractWithTable(scene, action.tableID);
			return;
		}

		GameObject* player = GetOwner(scene);
		if (!player) {
			return;
		}

		const glm::vec2 playerPos = ToVec2(player->GetPositionGLM());
		Math::Vector2D from(playerPos.x, playerPos.y);
		Math::Vector2D approach = tableLogic->GetClosestApproachPoint(scene, from);
		const glm::vec2 target(approach.x, approach.y);

		pendingTableID = action.tableID;
		MoveTo(scene, target);
		ShowClickMoveIndicator(scene, target);
	}
}

/**
 * @brief Resets mouse drag state.
 * @return Result produced by this operation.
 */
void PlayerLogic::ResetMouseDragState() {
	mouseDragActive_ = false;
	hasLastDragWorld_ = false;
	dragRetargetTimer_ = 0.0f;
}

/**
 * @brief Attempts to get mouse world.
 * @param scene Scene being processed.
 * @param input Input manager for the current frame.
 * @param mouseWorld Parameter for mouse world.
 * @return Result produced by this operation.
 */
bool PlayerLogic::TryGetMouseWorld(Scene& scene, InputManager& input, glm::vec2& mouseWorld) const {
	if (input.IsReplayOverride()) {
		const glm::dvec2 replayMousePos = input.GetMousePosition();
		return scene.GetGraphicsEngine().GetMouseWorldInScene(mouseWorld, &replayMousePos);
	}

	return scene.GetGraphicsEngine().GetMouseWorldInScene(mouseWorld, nullptr);
}

/**
 * @brief Clears movement target.
 * @param scene Scene being processed.
 * @return Result produced by this operation.
 */
void PlayerLogic::ClearMovementTarget(Scene& scene) {
	hasMoveTarget = false;
	blockedMoveFrames_ = 0;

	if (GameObject* player = GetOwner(scene)) {
		scene.GetMovementManager().ClearMoveTarget(player->GetID());
	}
}

/**
 * @brief Updates sprite.
 * @param scene Scene being processed.
 * @param player Parameter for player.
 * @param moveDirRaw Parameter for move dir raw.
 * @return Result produced by this operation.
 */
void PlayerLogic::UpdateSprite(Scene& scene, GameObject* player, const glm::vec2& moveDirRaw) {
	(void)scene;
	if (!player) {
		return;
	}

	const float moveThreshold = 0.01f;
	std::string desiredAnimation;
	const bool isHolding = carriedItemID >= 0;

	float absX = std::abs(moveDirRaw.x);
	float absY = std::abs(moveDirRaw.y);

	// Detect idle/no movement
	if (glm::length(moveDirRaw) < moveThreshold) {
		switch (facingDir) {
		case FacingDir::Right: desiredAnimation = isHolding ? "IDLE_RIGHT_CARRY" : "IDLE_RIGHT"; break;
		case FacingDir::Left:  desiredAnimation = isHolding ? "IDLE_LEFT_CARRY" : "IDLE_LEFT";  break;
		case FacingDir::Front: desiredAnimation = isHolding ? "IDLE_FRONT_CARRY" : "IDLE_FRONT"; break;			// Using normal idle for front since we dont have idle front carry animations yet
		case FacingDir::Back:  desiredAnimation = isHolding ? "IDLE_BACK_CARRY" : "IDLE_BACK";  break;			// Using normal idle for back since we dont have idle back carry animations yet
		}
	}
	else {
		if (absX > absY) {
			if (moveDirRaw.x > 0.0f) {
				desiredAnimation = isHolding ? "CARRY_RIGHT" : "WALK_RIGHT";
				facingDir = FacingDir::Right;
			}
			else {
				desiredAnimation = isHolding ? "CARRY_LEFT" : "WALK_LEFT";
				facingDir = FacingDir::Left;
			}
		}
		else {
			if (moveDirRaw.y > 0.0f) {
				desiredAnimation = isHolding ? "CARRY_FRONT" : "WALK_FRONT";
				facingDir = FacingDir::Front;
			}
			else {
				desiredAnimation = isHolding ? "CARRY_BACK" : "WALK_BACK";
				facingDir = FacingDir::Back;
			}
		}
	}

	// Query the currently playing animation (to prevent animation resets due to the same input pressed)
	std::string currentAnimation = scene.GetCurrentAnimationName(player->GetID());
	if (desiredAnimation != currentAnimation) {
		scene.SetAnimation(player->GetID(), desiredAnimation);
	}
}

/**
 * @brief Moves direct.
 * @param dest Parameter for dest.
 * @return Result produced by this operation.
 */
void PlayerLogic::MoveDirect(const glm::vec2& dest) {
	const float kRetargetEpsSq = 16.0f * 16.0f;

	if (hasMoveTarget && moveMode_ == MoveMode::Direct) {
		glm::vec2 d = dest - finalTarget_;
		if ((d.x * d.x + d.y * d.y) <= kRetargetEpsSq) {
			return;
		}
	}

	finalTarget_ = dest;
	moveTarget = dest;
	pathPoints_.clear();
	pathIndex_ = 0;
	hasMoveTarget = true;
	moveMode_ = MoveMode::Direct;
}

/**
 * @brief Moves to.
 * @param scene Scene being processed.
 * @param dest Parameter for dest.
 * @return Result produced by this operation.
 */
void PlayerLogic::MoveTo(Scene& scene, const glm::vec2& dest) {
	GameObject* player = GetOwner(scene);
	if (!player)
		return;

	glm::vec2 snappedDest = dest;

	// Snap interactable destinations to the nearest walkable cell center
	scene.GetNearestNavigationCellCenterForObject(player->GetID(), dest, snappedDest);

	const float kRetargetEpsSq = 16.0f * 16.0f;

	if (hasMoveTarget && moveMode_ == MoveMode::Pathfinding) {
		glm::vec2 d = snappedDest - finalTarget_;
		if ((d.x * d.x + d.y * d.y) <= kRetargetEpsSq) {
			return;
		}
	}

	finalTarget_ = snappedDest;
	pathPoints_.clear();
	pathIndex_ = 0;
	hasMoveTarget = false;
	moveMode_ = MoveMode::Pathfinding;

	const glm::vec3 pos3 = player->GetPositionGLM();
	const glm::vec2 startPos(pos3.x, pos3.y);

	if (!scene.FindPathForObject(player->GetID(), startPos, finalTarget_, pathPoints_)) {
		TS_LOG_DEBUG("[PlayerLogic] No path found.");
		pendingTableID = -1;
		moveMode_ = MoveMode::None;
		return;
	}

	while (!pathPoints_.empty()) {
		glm::vec2 d = pathPoints_.front() - startPos;
		const float kSkipWaypointRadius = 18.0f;
		if ((d.x * d.x + d.y * d.y) <= kSkipWaypointRadius * kSkipWaypointRadius) {
			pathPoints_.erase(pathPoints_.begin());
		}
		else {
			break;
		}
	}

	if (pathPoints_.empty()) {
		moveMode_ = MoveMode::None;
		OnArrived(scene);
		return;
	}

	hasMoveTarget = true;
	moveTarget = pathPoints_[0];
}

/**
 * @brief Returns whether in table interaction range.
 * @param scene Scene being processed.
 * @param tableObjectID Parameter for table object id.
 * @return True when the operation succeeds or the condition is met.
 */
bool PlayerLogic::IsInTableInteractionRange(Scene& scene, int tableObjectID) {
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
		Math::Vector2D from(playerPos.x, playerPos.y);
		Math::Vector2D approach = tableLogic->GetClosestApproachPoint(scene, from);

		const float dx = playerPos.x - approach.x;
		const float dy = playerPos.y - approach.y;
		const float distSq = dx * dx + dy * dy;

		if (distSq <= kPlayerInteractRadius * kPlayerInteractRadius) {
			return true;
		}
	}

	glm::vec2 playerCenter, playerHalf;
	glm::vec2 tableCenter, tableHalf;

	if (GetObjectRect(player, playerCenter, playerHalf) &&
		GetObjectRect(tableObj, tableCenter, tableHalf)) {
		constexpr float kTouchPadding = 14.0f;

		glm::vec2 expandedHalf(
			tableHalf.x + playerHalf.x + kTouchPadding,
			tableHalf.y + playerHalf.y + kTouchPadding
		);

		const float distSqToTableBody =
			DistanceSqPointToExpandedRect(playerCenter, tableCenter, expandedHalf);

		if (distSqToTableBody <= 0.0001f) {
			return true;
		}
	}

	return false;
}

/**
 * @brief Performs cancel queued table move.
 * @param scene Scene being processed.
 * @return True when the operation succeeds or the condition is met.
 */
void PlayerLogic::CancelQueuedTableMove(Scene& scene) {
	hasMoveTarget = false;
	moveMode_ = MoveMode::None;
	pathPoints_.clear();
	pathIndex_ = 0;
	pendingTableID = -1;

	if (GameObject* player = GetOwner(scene)) {
		scene.GetMovementManager().ClearMoveTarget(player->GetID());
	}
}

/**
 * @brief Performs enter pause state.
 * @param scene Scene being processed.
 * @return Result produced by this operation.
 */
void PlayerLogic::EnterPauseState(Scene& scene) {
	ClearInteractableVisualCues(scene);
	ClearClickMoveIndicator(scene);
	CancelQueuedTableMove(scene);
	ClearQueuedAction();
	ResetMouseDragState();
	suppressMouseUntilRelease_ = true;
}

/**
 * @brief Handles click input.
 * @param scene Scene being processed.
 * @param input Input manager for the current frame.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void PlayerLogic::HandleClickInput(Scene& scene, InputManager& input, float dt) {
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

	// ------------------------------------------------------------
	// If player is currently locked (e.g. chopping), allow ONLY one
	// queued next action. No drag-retarget spam while locked.
	// ------------------------------------------------------------
	if (movementLocked_) {
		ResetMouseDragState();
		return;
	}

	// If we're extremely close to finishing the currently pending table interaction,
	// do NOT let drag/click cancel it at the last moment.
	if (lmbHeld && !lmbJustPressed && pendingTableID >= 0 && IsInTableCommitRange(scene, pendingTableID)) {
		return;
	}

	// Normal drag-to-move behaviour
	if (lmbHeld && !lmbJustPressed) {
		if (!mouseDragActive_) {
			mouseDragActive_ = true;
			hasLastDragWorld_ = false;
			dragRetargetTimer_ = 0.0f;
		}

		dragRetargetTimer_ -= dt;
		const float minDragRetargetDistSq = kDragRetargetDistance * kDragRetargetDistance;
		const glm::vec2 delta = mouseWorld - lastDragWorld_;
		const bool movedEnough = !hasLastDragWorld_ ||
			DistanceSquared(delta, glm::vec2(0.0f, 0.0f)) >= minDragRetargetDistSq;

		if (movedEnough && dragRetargetTimer_ <= 0.0f) {
			ClearQueuedAction();
			pendingTableID = -1;
			MoveTo(scene, mouseWorld);
			ShowClickMoveIndicator(scene, mouseWorld);
			lastDragWorld_ = mouseWorld;
			hasLastDragWorld_ = true;
			dragRetargetTimer_ = kDragRetargetInterval;
		}
		return;
	}

	mouseDragActive_ = true;
	lastDragWorld_ = mouseWorld;
	hasLastDragWorld_ = true;
	dragRetargetTimer_ = 0.0f;

	int clickedTableID = -1;
	TableLogic* clickedTableLogic = nullptr;
	const bool clickedTable =
		TryResolveClickedTableTarget(scene, mouseWorld, clickedTableID, clickedTableLogic);

	// ------------------------------------------------------------
	// COMMIT RULE:
	// If the player is already very close to completing the currently
	// pending interaction, finish THAT first, then do one queued action.
	// ------------------------------------------------------------
	if (pendingTableID >= 0 && IsInTableCommitRange(scene, pendingTableID)) {
		const int committedTableID = pendingTableID;

		if (clickedTable) {
			if (clickedTableID != committedTableID) {
				QueueTableAction(clickedTableID);

				const glm::vec2 playerPos = ToVec2(player->GetPositionGLM());
				Math::Vector2D from(playerPos.x, playerPos.y);
				Math::Vector2D approach = clickedTableLogic->GetClosestApproachPoint(scene, from);
				ShowClickMoveIndicator(scene, glm::vec2(approach.x, approach.y));
			}
			else {
				ClearQueuedAction();
			}
		}
		else {
			QueueMoveAction(mouseWorld);
			ShowClickMoveIndicator(scene, mouseWorld);
		}

		CancelQueuedTableMove(scene);
		InteractWithTable(scene, committedTableID);

		if (!movementLocked_) {
			ExecuteQueuedAction(scene);
		}
		return;
	}

	// ------------------------------------------------------------
	// Normal click handling
	// ------------------------------------------------------------
	if (clickedTable && clickedTableLogic) {
		ClearQueuedAction();

		if (IsInTableInteractionRange(scene, clickedTableID)) {
			CancelQueuedTableMove(scene);
			InteractWithTable(scene, clickedTableID);
			return;
		}

		pendingTableID = clickedTableID;

		const glm::vec2 playerPos = ToVec2(player->GetPositionGLM());
		Math::Vector2D from(playerPos.x, playerPos.y);
		Math::Vector2D approach = clickedTableLogic->GetClosestApproachPoint(scene, from);
		const glm::vec2 target(approach.x, approach.y);

		MoveTo(scene, target);
		ShowClickMoveIndicator(scene, target);
		return;
	}

	ClearQueuedAction();
	pendingTableID = -1;
	MoveTo(scene, mouseWorld);
	ShowClickMoveIndicator(scene, mouseWorld);
}

/**
 * @brief Updates movement.
 * @param dt Frame delta time in seconds.
 * @param scene Scene being processed.
 * @return Result produced by this operation.
 */
void PlayerLogic::UpdateMovement(float dt, Scene& scene) {
	if (movementLocked_)
		return;

	if (!hasMoveTarget)
		return;

	GameObject* player = GetOwner(scene);
	if (!player)
		return;

	glm::vec3 pos3 = player->GetPositionGLM();
	glm::vec2 pos(pos3.x, pos3.y);

	// -------------------------------------------------
// LIVE SHORTCUT CHECK:
// while following an A* path, if final target is now
// directly reachable, switch immediately to direct mode
// -------------------------------------------------
	if (moveMode_ == MoveMode::Pathfinding) {
		directPathCheckTimer_ -= dt;

		if (directPathCheckTimer_ <= 0.0f) {
			directPathCheckTimer_ = kDirectPathCheckInterval;

			if (scene.HasDirectPathForObject(player->GetID(), pos, finalTarget_)) {
				moveMode_ = MoveMode::Direct;
				moveTarget = finalTarget_;
				pathPoints_.clear();
				pathIndex_ = 0;
				hasMoveTarget = true;
			}
		}
	}

	const float arriveRadius = 6.0f;
	const float arriveRadiusSq = arriveRadius * arriveRadius;

	// ---------------------------
	// DIRECT FREE MOVEMENT MODE
	// ---------------------------
	if (moveMode_ == MoveMode::Direct) {
		moveTarget = finalTarget_;

		glm::vec2 dir = moveTarget - pos;
		float distSq = dir.x * dir.x + dir.y * dir.y;

		if (distSq <= arriveRadiusSq) {
			hasMoveTarget = false;
			moveMode_ = MoveMode::None;
			pathPoints_.clear();
			pathIndex_ = 0;

			if (GameObject* p = GetOwner(scene)) {
				scene.GetMovementManager().ClearMoveTarget(p->GetID());
			}

			OnArrived(scene);
			return;
		}

		float dist = std::sqrt(distSq);
		if (dist > 0.0001f) {
			dir.x /= dist;
			dir.y /= dist;
		}

		float step = moveSpeed * dt;
		if (step > dist)
			step = dist;

		glm::vec2 desiredDelta(dir.x * step, dir.y * step);
		glm::vec2 allowedDelta = scene.ResolveWorldStep(player, desiredDelta);

		const float allowedLenSq =
			allowedDelta.x * allowedDelta.x +
			allowedDelta.y * allowedDelta.y;

		if (allowedLenSq < 0.0001f) {
			std::vector<glm::vec2> newPath;

			if (scene.FindPathForObject(player->GetID(), pos, finalTarget_, newPath)) {
				pathPoints_ = newPath;
				pathIndex_ = 0;
				moveMode_ = MoveMode::Pathfinding;
				hasMoveTarget = !pathPoints_.empty();

				while (!pathPoints_.empty()) {
					glm::vec2 d = pathPoints_.front() - pos;
					if ((d.x * d.x + d.y * d.y) <= arriveRadiusSq) {
						pathPoints_.erase(pathPoints_.begin());
					}
					else {
						break;
					}
				}

				if (!pathPoints_.empty()) {
					moveTarget = pathPoints_.front();
					return;
				}
			}

			hasMoveTarget = false;
			moveMode_ = MoveMode::None;
			pathPoints_.clear();
			pathIndex_ = 0;
			pendingTableID = -1;

			if (GameObject* p = GetOwner(scene)) {
				scene.GetMovementManager().ClearMoveTarget(p->GetID());
			}
			return;
		}

		pos.x += allowedDelta.x;
		pos.y += allowedDelta.y;

		player->SetPosition(glm::vec3(pos.x, pos.y, pos3.z));
		scene.ClampToWalkArea(player);

		UpdateSprite(scene, player, allowedDelta);
		return;
	}

	// ---------------------------
	// PATHFINDING MODE
	// ---------------------------
	if (pathPoints_.empty()) {
		hasMoveTarget = false;
		moveMode_ = MoveMode::None;
		return;
	}

	// Advance waypoint(s) if already reached
	while (pathIndex_ < pathPoints_.size()) {
		glm::vec2 toWaypoint = pathPoints_[pathIndex_] - pos;
		float distSq = toWaypoint.x * toWaypoint.x + toWaypoint.y * toWaypoint.y;

		if (distSq <= arriveRadiusSq) {
			++pathIndex_;
		}
		else {
			break;
		}
	}

	if (pathIndex_ >= pathPoints_.size()) {
		hasMoveTarget = false;
		moveMode_ = MoveMode::None;
		pathPoints_.clear();
		pathIndex_ = 0;

		if (GameObject* p = GetOwner(scene)) {
			scene.GetMovementManager().ClearMoveTarget(p->GetID());
		}

		OnArrived(scene);
		return;
	}

	moveTarget = pathPoints_[pathIndex_];

	glm::vec2 dir = moveTarget - pos;
	float distSq = dir.x * dir.x + dir.y * dir.y;
	float dist = std::sqrt(distSq);

	if (dist > 0.0001f) {
		dir.x /= dist;
		dir.y /= dist;
	}

	float step = moveSpeed * dt;
	if (step > dist)
		step = dist;

	glm::vec2 desiredDelta(dir.x * step, dir.y * step);
	glm::vec2 allowedDelta = scene.ResolveWorldStep(player, desiredDelta);

	const float allowedLenSq =
		allowedDelta.x * allowedDelta.x +
		allowedDelta.y * allowedDelta.y;

	if (allowedLenSq < 0.0001f) {
		std::vector<glm::vec2> newPath;

		if (scene.FindPathForObject(player->GetID(), pos, finalTarget_, newPath)) {
			pathPoints_ = newPath;
			pathIndex_ = 0;

			while (!pathPoints_.empty()) {
				glm::vec2 d = pathPoints_.front() - pos;
				if ((d.x * d.x + d.y * d.y) <= arriveRadiusSq) {
					pathPoints_.erase(pathPoints_.begin());
				}
				else {
					break;
				}
			}

			if (!pathPoints_.empty()) {
				moveTarget = pathPoints_.front();
				return;
			}
		}

		TS_LOG_DEBUG("[PlayerLogic] Path blocked and repath failed.");

		hasMoveTarget = false;
		moveMode_ = MoveMode::None;
		pathPoints_.clear();
		pathIndex_ = 0;
		pendingTableID = -1;

		if (GameObject* p = GetOwner(scene)) {
			scene.GetMovementManager().ClearMoveTarget(p->GetID());
		}
		return;
	}

	pos.x += allowedDelta.x;
	pos.y += allowedDelta.y;

	player->SetPosition(glm::vec3(pos.x, pos.y, pos3.z));
	scene.ClampToWalkArea(player);

	UpdateSprite(scene, player, allowedDelta);

	if (DebugRenderer::IsEnabled() && hasMoveTarget) {
		glm::vec2 prev(pos.x, pos.y);

		for (std::size_t i = pathIndex_; i < pathPoints_.size(); ++i) {
			glm::vec2 next = pathPoints_[i];
			DebugRenderer::DrawLine(
				glm::vec3(prev.x, prev.y, pos3.z),
				glm::vec3(next.x, next.y, pos3.z),
				glm::vec3(0.0f, 1.0f, 0.0f)
			);
			prev = next;
		}
	}
}

// Unity: OnArrived()
/**
 * @brief Performs on arrived.
 * @param scene Scene being processed.
 * @return Result produced by this operation.
 */
void PlayerLogic::OnArrived(Scene& scene) {
	if (pendingTableID < 0) {
		return;
	}

	if (IsInTableInteractionRange(scene, pendingTableID)) {
		InteractWithTable(scene, pendingTableID);
	}

	pendingTableID = -1;
}

/**
 * @brief Performs pick up.
 * @param scene Scene being processed.
 * @param itemID Parameter for item id.
 * @return Result produced by this operation.
 */
void PlayerLogic::PickUp(Scene& scene, int itemID) {
	GameObject* item = scene.GetGameObjectByID(itemID);
	GameObject* player = GetOwner(scene);
	if (!item || !player) {
		return;
	}

	carriedItemID = itemID;

	// Store original layer so we can restore on drop
	carriedItemOriginalLayer_ = scene.GetObjectLayer(itemID);
	hasCarriedItemOriginalLayer_ = true;

	// Play UI click sound for pickup feedback (release mode only)
#ifndef _DEBUG
	if (AudioManager* audioMgr = scene.GetAudioManager()) {
		glm::vec3 playerPos = player->GetPositionGLM();
		audioMgr->PlaySound3D("ui_click", playerPos.x, playerPos.y, playerPos.z,
			audioMgr->GetVfxVolume() * 0.3f);
	}
#endif

	//std::cout << "[PlayerLogic] PickUp item " << itemID << "\n";

	// Save original collider size
	carriedItemOriginalColliderSize = item->GetColliderSize();
	hasCarriedItemOriginalColliderSize = true;

	// Shrink collider so physics stops pushing the player around
	// (Adjust to your GameObject API if needed)
	item->SetColliderSize(Math::Vector2D(0.f, 0.f));

	// Snap once, then every frame we keep it following in UpdateCarriedItemTransform
	UpdateCarriedItemTransform(scene);
}

/**
 * @brief Performs drop.
 * @param scene Scene being processed.
 * @return Result produced by this operation.
 */
void PlayerLogic::Drop(Scene& scene) {
	if (carriedItemID < 0) {
		return;
	}

	GameObject* player = GetOwner(scene);
	GameObject* item = scene.GetGameObjectByID(carriedItemID);
	if (!player || !item) {
		//std::cout << "[PlayerLogic] Drop failed, invalid item or player. Clearing carriedItemID.\n";
		carriedItemID = -1;
		return;
	}

	//std::cout << "[PlayerLogic] Drop item " << carriedItemID << "\n";

	if (hasCarriedItemOriginalColliderSize) {
		item->SetColliderSize(carriedItemOriginalColliderSize);
		hasCarriedItemOriginalColliderSize = false;
	}

	// Restore original layer so it goes back to normal rendering order
	RestoreCarriedItemLayer(scene, carriedItemID);

	glm::vec3 p = player->GetPositionGLM();
	item->SetPosition(glm::vec3(p.x + 16.f, p.y, p.z)); // simple in front drop
	carriedItemID = -1;
}

/**
 * @brief Handles keyboard movement.
 * @param dt Frame delta time in seconds.
 * @param scene Scene being processed.
 * @param input Input manager for the current frame.
 * @param player Parameter for player.
 * @param playerPos Parameter for player pos.
 * @return Result produced by this operation.
 */
void PlayerLogic::HandleKeyboardMovement(float dt, Scene& scene, InputManager& input, GameObject* player, const glm::vec3& playerPos) {
	if (movementLocked_) {
		UpdateSprite(scene, player, glm::vec2(0.0f, 0.0f));
		return;
	}

	glm::vec2 inputDir(0.0f, 0.0f);
	if (input.IsKeyPressed(GLFW_KEY_A)) inputDir.x -= 1.0f;
	if (input.IsKeyPressed(GLFW_KEY_D)) inputDir.x += 1.0f;
	if (input.IsKeyPressed(GLFW_KEY_W)) inputDir.y -= 1.0f;
	if (input.IsKeyPressed(GLFW_KEY_S)) inputDir.y += 1.0f;

	if (inputDir.x != 0.0f || inputDir.y != 0.0f) {
		hasMoveTarget = false;
		moveMode_ = MoveMode::None;
		pathPoints_.clear();
		pathIndex_ = 0;
		pendingTableID = -1;
		ClearQueuedAction();

		const glm::vec2 normalizedInput = NormalizeOrZero(inputDir);
		glm::vec2 desiredDelta(normalizedInput.x * moveSpeed * dt,
			normalizedInput.y * moveSpeed * dt);
		glm::vec2 allowedDelta = scene.ResolveWorldStep(player, desiredDelta);

		glm::vec3 pos3 = playerPos;
		pos3.x += allowedDelta.x;
		pos3.y += allowedDelta.y;
		player->SetPosition(pos3);
		scene.ClampToWalkArea(player);
		UpdateSprite(scene, player, allowedDelta);
		return;
	}

	if (hasMoveTarget) {
		UpdateSprite(scene, player, moveTarget - ToVec2(player->GetPositionGLM()));
	}
	else {
		UpdateSprite(scene, player, glm::vec2(0.0f, 0.0f));
	}
}

/**
 * @brief Updates footstep trail and audio.
 * @param dt Frame delta time in seconds.
 * @param scene Scene being processed.
 * @param input Input manager for the current frame.
 * @param player Parameter for player.
 * @param beforePos Parameter for before pos.
 * @param afterPos Parameter for after pos.
 * @return Result produced by this operation.
 */
void PlayerLogic::UpdateFootstepTrailAndAudio(float dt, Scene& scene, InputManager& input, GameObject* player, const glm::vec3& beforePos, const glm::vec3& afterPos) {
	(void)dt;
	const bool hasIntent =
		input.IsKeyPressed(GLFW_KEY_A) || input.IsKeyPressed(GLFW_KEY_D) ||
		input.IsKeyPressed(GLFW_KEY_W) || input.IsKeyPressed(GLFW_KEY_S) ||
		hasMoveTarget;

	const glm::vec2 moveDelta(afterPos.x - beforePos.x, afterPos.y - beforePos.y);
	const bool actuallyMoved = std::sqrt(moveDelta.x * moveDelta.x + moveDelta.y * moveDelta.y) > kTrailJitterEpsilon;
	if (!hasIntent || !actuallyMoved) {
		hasLastTrailPos_ = false;
		trailCarry_ = 0.0f;
		footstepEmitTimer_ = 0.0f;
		return;
	}

#ifndef _DEBUG
	footstepEmitTimer_ += dt;
	if (footstepEmitTimer_ >= kFootstepInterval) {
		footstepEmitTimer_ = 0.0f;
		if (AudioManager* audioMgr = scene.GetAudioManager()) {
			audioMgr->PlaySound3D("sfx_step_1", afterPos.x, afterPos.y, afterPos.z,
				audioMgr->GetVfxVolume() * 0.04f);
		}
	}
#endif

	glm::vec3 feet = afterPos;
	const auto co = player->GetColliderOffset();
	const auto scale = player->GetScaleGLM();
	feet.x += co.x;
	feet.y += co.y + (scale.y * 0.5f) - 6.0f;

	const glm::vec2 dir = NormalizeOrZero(moveDelta);
	if (!hasLastTrailPos_) {
		lastTrailPos_ = feet;
		hasLastTrailPos_ = true;
		trailCarry_ = 0.0f;
	}

	const glm::vec2 a(lastTrailPos_.x, lastTrailPos_.y);
	const glm::vec2 b(feet.x, feet.y);
	const glm::vec2 d = b - a;
	const float segmentDist = std::sqrt(d.x * d.x + d.y * d.y);
	if (segmentDist > 0.0001f) {
		const glm::vec2 segDir = d / segmentDist;
		const float spacing = 15.0f;
		const float total = segmentDist + trailCarry_;
		const int count = static_cast<int>(std::floor(total / spacing));

		for (int i = 0; i < count; ++i) {
			const float along = spacing * (i + 1) - trailCarry_;
			const glm::vec2 p2 = a + segDir * along;
			glm::vec3 trailPos(p2.x, p2.y, afterPos.z);

			const float behind = 20.0f;
			trailPos.x -= dir.x * behind;
			trailPos.y -= dir.y * behind;

			const glm::vec2 perp(-dir.y, dir.x);
			static std::uniform_real_distribution<float> jitterDist(-1.5f, 1.5f);
			const float jitter = jitterDist(EngineRng::Get());
			trailPos.x += perp.x * jitter;
			trailPos.y += perp.y * jitter;

			scene.GetParticleSystem().EmitTrail(scene.GetEntityManager(), trailPos, afterPos.z, dir);
		}

		trailCarry_ = total - count * spacing;
	}

	lastTrailPos_ = feet;
}

/**
 * @brief Updates this object.
 * @param dt Frame delta time in seconds.
 * @param scene Scene being processed.
 * @param input Input manager for the current frame.
 * @return Result produced by this operation.
 */
void PlayerLogic::Update(float dt, Scene& scene, InputManager& input) {
	const float safeDt = std::clamp(dt, 0.0f, kMaxPlayerUpdateDt);

	if (!scene.IsSimulationActive() || scene.IsPauseOverlayActive()) {
		EnterPauseState(scene);
		return;
	}

	if (suppressMouseUntilRelease_) {
		if (input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) ||
			input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
			return;
		}

		suppressMouseUntilRelease_ = false;
		return;
	}

	const std::string levelPath = scene.GetCurrentLevelPath();
	const bool isLevel1 = levelPath.find("kitchen01") != std::string::npos;
	const bool isLevel2 = levelPath.find("kitchen02") != std::string::npos;
	const bool forceClearShortcut =
		((isLevel1 || isLevel2) && input.IsKeyJustPressed(GLFW_KEY_F10));

	if (forceClearShortcut && !Economy::gQuotaReached) {
		Economy::gPlayerMoney = Economy::kQuota;
		Economy::SyncUI();
		Economy::gQuotaReached = true;
		Economy::OnQuotaReached(scene);
		return;
	}

	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	const bool wasLockedBefore = movementLocked_;
	UpdateStationLock(scene);

	if (wasLockedBefore && !movementLocked_) {
		ExecuteQueuedAction(scene);
	}

	UpdateInteractableVisualCues(scene, input, safeDt);
	UpdateClickMoveIndicator(scene, safeDt);

	// IMPORTANT: if chopping is active, do nothing else this frame
	if (movementLocked_ && ShouldPlayChopAnimation(scene)) {
		ResetMouseDragState();
		EnsureChopAnimation(scene, player);
		UpdateCarriedItemTransform(scene);
		return;
	}

	const glm::vec3 beforePos = player->GetPositionGLM();

	const float physicsDt = scene.GetLastPhysicsDt();
	const bool stepMode = scene.GetStepController().enabled;
	if (stepMode && physicsDt <= 0.0f) {
		HandleClickInput(scene, input, safeDt);
		return;
	}

	HandleKeyboardMovement(safeDt, scene, input, player, beforePos);
	HandleClickInput(scene, input, safeDt);
	UpdateMovement(safeDt, scene);

	const glm::vec3 afterPos = player->GetPositionGLM();
	UpdateFootstepTrailAndAudio(safeDt, scene, input, player, beforePos, afterPos);
	UpdateCarriedItemTransform(scene);
}

/**
 * @brief Updates interactable visual cues.
 * @param scene Scene being processed.
 * @param input Input manager for the current frame.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void PlayerLogic::UpdateInteractableVisualCues(Scene& scene, InputManager& input, float dt) {
	(void)dt;

	glm::vec2 mouseWorld{};
	const bool hasMouseWorld = TryGetMouseWorld(scene, input, mouseWorld);

	std::unordered_set<int> nextHighlighted;
	nextHighlighted.reserve(16);

	LogicManager& logicMgr = scene.GetLogicManager();

	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		if (!obj) {
			continue;
		}

		const int id = obj->GetID();
		TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(id);
		if (!tableLogic) {
			continue;
		}

		const bool hovered = hasMouseWorld && IsPointInsideObjectCollider(obj, mouseWorld);
		if (!hovered) {
			continue;
		}

		nextHighlighted.insert(id);
		EnsureHoverOutline(scene, obj, id);
	}

	for (int id : highlightedInteractableIDs_) {
		if (nextHighlighted.find(id) != nextHighlighted.end()) {
			continue;
		}
		RemoveHoverOutline(scene, id);
	}

	highlightedInteractableIDs_ = std::move(nextHighlighted);
}

/**
 * @brief Returns whether point inside object collider.
 * @param obj Parameter for obj.
 * @param worldPoint Parameter for world point.
 * @return True when the operation succeeds or the condition is met.
 */
bool PlayerLogic::IsPointInsideObjectCollider(const GameObject* obj, const glm::vec2& worldPoint) const {
	if (!obj) {
		return false;
	}

	auto colSize = obj->GetColliderSize();
	if (colSize.x <= 0.0f || colSize.y <= 0.0f) {
		return false;
	}

	auto colOffset = obj->GetColliderOffset();
	glm::vec3 objPos = obj->GetPositionGLM();
	glm::vec2 center(objPos.x + colOffset.x, objPos.y + colOffset.y);

	float halfW = colSize.x * 0.5f;
	float halfH = colSize.y * 0.5f;

	return (worldPoint.x >= center.x - halfW && worldPoint.x <= center.x + halfW) &&
		(worldPoint.y >= center.y - halfH && worldPoint.y <= center.y + halfH);
}

/**
 * @brief Performs show click move indicator.
 * @param scene Scene being processed.
 * @param worldPoint Parameter for world point.
 * @return Result produced by this operation.
 */
void PlayerLogic::ShowClickMoveIndicator(Scene& scene, const glm::vec2& worldPoint) {
	const glm::vec3 markerPos(worldPoint.x, worldPoint.y, 0.0f);

	// Despawn old indicator so each click restarts the animation cleanly
	if (clickIndicatorID_ >= 0) {
		scene.DespawnByID(clickIndicatorID_);
		clickIndicatorID_ = -1;
	}

	static const std::vector<glm::vec4> kArrowFrames = []() {
		std::vector<glm::vec4> frames;
		frames.reserve(8);

		constexpr int kFrameCount = 8;
		constexpr float kFrameWidth = 1.0f / static_cast<float>(kFrameCount);

		for (int i = 0; i < kFrameCount; ++i) {
			frames.emplace_back(i * kFrameWidth, 0.0f, kFrameWidth, 1.0f);
		}

		return frames;
		}();

	std::string markerLayer = "1";
	if (GameObject* player = GetOwner(scene)) {
		const std::string playerLayer = scene.GetObjectLayer(player->GetID());
		if (!playerLayer.empty()) {
			markerLayer = playerLayer;
		}
	}

	GameObject* marker = scene.SpawnAnimatedSprite(
		"../assets/arrow-Sheet.png",
		markerPos,
		glm::vec2(kClickIndicatorBaseSize, kClickIndicatorBaseSize),
		kArrowFrames,
		0.06f,
		true,
		markerLayer
	);

	if (!marker) {
		clickIndicatorID_ = -1;
		clickIndicatorTimeLeft_ = 0.0f;
		return;
	}

	clickIndicatorID_ = marker->GetID();
	clickIndicatorTimeLeft_ = kClickIndicatorLifetime;

	marker->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
	marker->SetRenderSortOrder(1000);
	marker->SetScale(glm::vec3(kClickIndicatorBaseSize, kClickIndicatorBaseSize, 1.0f));
	marker->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

/**
 * @brief Updates click move indicator.
 * @param scene Scene being processed.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void PlayerLogic::UpdateClickMoveIndicator(Scene& scene, float dt) {
	if (clickIndicatorID_ < 0) {
		return;
	}

	GameObject* marker = scene.GetGameObjectByID(clickIndicatorID_);
	if (!marker) {
		clickIndicatorID_ = -1;
		clickIndicatorTimeLeft_ = 0.0f;
		return;
	}

	clickIndicatorTimeLeft_ -= dt;
	if (clickIndicatorTimeLeft_ <= 0.0f) {
		scene.DespawnByID(clickIndicatorID_);
		clickIndicatorID_ = -1;
		clickIndicatorTimeLeft_ = 0.0f;
		return;
	}

	// Temporarily disabled while testing whether the arrow animation is playing correctly.
	// Keep the indicator at a fixed size and fixed color.
	/*
	const float normalizedTimeLeft =
		std::clamp(clickIndicatorTimeLeft_ / kClickIndicatorLifetime, 0.0f, 1.0f);
	const float progress = 1.0f - normalizedTimeLeft;

	// Optional pulse
	const float pulse = std::sin(progress * 3.14159265f);
	const float size =
		kClickIndicatorBaseSize + pulse * (kClickIndicatorPopSize - kClickIndicatorBaseSize);

	marker->SetScale(glm::vec3(size, size, 1.0f));

	// Optional fade
	marker->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, normalizedTimeLeft));
	*/

	marker->SetScale(glm::vec3(kClickIndicatorBaseSize, kClickIndicatorBaseSize, 1.0f));
	marker->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

/**
 * @brief Clears click move indicator.
 * @param scene Scene being processed.
 * @return Result produced by this operation.
 */
void PlayerLogic::ClearClickMoveIndicator(Scene& scene) {
	if (clickIndicatorID_ >= 0) {
		scene.DespawnByID(clickIndicatorID_);
	}

	clickIndicatorID_ = -1;
	clickIndicatorTimeLeft_ = 0.0f;
}

/**
 * @brief Clears interactable visual cues.
 * @param scene Scene being processed.
 * @return Result produced by this operation.
 */
void PlayerLogic::ClearInteractableVisualCues(Scene& scene) {
	for (int id : highlightedInteractableIDs_) {
		if (GameObject* obj = scene.GetGameObjectByID(id)) {
			obj->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		}
	}

	highlightedInteractableIDs_.clear();
	ClearHoverOutlines(scene);
}

/**
 * @brief Performs interact with table.
 * @param scene Scene being processed.
 * @param tableObjectID Parameter for table object id.
 * @return Result produced by this operation.
 */
void PlayerLogic::InteractWithTable(Scene& scene, int tableObjectID) {
	//std::cout << "[PlayerLogic] InteractWithTable tableID=" << tableObjectID << "\n";

	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	// Play interact audio for the table being interacted with
	scene.PlayInteractAudio(tableObjectID);

	// Get table logic for the clicked/selected GameObject
	LogicManager& logicMgr = scene.GetLogicManager();
	TableLogic* table = logicMgr.GetLogicForObject<TableLogic>(tableObjectID);
	if (!table) {
		//std::cout << "[PlayerLogic] InteractWithTable: no TableLogic found on that object\n";
		return;
	}

	bool playerHolding = (carriedItemID >= 0);
	bool tableHasItem = table->HasItem();

	//std::cout << "  [PlayerLogic] state: playerHolding=" << playerHolding
	//	<< " carriedItemID=" << carriedItemID
	//	<< " tableHasItem=" << tableHasItem
	//	<< " tableHeldItemID=" << (tableHasItem ? table->GetHeldItemID() : -1)
	//	<< "\n";

	// --- Special case: Ingredient box ---
	if (IngredientBoxLogic* box = logicMgr.GetLogicForObject<IngredientBoxLogic>(tableObjectID)) {
		//std::cout << "  [PlayerLogic] This table is an IngredientBox\n";

		if (carriedItemID >= 0) {
			//std::cout << "  [PlayerLogic] Already holding item " << carriedItemID
			//	<< ", ignoring ingredient box\n";
			return;
		}

		int newItemID = box->SpawnIngredient(scene);
		if (newItemID >= 0) {
			PickUp(scene, newItemID);  // auto-pickup
		}
		return; // Do not fall through to normal table logic
	}

	// Special case: Customer table payment
	if (CustomerTableLogic* ctable = logicMgr.GetLogicForObject<CustomerTableLogic>(tableObjectID)) {
		// If the customer is in Paying state, consume the click and stop here.
		if (ctable->TryTakePayment(scene)) {
			return;
		}
	}

	// Special case: Trash can
	// If this object is a trash can, placing an item should delete it immediately.
	if (TrashCanLogic* trash = logicMgr.GetLogicForObject<TrashCanLogic>(tableObjectID)) {
		// Only meaningful if player is holding something
		if (carriedItemID >= 0) {
			const int itemToTrash = carriedItemID;

			// Try to "place" it on the trash can (TrashCanLogic will despawn it)
			if (trash->PlaceItem(scene, itemToTrash)) {
				// Restore collider size if the object still exists this frame
				// (depending on when despawns are processed)
				if (hasCarriedItemOriginalColliderSize) {
					if (GameObject* item = scene.GetGameObjectByID(itemToTrash)) {
						item->SetColliderSize(carriedItemOriginalColliderSize);
					}
					hasCarriedItemOriginalColliderSize = false;
				}

				RestoreCarriedItemLayer(scene, itemToTrash);

				// Clear carried item
				carriedItemID = -1;
			}
		}

		// Either way, stop here Ã¢â‚¬â€ trash can shouldn't behave like normal tables.
		return;
	}

	playerHolding = (carriedItemID >= 0);
	tableHasItem = table->HasItem();

	// CASE 1: Player empty-handed, table has an item -> pick up
	if (!playerHolding && tableHasItem) {
		// Workstations: do NOT allow taking back raw / still-processing ingredients.
		if (WorkTableLogic* wt = logicMgr.GetLogicForObject<WorkTableLogic>(tableObjectID)) {
			if (!wt->CanTakeHeldItem(scene)) {
				return;
			}
		}

		int itemID = table->TakeItem(scene);
		if (itemID >= 0) {
			PickUp(scene, itemID);
		}
		return;
	}

	// CASE 2: Player holding something, table is empty -> drop onto table
	if (playerHolding && !tableHasItem) {
		//std::cout << "  [PlayerLogic] CASE2: player holding " << carriedItemID
		//	<< ", table empty -> PlaceItem\n";

		if (table->CanAcceptItem(scene, carriedItemID)) {
			if (table->PlaceItem(scene, carriedItemID)) {
				//std::cout << "  [PlayerLogic] CASE2: PlaceItem success, clearing carriedItem\n";
				if (hasCarriedItemOriginalColliderSize) {
					if (GameObject* item = scene.GetGameObjectByID(carriedItemID))
						item->SetColliderSize(carriedItemOriginalColliderSize);
					hasCarriedItemOriginalColliderSize = false;
				}
				// R	estore original layer so it goes back to normal rendering order on the table
				RestoreCarriedItemLayer(scene, carriedItemID);
				carriedItemID = -1;

				// Play put down sound effect (release mode only)
#ifndef _DEBUG
				if (AudioManager* audioMgr = scene.GetAudioManager()) {
					glm::vec3 playerPos = player->GetPositionGLM();
					audioMgr->PlaySound3D("sfx_put_down", playerPos.x, playerPos.y, playerPos.z,
						audioMgr->GetVfxVolume() * 0.8f);
				}
#endif
			}
			else {
				//std::cout << "  [PlayerLogic] CASE2: PlaceItem FAILED\n";
			}
		}
		else {
			//std::cout << "  [PlayerLogic] CASE2: CanAcceptItem = false\n";
		}
		// --- NEW: if this is a cutting board and it started processing, lock player movement ---
		if (WorkTableLogic* wt = logicMgr.GetLogicForObject<WorkTableLogic>(tableObjectID)) {
			if (wt->LocksPlayerMovementWhileProcessing() && wt->IsProcessing()) {
				BeginStationLock(scene, tableObjectID);
			}
		}
		return;
	}

	// CASE 3: Player holding something, table already has an item
	// Supports BOTH directions:
	//   A) player holds PLATE, table has processed INGREDIENT
	//   B) player holds processed INGREDIENT, table has PLATE
	if (playerHolding && tableHasItem) {
		const int tableItemID = table->GetHeldItemID();

		PlateLogic* heldPlate = logicMgr.GetLogicForObject<PlateLogic>(carriedItemID);
		IngredientLogic* heldIngredient = logicMgr.GetLogicForObject<IngredientLogic>(carriedItemID);

		PlateLogic* tablePlate = logicMgr.GetLogicForObject<PlateLogic>(tableItemID);
		IngredientLogic* tableIngredient = logicMgr.GetLogicForObject<IngredientLogic>(tableItemID);

		// ------------------------------------------------------------
		// CASE 3A: player is holding a PLATE, table has an INGREDIENT
		// Overcooked-style:
		// - blank plate + processed ingredient on table -> plate with 1 ingredient
		// - plate with 1 ingredient + processed ingredient on table -> complete dish
		// ------------------------------------------------------------
		if (heldPlate && tableIngredient) {
			// Only allow processed ingredients to be taken onto a plate
			if (!tableIngredient->IsProcessed()) {
				return;
			}

			if (!heldPlate->CanAcceptIngredientType(tableIngredient->GetType())) {
				return;
			}

			const int ingredientObjID = tableItemID;
			const int ingredientCountBefore = heldPlate->GetIngredientCount();

			// Remove ingredient from the table first
			const int removedID = table->TakeItem(scene);
			if (removedID < 0) {
				return;
			}

			bool consumedNow = false;
			if (!heldPlate->TryAddIngredient(*tableIngredient, consumedNow)) {
				// Safety revert if something failed unexpectedly
				table->PlaceItem(scene, removedID);
				return;
			}

			// First ingredient on held plate -> keep ingredient object as visual child
			if (ingredientCountBefore == 0) {
				GameObject* plateObj = scene.GetGameObjectByID(carriedItemID);
				GameObject* ingredientObj = scene.GetGameObjectByID(ingredientObjID);

				if (plateObj && ingredientObj) {
					const glm::vec3 platePos = plateObj->GetPositionGLM();
					ingredientObj->SetPosition(platePos);

					heldPlate->SetFirstIngredientObjectID(ingredientObjID);
					ingredientObj->SetColliderSize(Math::Vector2D(0.f, 0.f));
					ingredientObj->SetMovableByPhysics(false);
				}
			}

			// Try to assemble if this becomes the second ingredient
			DishType dishType;
			std::vector<IngredientType> consumedTypes;
			if (heldPlate->TryAssembleDish(dishType, consumedTypes)) {
				heldPlate->ApplyDishVisual(scene);

				// Remove old first-ingredient visual if any
				const int firstObjID = heldPlate->GetFirstIngredientObjectID();
				if (firstObjID >= 0) {
					scene.DespawnByID(firstObjID);
					heldPlate->SetFirstIngredientObjectID(-1);
				}

				// Remove the second ingredient object taken from the table
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
			return;
		}

		// ------------------------------------------------------------
		// CASE 3B: player is holding an INGREDIENT, table has a PLATE
		// (your original behaviour, kept here)
		// ------------------------------------------------------------
		if (tablePlate && heldIngredient) {
			const int ingredientObjID = heldIngredient->GetOwnerID();
			const int ingredientCountBefore = tablePlate->GetIngredientCount();

			bool consumedNow = false;
			if (tablePlate->TryAddIngredient(*heldIngredient, consumedNow)) {
				// First ingredient placed onto the plate -> keep ingredient object as visual child
				if (ingredientCountBefore == 0) {
					GameObject* plateObj = scene.GetGameObjectByID(tableItemID);
					GameObject* ingredientObj = scene.GetGameObjectByID(ingredientObjID);

					if (plateObj && ingredientObj) {
						glm::vec3 platePos = plateObj->GetPositionGLM();
						ingredientObj->SetPosition(platePos);

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
			}

			return;
		}

		return;
	}

	//std::cout << "  [PlayerLogic] No case matched, doing nothing.\n";
}

/**
 * @brief Updates carried item transform.
 * @param scene Scene being processed.
 * @return Result produced by this operation.
 */
void PlayerLogic::UpdateCarriedItemTransform(Scene& scene) {
	if (carriedItemID < 0) {
		return;
	}

	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	GameObject* item = scene.GetGameObjectByID(carriedItemID);
	if (!item) {
		return;
	}

	const glm::vec2 carry = GetCarryOffsetForFacing();
	glm::vec3 p = player->GetPositionGLM();
	item->SetPosition(glm::vec3(p.x + carry.x,
		p.y + carry.y,
		p.z));

	// Base sort order for carried item
	item->SetRenderSortOrder(0);

	// Layer adjustment so the carried item renders above the player sprite
	ApplyCarryLayer(scene, carriedItemID);

	// If the carried item is a plate with 1 ingredient attached visually, move that ingredient too.
	if (auto* plate = scene.GetLogicManager().GetLogicForObject<PlateLogic>(carriedItemID)) {
		const int child = plate->GetFirstIngredientObjectID();
		if (child >= 0 && !plate->HasPreparedDish()) {
			if (GameObject* ingObj = scene.GetGameObjectByID(child)) {
				// Follow the plate exactly (same position as plate)
				glm::vec3 platePos = item->GetPositionGLM();
				ingObj->SetPosition(platePos);

				// Also adjust layer to match the plate's layer offset (so it renders above the plate)
				const std::string playerLayer = scene.GetObjectLayer(player->GetID());
				const std::string childLayer = GetCarryChildLayerForFacing(playerLayer);
				if (!childLayer.empty()) {
					scene.AssignObjectToLayer(child, childLayer);
				}

				// Ensure stable ordering above the plate
				ingObj->SetRenderSortOrder(1);

				// Make sure it doesn't collide / get pushed
				ingObj->SetColliderSize(Math::Vector2D(0.f, 0.f));
				ingObj->SetMovableByPhysics(false);
			}
		}
	}

	// Debug
	//std::cout << "[PlayerLogic] Updating carried item " << carriedItemID
	//	<< " to follow player at (" << p.x + carryOffset.x << ", "
	//	<< p.y + carryOffset.y << ")\n";
}

/**
 * @brief Begins station lock.
 * @param scene Scene being processed.
 * @param tableID Parameter for table id.
 * @return Result produced by this operation.
 */
void PlayerLogic::BeginStationLock(Scene& scene, int tableID) {
	movementLocked_ = true;
	lockedTableID_ = tableID;

	ClearQueuedAction();

	hasMoveTarget = false;
	moveMode_ = MoveMode::None;

	if (GameObject* p = GetOwner(scene)) {
		scene.GetMovementManager().ClearMoveTarget(p->GetID());
		if (ShouldPlayChopAnimation(scene)) {
			EnsureChopAnimation(scene, p);
		}
	}
}

/**
 * @brief Ends station lock.
 * @return Result produced by this operation.
 */
void PlayerLogic::EndStationLock() {
	movementLocked_ = false;
	lockedTableID_ = -1;
}

/**
 * @brief Updates station lock.
 * @param scene Scene being processed.
 * @return Result produced by this operation.
 */
void PlayerLogic::UpdateStationLock(Scene& scene) {
	if (!movementLocked_)
		return;

	// If the table vanished, unlock
	if (lockedTableID_ < 0) {
		EndStationLock();
		return;
	}

	// Only remain locked while the cutting board is actively processing
	WorkTableLogic* wt = scene.GetLogicManager().GetLogicForObject<WorkTableLogic>(lockedTableID_);
	if (!wt || !wt->LocksPlayerMovementWhileProcessing() || !wt->IsProcessing()) {
		EndStationLock();
	}
}

/**
 * @brief Returns whether play chop animation.
 * @param scene Scene being processed.
 * @return True when the operation succeeds or the condition is met.
 */
bool PlayerLogic::ShouldPlayChopAnimation(Scene& scene) const {
	if (lockedTableID_ < 0) {
		return false;
	}

	WorkTableLogic* wt = scene.GetLogicManager().GetLogicForObject<WorkTableLogic>(lockedTableID_);
	if (!wt) {
		return false;
	}

	return wt->LocksPlayerMovementWhileProcessing() && wt->IsProcessing();
}

/**
 * @brief Performs ensure chop animation.
 * @param scene Scene being processed.
 * @param player Parameter for player.
 * @return Result produced by this operation.
 */
void PlayerLogic::EnsureChopAnimation(Scene& scene, GameObject* player) {
	if (!player) {
		return;
	}

	const std::string currentAnimation = scene.GetCurrentAnimationName(player->GetID());
	if (currentAnimation != "CHOP") {
		scene.SetAnimation(player->GetID(), "CHOP");
	}
}

/**
 * @brief Returns carry offset for facing.
 * @return Requested value.
 */
glm::vec2 PlayerLogic::GetCarryOffsetForFacing() const {
	switch (facingDir) {
	case FacingDir::Front: return carryOffsetFront_;
	case FacingDir::Back:  return carryOffsetBack_;
	case FacingDir::Left:  return carryOffsetLeft_;
	case FacingDir::Right: return carryOffsetRight_;
	default:               return carryOffset;
	}
}

/**
 * @brief Applies carry layer.
 * @param scene Scene being processed.
 * @param itemID Parameter for item id.
 * @return Result produced by this operation.
 */
void PlayerLogic::ApplyCarryLayer(Scene& scene, int itemID) {
	if (!hasCarriedItemOriginalLayer_) {
		return;
	}

	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	const std::string playerLayer = scene.GetObjectLayer(player->GetID());
	const std::string desiredLayer = GetCarryLayerForFacing(playerLayer);
	if (desiredLayer.empty()) {
		return;
	}

	const std::string currentLayer = scene.GetObjectLayer(itemID);
	if (currentLayer != desiredLayer) {
		scene.AssignObjectToLayer(itemID, desiredLayer);
	}
}

/**
 * @brief Restores carried item layer.
 * @param scene Scene being processed.
 * @param itemID Parameter for item id.
 * @return Result produced by this operation.
 */
void PlayerLogic::RestoreCarriedItemLayer(Scene& scene, int itemID) {
	if (!hasCarriedItemOriginalLayer_) {
		return;
	}

	if (scene.GetGameObjectByID(itemID)) {
		scene.AssignObjectToLayer(itemID, carriedItemOriginalLayer_);
		if (GameObject* item = scene.GetGameObjectByID(itemID)) {
			item->SetRenderSortOrder(0);
		}
	}

	// If this is a plate with an ingredient visually attached, restore the ingredient's layer and order too.
	if (auto* plate = scene.GetLogicManager().GetLogicForObject<PlateLogic>(itemID)) {
		const int child = plate->GetFirstIngredientObjectID();
		if (child >= 0) {
			const std::string childLayer = GetChildLayerAbove(carriedItemOriginalLayer_);
			if (!childLayer.empty()) {
				scene.AssignObjectToLayer(child, childLayer);
			}
			if (GameObject* ingObj = scene.GetGameObjectByID(child)) {
				ingObj->SetRenderSortOrder(1);
			}
		}
	}

	carriedItemOriginalLayer_.clear();
	hasCarriedItemOriginalLayer_ = false;
}

namespace {

	/**
	 * @brief Attempts to parse layer number.
	 * @param layer Parameter for layer.
	 * @param outValue Output value for out value.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool TryParseLayerNumber(const std::string& layer, int& outValue) {
		if (layer.empty()) {
			return false;
		}

		int value = 0;
		for (char c : layer) {
			if (!std::isdigit(static_cast<unsigned char>(c))) {
				return false;
			}
			value = value * 10 + (c - '0');
		}

		outValue = value;
		return true;
	}
}

/**
 * @brief Returns carry layer for facing.
 * @return Requested value.
 */
std::string PlayerLogic::GetCarryLayerForFacing(const std::string& /*baseLayer*/) const {
	switch (facingDir) {
	case FacingDir::Front: return "6";
	case FacingDir::Back:  return "6";
	case FacingDir::Left:  return "6";
	case FacingDir::Right: return "6";
	default:               return "6";
	}
}

/**
 * @brief Returns carry child layer for facing.
 * @return Requested value.
 */
std::string PlayerLogic::GetCarryChildLayerForFacing(const std::string& /*baseLayer*/) const {
	switch (facingDir) {
	case FacingDir::Front: return "6";
	case FacingDir::Back:  return "6";
	case FacingDir::Left:  return "6";
	case FacingDir::Right: return "6";
	default:               return "6";
	}
}

/**
 * @brief Returns child layer above.
 * @param baseLayer Parameter for base layer.
 * @return Requested value.
 */
std::string PlayerLogic::GetChildLayerAbove(const std::string& baseLayer) const {
	int value = 0;
	if (!TryParseLayerNumber(baseLayer, value)) {
		return baseLayer;
	}

	return std::to_string(value + 1);
}

namespace {
	constexpr float kHoverOutlineOffset = 2.5f;

	// Draw above nearby interactables on the same layer
	constexpr int kHoverOutlineSortOrder = 200;
	constexpr int kHoverOutlineMaskSortOrder = 201;

	/**
	 * @brief Performs k hover outline tint.
	 * @param f Parameter for f.
	 * @param f Parameter for f.
	 * @param f Parameter for f.
	 * @param f Parameter for f.
	 * @return Result produced by this operation.
	 */
	const glm::vec4 kHoverOutlineTint(0.0f, 1.0f, 1.0f, 0.92f);
}

/**
 * @brief Performs ensure hover outline.
 * @param scene Scene being processed.
 * @param sourceObj Parameter for source obj.
 * @param sourceID Parameter for source id.
 * @return Result produced by this operation.
 */
void PlayerLogic::EnsureHoverOutline(Scene& scene, GameObject* sourceObj, int sourceID) {
	if (!sourceObj || sourceID < 0) {
		return;
	}

	const std::string texturePath = scene.GetObjectTexturePath(sourceID);
	if (texturePath.empty()) {
		return;
	}

	// Put outline on same layer as source, so table contents/VFX can appear over it.
	const std::string outlineLayer = scene.GetObjectLayer(sourceID);

	auto it = hoverOutlineIDs_.find(sourceID);
	if (it == hoverOutlineIDs_.end()) {
		std::array<int, 5> outlineIDs{ -1, -1, -1, -1, -1 };
		const glm::vec3 srcPos = sourceObj->GetPositionGLM();
		const glm::vec3 srcScale = sourceObj->GetScaleGLM();

		const std::array<glm::vec2, 4> offsets{
			glm::vec2(-kHoverOutlineOffset, 0.0f),
			glm::vec2(kHoverOutlineOffset, 0.0f),
			glm::vec2(0.0f, -kHoverOutlineOffset),
			glm::vec2(0.0f,  kHoverOutlineOffset)
		};

		// Cyan edge copies
		for (std::size_t i = 0; i < offsets.size(); ++i) {
			const glm::vec2& offset = offsets[i];
			const glm::vec3 outlinePos(srcPos.x + offset.x, srcPos.y + offset.y, srcPos.z);

			GameObject* outline = scene.SpawnStaticSprite(
				texturePath,
				outlinePos,
				glm::vec2(std::abs(srcScale.x), std::abs(srcScale.y)),
				outlineLayer
			);

			if (!outline) {
				continue;
			}

			outline->SetColorTint(kHoverOutlineTint);
			outline->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
			outline->SetMovableByPhysics(false);
			outline->EnableShadow(false);
			outline->SetRenderSortOrder(kHoverOutlineSortOrder);
			outline->SetRotation(sourceObj->GetRotation(), glm::vec3(0.0f, 0.0f, 1.0f));

			if (Shader* outlineShader = ResourceManager::Instance().GetShader("hover_outline")) {
				outline->SetShader(outlineShader);
			}

			outlineIDs[i] = outline->GetID();
		}

		// Center mask copy (restores interior, leaving only cyan border visible)
		GameObject* mask = scene.SpawnStaticSprite(
			texturePath,
			srcPos,
			glm::vec2(std::abs(srcScale.x), std::abs(srcScale.y)),
			outlineLayer
		);

		if (mask) {
			mask->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			mask->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
			mask->SetMovableByPhysics(false);
			mask->EnableShadow(false);
			mask->SetRenderSortOrder(kHoverOutlineMaskSortOrder);
			mask->SetRotation(sourceObj->GetRotation(), glm::vec3(0.0f, 0.0f, 1.0f));
			outlineIDs[4] = mask->GetID();
		}

		hoverOutlineIDs_[sourceID] = outlineIDs;
		it = hoverOutlineIDs_.find(sourceID);
	}

	// Keep overlay synced with source transform.
	const glm::vec3 srcPos = sourceObj->GetPositionGLM();
	const glm::vec3 srcScale = sourceObj->GetScaleGLM();
	const float srcRot = sourceObj->GetRotation();

	const std::array<glm::vec2, 4> offsets{
		glm::vec2(-kHoverOutlineOffset, 0.0f),
		glm::vec2(kHoverOutlineOffset, 0.0f),
		glm::vec2(0.0f, -kHoverOutlineOffset),
		glm::vec2(0.0f,  kHoverOutlineOffset)
	};

	for (std::size_t i = 0; i < 4; ++i) {
		const int id = it->second[i];
		if (id < 0) {
			continue;
		}

		GameObject* outline = scene.GetGameObjectByID(id);
		if (!outline) {
			continue;
		}

		const glm::vec2& offset = offsets[i];
		outline->SetPosition(glm::vec3(srcPos.x + offset.x, srcPos.y + offset.y, srcPos.z));
		outline->SetScale(glm::vec3(std::abs(srcScale.x), std::abs(srcScale.y), 1.0f));
		outline->SetRotation(srcRot, glm::vec3(0.0f, 0.0f, 1.0f));
		outline->SetColorTint(kHoverOutlineTint);
		outline->SetRenderSortOrder(kHoverOutlineSortOrder);
		scene.AssignObjectToLayer(id, outlineLayer);
	}

	// Sync mask
	const int maskID = it->second[4];
	if (maskID >= 0) {
		if (GameObject* mask = scene.GetGameObjectByID(maskID)) {
			mask->SetPosition(srcPos);
			mask->SetScale(glm::vec3(std::abs(srcScale.x), std::abs(srcScale.y), 1.0f));
			mask->SetRotation(srcRot, glm::vec3(0.0f, 0.0f, 1.0f));
			mask->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			mask->SetRenderSortOrder(kHoverOutlineMaskSortOrder);
			scene.AssignObjectToLayer(maskID, outlineLayer);
		}
	}
}

/**
 * @brief Removes hover outline.
 * @param scene Scene being processed.
 * @param sourceID Parameter for source id.
 * @return Result produced by this operation.
 */
void PlayerLogic::RemoveHoverOutline(Scene& scene, int sourceID) {
	auto it = hoverOutlineIDs_.find(sourceID);
	if (it == hoverOutlineIDs_.end()) {
		return;
	}

	for (int outlineID : it->second) {
		if (outlineID >= 0 && scene.GetGameObjectByID(outlineID)) {
			scene.DespawnByID(outlineID);
		}
	}

	hoverOutlineIDs_.erase(it);
}

/**
 * @brief Clears hover outlines.
 * @param scene Scene being processed.
 * @return Result produced by this operation.
 */
void PlayerLogic::ClearHoverOutlines(Scene& scene) {
	for (auto& [sourceID, outlineIDs] : hoverOutlineIDs_) {
		(void)sourceID;
		for (int outlineID : outlineIDs) {
			if (outlineID >= 0 && scene.GetGameObjectByID(outlineID)) {
				scene.DespawnByID(outlineID);
			}
		}
	}

	hoverOutlineIDs_.clear();
}

