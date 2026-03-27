/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogicMovement.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements PlayerLogic movement, locomotion, and station-lock behavior.
					- Updates direct movement and path-following navigation
					- Chooses locomotion and idle animations from movement direction
					- Handles keyboard movement overrides and arrival callbacks
					- Manages footstep VFX/SFX and workstation movement locks

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/DebugUI.hpp"
#include "EngineCore/EngineRng.hpp"
#include "EngineCore/InputManager.hpp"
#include "EngineCore/Logger.hpp"
#include "GameCore/PlayerLogicShared.hpp"
#include "GameCore/WorkTableLogic.hpp"

 /**
  * @brief Resets the transient state used for click-and-drag retargeting.
  */
void PlayerLogic::ResetMouseDragState() {
	// Drop any drag-specific history so the next press starts a fresh retarget gesture.
	mouseDragActive_ = false;
	hasLastDragWorld_ = false;
	dragRetargetTimer_ = 0.0f;
}

/**
 * @brief Clears the active movement target without changing queued interactions.
 * @param scene Active scene used to clear movement-manager state.
 */
void PlayerLogic::ClearMovementTarget(Scene& scene) {
	// Clear the current destination without touching queued interaction state.
	hasMoveTarget = false;
	blockedMoveFrames_ = 0;

	if (GameObject* player = GetOwner(scene)) {
		scene.GetMovementManager().ClearMoveTarget(player->GetID());
	}
}

/**
 * @brief Resets navigation state after arrival, cancellation, or path failure.
 * @param scene Active scene used to clear movement-manager state.
 * @param clearPendingTable True to discard any pending table interaction target.
 */
void PlayerLogic::FinishMovement(Scene& scene, bool clearPendingTable) {
	// Fully reset navigation state after arrival, cancellation, or a failed path update.
	hasMoveTarget = false;
	moveMode_ = MoveMode::None;
	pathPoints_.clear();
	pathIndex_ = 0;

	if (clearPendingTable) {
		pendingTableID = -1;
	}

	if (GameObject* player = GetOwner(scene)) {
		scene.GetMovementManager().ClearMoveTarget(player->GetID());
	}
}

/**
 * @brief Skips over waypoints that are already within the arrival radius.
 * @param currentPos Current player world position.
 * @param arriveRadiusSq Squared distance threshold used to treat a waypoint as reached.
 * @return True when all waypoints have been consumed.
 */
bool PlayerLogic::AdvancePathWaypoints(const glm::vec2& currentPos, float arriveRadiusSq) {
	// Skip over any waypoints that the player has effectively already reached this frame.
	while (pathIndex_ < pathPoints_.size()) {
		glm::vec2 toWaypoint = pathPoints_[pathIndex_] - currentPos;
		float distSq = toWaypoint.x * toWaypoint.x + toWaypoint.y * toWaypoint.y;
		if (distSq > arriveRadiusSq) {
			break;
		}

		++pathIndex_;
	}

	return pathIndex_ >= pathPoints_.size();
}

/**
 * @brief Attempts to rebuild a path from the current player position to the final target.
 * @param scene Active scene used for path queries.
 * @param player Owning player object.
 * @param currentPos Current player world position.
 * @param arriveRadiusSq Squared threshold used for waypoint skipping.
 * @param switchToPathMode True to force the move mode back to pathfinding.
 * @return True when a replacement path was found.
 */
bool PlayerLogic::TryRepathToFinalTarget(Scene& scene, GameObject* player, const glm::vec2& currentPos, float arriveRadiusSq, bool switchToPathMode) {
	// Rebuild the remaining route from the current position when a direct or path move gets blocked.
	std::vector<glm::vec2> newPath;
	if (!scene.FindPathForObject(player->GetID(), currentPos, finalTarget_, newPath)) {
		return false;
	}

	pathPoints_ = std::move(newPath);
	pathIndex_ = 0;
	if (switchToPathMode) {
		// Direct movement promotes itself back to waypoint mode after a successful repath.
		moveMode_ = MoveMode::Pathfinding;
	}

	while (!pathPoints_.empty()) {
		glm::vec2 d = pathPoints_.front() - currentPos;
		if ((d.x * d.x + d.y * d.y) > arriveRadiusSq) {
			break;
		}

		pathPoints_.erase(pathPoints_.begin());
	}

	hasMoveTarget = !pathPoints_.empty();
	if (hasMoveTarget) {
		// Prime the next waypoint so the regular movement update can continue immediately.
		moveTarget = pathPoints_.front();
	}

	return hasMoveTarget;
}

/**
 * @brief Finalizes a direct move when the player is close enough to the final target.
 * @param scene Active scene being processed.
 * @param player Owning player object.
 * @param currentPos Current player world position.
 * @param arriveRadiusSq Squared distance threshold used for arrival.
 * @return True when the direct move has completed.
 */
bool PlayerLogic::TryFinishDirectMovement(Scene& scene, GameObject* player, const glm::vec2& currentPos, float arriveRadiusSq) {
	// Treat the final target as the active move target so arrival and sprite logic stay consistent.
	(void)player;
	moveTarget = finalTarget_;
	glm::vec2 dir = moveTarget - currentPos;
	const float distSq = dir.x * dir.x + dir.y * dir.y;
	if (distSq > arriveRadiusSq) {
		return false;
	}

	FinishMovement(scene, false);
	OnArrived(scene);
	return true;
}

/**
 * @brief Updates one frame of direct movement toward the current target.
 * @param dt Frame delta time in seconds.
 * @param scene Active scene being processed.
 * @param player Owning player object.
 * @param playerPos3 Current player position in 3D space.
 * @param currentPos Current player position in 2D space.
 * @param arriveRadiusSq Squared distance threshold used for arrival.
 * @return True when this function handled the movement update.
 */
bool PlayerLogic::TryUpdateDirectMovement(float dt, Scene& scene, GameObject* player, const glm::vec3& playerPos3, const glm::vec2& currentPos, float arriveRadiusSq) {
	// Prefer a cheap direct move until we prove the route is blocked or complete.
	if (TryFinishDirectMovement(scene, player, currentPos, arriveRadiusSq)) {
		return true;
	}

	glm::vec2 dir = moveTarget - currentPos;
	float distSq = dir.x * dir.x + dir.y * dir.y;
	float dist = std::sqrt(distSq);
	if (dist > 0.0001f) {
		dir /= dist;
	}

	float step = std::min(moveSpeed * dt, dist);
	glm::vec2 desiredDelta(dir.x * step, dir.y * step);
	glm::vec2 allowedDelta = scene.ResolveWorldStep(player, desiredDelta);
	const float allowedLenSq = allowedDelta.x * allowedDelta.x + allowedDelta.y * allowedDelta.y;

	if (allowedLenSq < 0.0001f) {
		// When the step is fully blocked, fall back to pathfinding before giving up.
		if (TryRepathToFinalTarget(scene, player, currentPos, arriveRadiusSq, true)) {
			return true;
		}

		FinishMovement(scene, true);
		return true;
	}

	player->SetPosition(glm::vec3(currentPos.x + allowedDelta.x, currentPos.y + allowedDelta.y, playerPos3.z));
	scene.ClampToWalkArea(player);
	UpdateSprite(scene, player, allowedDelta);
	return true;
}

/**
 * @brief Updates one frame of waypoint-based path movement.
 * @param dt Frame delta time in seconds.
 * @param scene Active scene being processed.
 * @param player Owning player object.
 * @param playerPos3 Current player position in 3D space.
 * @param currentPos Current player position in 2D space.
 * @param arriveRadiusSq Squared distance threshold used for waypoint arrival.
 * @return True when this function handled the movement update.
 */
bool PlayerLogic::TryUpdatePathMovement(float dt, Scene& scene, GameObject* player, const glm::vec3& playerPos3, const glm::vec2& currentPos, float arriveRadiusSq) {
	// Move toward the current waypoint and keep the debug path visualization in sync.
	moveTarget = pathPoints_[pathIndex_];
	glm::vec2 dir = moveTarget - currentPos;
	float distSq = dir.x * dir.x + dir.y * dir.y;
	float dist = std::sqrt(distSq);
	if (dist > 0.0001f) {
		dir /= dist;
	}

	float step = std::min(moveSpeed * dt, dist);
	glm::vec2 desiredDelta(dir.x * step, dir.y * step);
	glm::vec2 allowedDelta = scene.ResolveWorldStep(player, desiredDelta);
	const float allowedLenSq = allowedDelta.x * allowedDelta.x + allowedDelta.y * allowedDelta.y;

	if (allowedLenSq < 0.0001f) {
		// Try to recover from stale nav data before cancelling the move outright.
		if (TryRepathToFinalTarget(scene, player, currentPos, arriveRadiusSq, false)) {
			return true;
		}

		TS_LOG_DEBUG("[PlayerLogic] Path blocked and repath failed.");
		FinishMovement(scene, true);
		return true;
	}

	const glm::vec2 nextPos(currentPos.x + allowedDelta.x, currentPos.y + allowedDelta.y);
	player->SetPosition(glm::vec3(nextPos.x, nextPos.y, playerPos3.z));
	scene.ClampToWalkArea(player);
	UpdateSprite(scene, player, allowedDelta);

	if (DebugRenderer::IsEnabled() && hasMoveTarget) {
		// Visualize the remaining route from the player's current point to the final waypoint.
		glm::vec2 prev = nextPos;
		for (std::size_t i = pathIndex_; i < pathPoints_.size(); ++i) {
			const glm::vec2 next = pathPoints_[i];
			DebugRenderer::DrawLine(
				glm::vec3(prev.x, prev.y, playerPos3.z),
				glm::vec3(next.x, next.y, playerPos3.z),
				glm::vec3(0.0f, 1.0f, 0.0f));
			prev = next;
		}
	}

	return true;
}

/**
 * @brief Updates the player's locomotion animation based on movement direction and carry state.
 * @param scene Active scene used for animation queries.
 * @param player Owning player object.
 * @param moveDirRaw Raw movement vector for the current frame.
 */
void PlayerLogic::UpdateSprite(Scene& scene, GameObject* player, const glm::vec2& moveDirRaw) {
	// Pick the best locomotion or idle animation from the latest movement direction and carry state.
	(void)scene;
	if (!player) {
		return;
	}

	const float moveThreshold = 0.01f;
	std::string desiredAnimation;
	const bool isHolding = carriedItemID >= 0;

	float absX = std::abs(moveDirRaw.x);
	float absY = std::abs(moveDirRaw.y);

	if (glm::length(moveDirRaw) < moveThreshold) {
		switch (facingDir) {
		case FacingDir::Right: desiredAnimation = isHolding ? "IDLE_RIGHT_CARRY" : "IDLE_RIGHT"; break;
		case FacingDir::Left:  desiredAnimation = isHolding ? "IDLE_LEFT_CARRY" : "IDLE_LEFT";  break;
		case FacingDir::Front: desiredAnimation = isHolding ? "IDLE_FRONT_CARRY" : "IDLE_FRONT"; break;
		case FacingDir::Back:  desiredAnimation = isHolding ? "IDLE_BACK_CARRY" : "IDLE_BACK";  break;
		}
	}
	else if (absX > absY) {
		if (moveDirRaw.x > 0.0f) {
			desiredAnimation = isHolding ? "CARRY_RIGHT" : "WALK_RIGHT";
			facingDir = FacingDir::Right;
		}
		else {
			desiredAnimation = isHolding ? "CARRY_LEFT" : "WALK_LEFT";
			facingDir = FacingDir::Left;
		}
	}
	else if (moveDirRaw.y > 0.0f) {
		desiredAnimation = isHolding ? "CARRY_FRONT" : "WALK_FRONT";
		facingDir = FacingDir::Front;
	}
	else {
		desiredAnimation = isHolding ? "CARRY_BACK" : "WALK_BACK";
		facingDir = FacingDir::Back;
	}

	std::string currentAnimation = scene.GetCurrentAnimationName(player->GetID());
	if (desiredAnimation != currentAnimation) {
		// Avoid redundant animation changes so transition timing stays stable.
		scene.SetAnimation(player->GetID(), desiredAnimation);
	}
}

/**
 * @brief Starts direct movement toward a world-space target.
 * @param dest World-space target position.
 */
void PlayerLogic::MoveDirect(const glm::vec2& dest) {
	// Direct moves are used when we can head straight for the goal without table-specific setup.
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
 * @brief Starts path-based movement toward a snapped navigation destination.
 * @param scene Active scene used for pathfinding and snapping.
 * @param dest Requested world-space destination.
 */
void PlayerLogic::MoveTo(Scene& scene, const glm::vec2& dest) {
	// Snap click targets onto the navigation grid so pathfinding uses consistent cell centers.
	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	glm::vec2 snappedDest = dest;
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
		// Drop the pending table intent when no valid route exists to the requested target.
		TS_LOG_DEBUG("[PlayerLogic] No path found.");
		pendingTableID = -1;
		moveMode_ = MoveMode::None;
		return;
	}

	while (!pathPoints_.empty()) {
		glm::vec2 d = pathPoints_.front() - startPos;
		const float kSkipWaypointRadius = 18.0f;
		if ((d.x * d.x + d.y * d.y) > kSkipWaypointRadius * kSkipWaypointRadius) {
			break;
		}

		pathPoints_.erase(pathPoints_.begin());
	}

	if (pathPoints_.empty()) {
		// Interactions can fire immediately when the snapped destination is already effectively reached.
		moveMode_ = MoveMode::None;
		OnArrived(scene);
		return;
	}

	hasMoveTarget = true;
	moveTarget = pathPoints_[0];
}

/**
 * @brief Cancels the current table-bound move and clears its pending interaction.
 * @param scene Active scene used to clear movement-manager state.
 */
void PlayerLogic::CancelQueuedTableMove(Scene& scene) {
	// Cancel both navigation and the deferred interaction that depended on it.
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
 * @brief Updates player navigation and arrival handling for the current frame.
 * @param dt Frame delta time in seconds.
 * @param scene Active scene containing navigation data.
 */
void PlayerLogic::UpdateMovement(float dt, Scene& scene) {
	// Station-locked actions own the player pose, so skip all navigation while they are active.
	if (movementLocked_ || !hasMoveTarget) {
		return;
	}

	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	glm::vec3 pos3 = player->GetPositionGLM();
	glm::vec2 pos(pos3.x, pos3.y);

	if (moveMode_ == MoveMode::Pathfinding) {
		// Periodically test whether a straight shot to the goal has opened up.
		directPathCheckTimer_ -= dt;

		if (directPathCheckTimer_ <= 0.0f) {
			directPathCheckTimer_ = kDirectPathCheckInterval;

			if (scene.HasDirectPathForObject(player->GetID(), pos, finalTarget_)) {
				// Promote to direct mode to simplify the remaining movement once the line is clear.
				moveMode_ = MoveMode::Direct;
				moveTarget = finalTarget_;
				pathPoints_.clear();
				pathIndex_ = 0;
				hasMoveTarget = true;
			}
		}
	}

	constexpr float kMoveArriveRadius = 6.0f;
	const float arriveRadiusSq = kMoveArriveRadius * kMoveArriveRadius;

	if (moveMode_ == MoveMode::Direct) {
		(void)TryUpdateDirectMovement(dt, scene, player, pos3, pos, arriveRadiusSq);
		return;
	}

	if (pathPoints_.empty()) {
		// A path move with no waypoints left is effectively complete.
		FinishMovement(scene, false);
		return;
	}

	if (AdvancePathWaypoints(pos, arriveRadiusSq)) {
		FinishMovement(scene, false);
		return;
	}

	(void)TryUpdatePathMovement(dt, scene, player, pos3, pos, arriveRadiusSq);
}

/**
 * @brief Runs arrival logic after the player reaches its pending interaction destination.
 * @param scene Active scene being processed.
 */
void PlayerLogic::OnArrived(Scene& scene) {
	// Only arrival callbacks tied to a pending table interaction need follow-up work here.
	if (pendingTableID < 0) {
		return;
	}

	const int arrivedTableID = pendingTableID;
	pendingTableID = -1;

	if (IsInTableInteractionRange(scene, arrivedTableID)) {
		// Process the table immediately, then resume any click the player queued during the lock.
		InteractWithTable(scene, arrivedTableID);

		if (!movementLocked_) {
			ExecuteQueuedAction(scene);
		}
	}
}

/**
 * @brief Applies direct keyboard movement and overrides click-to-move when necessary.
 * @param dt Frame delta time in seconds.
 * @param scene Active scene being processed.
 * @param input Centralized input snapshot for the current frame.
 * @param player Owning player object.
 * @param playerPos Current player position before movement.
 */
void PlayerLogic::HandleKeyboardMovement(float dt, Scene& scene, InputManager& input, GameObject* player, const glm::vec3& playerPos) {
	// Manual input overrides click-to-move so keyboard control always feels authoritative.
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
		// Cancel queued point-and-click state before applying direct keyboard displacement.
		hasMoveTarget = false;
		moveMode_ = MoveMode::None;
		pathPoints_.clear();
		pathIndex_ = 0;
		pendingTableID = -1;
		ClearQueuedAction();

		const glm::vec2 normalizedInput = PlayerLogicDetail::NormalizeOrZero(inputDir);
		glm::vec2 desiredDelta(normalizedInput.x * moveSpeed * dt, normalizedInput.y * moveSpeed * dt);
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
		// Keep facing aligned with the active destination even if movement happens elsewhere in the frame.
		UpdateSprite(scene, player, moveTarget - PlayerLogicDetail::ToVec2(player->GetPositionGLM()));
	}
	else {
		UpdateSprite(scene, player, glm::vec2(0.0f, 0.0f));
	}
}

/**
 * @brief Updates footstep particles and audio based on actual player movement.
 * @param dt Frame delta time in seconds.
 * @param scene Active scene used for particle and audio playback.
 * @param input Centralized input snapshot for the current frame.
 * @param player Owning player object.
 * @param beforePos Player position before movement.
 * @param afterPos Player position after movement.
 */
void PlayerLogic::UpdateFootstepTrailAndAudio(float dt, Scene& scene, InputManager& input, GameObject* player, const glm::vec3& beforePos, const glm::vec3& afterPos) {
	// Emit footsteps and trail particles only when the player both intends to move and actually moved.
	(void)dt;
	const bool hasIntent =
		input.IsKeyPressed(GLFW_KEY_A) || input.IsKeyPressed(GLFW_KEY_D) ||
		input.IsKeyPressed(GLFW_KEY_W) || input.IsKeyPressed(GLFW_KEY_S) ||
		hasMoveTarget;

	const glm::vec2 moveDelta(afterPos.x - beforePos.x, afterPos.y - beforePos.y);
	const bool actuallyMoved = std::sqrt(moveDelta.x * moveDelta.x + moveDelta.y * moveDelta.y) > PlayerLogicDetail::kTrailJitterEpsilon;
	if (!hasIntent || !actuallyMoved) {
		// Reset trail accumulation whenever movement stops so the next burst starts cleanly.
		hasLastTrailPos_ = false;
		trailCarry_ = 0.0f;
		footstepEmitTimer_ = 0.0f;
		return;
	}

#ifndef _DEBUG
	footstepEmitTimer_ += dt;
	if (footstepEmitTimer_ >= PlayerLogicDetail::kFootstepInterval) {
		// Rate-limit footstep SFX so long moves do not spam overlapping sounds.
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

	const glm::vec2 dir = PlayerLogicDetail::NormalizeOrZero(moveDelta);
	if (!hasLastTrailPos_) {
		// Seed the trail segment start on the first movement frame after becoming active.
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
			// Scatter small trail puffs along the traveled segment with slight perpendicular jitter.
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
 * @brief Locks the player in place while a workstation owns the current interaction.
 * @param scene Active scene being processed.
 * @param tableID Object ID of the workstation requesting the lock.
 */
void PlayerLogic::BeginStationLock(Scene& scene, int tableID) {
	// Freeze player movement while a workstation owns the interaction animation.
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
 * @brief Releases the current workstation movement lock.
 */
void PlayerLogic::EndStationLock() {
	// Release the processing lock so deferred clicks can resume on the next update.
	movementLocked_ = false;
	lockedTableID_ = -1;
}

/**
 * @brief Refreshes the workstation lock state and releases it when processing ends.
 * @param scene Active scene being processed.
 */
void PlayerLogic::UpdateStationLock(Scene& scene) {
	// Unlock automatically once the associated workstation stops processing.
	if (!movementLocked_) {
		return;
	}

	if (lockedTableID_ < 0) {
		EndStationLock();
		return;
	}

	WorkTableLogic* wt = scene.GetLogicManager().GetLogicForObject<WorkTableLogic>(lockedTableID_);
	if (!wt || !wt->LocksPlayerMovementWhileProcessing() || !wt->IsProcessing()) {
		EndStationLock();
	}
}

/**
 * @brief Returns whether the chop animation should be forced this frame.
 * @param scene Active scene being processed.
 * @return True when the locked workstation is actively processing.
 */
bool PlayerLogic::ShouldPlayChopAnimation(Scene& scene) const {
	// The chop loop should only play while the locked workstation is actively processing.
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
 * @brief Ensures the player's current animation is the chop loop.
 * @param scene Active scene used for animation queries.
 * @param player Owning player object.
 */
void PlayerLogic::EnsureChopAnimation(Scene& scene, GameObject* player) {
	// Avoid restarting the same animation every frame while the player remains locked.
	if (!player) {
		return;
	}

	const std::string currentAnimation = scene.GetCurrentAnimationName(player->GetID());
	if (currentAnimation != "CHOP") {
		scene.SetAnimation(player->GetID(), "CHOP");
	}
}
