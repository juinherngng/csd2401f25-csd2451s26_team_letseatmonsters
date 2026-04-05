/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (35%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu   (65%)

 DESCRIPTION:		Implements the high-level PlayerLogic lifecycle entry points.
					- Resets per-scene runtime player state on start
					- Handles the top-level per-frame gameplay update flow
					- Coordinates pause-state cleanup and input suppression
					- Delegates detailed movement, interaction, hover, and carry work

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineCore/InputManager.hpp"
#include "GameCore/PlayerLogicShared.hpp"
#include "GameCore/Quota.hpp"

namespace {
	constexpr float kBackCarryPlayerAlphaMultiplier = 0.55f;

	bool UsesBackCarryTransparency(const std::string& animationName) {
		return animationName == "CARRY_BACK" || animationName == "IDLE_BACK_CARRY";
	}
}

/**
 * @brief Updates the player's alpha while using the back-facing carry animation.
 * @param scene Active scene used to inspect the player's current animation.
 */
void PlayerLogic::UpdateCarryBackTransparency(Scene& scene) {
	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	const std::string animationName = scene.GetCurrentAnimationName(player->GetID());
	const bool shouldFadePlayer =
		carriedItemID >= 0 &&
		UsesBackCarryTransparency(animationName);

	const float targetAlpha = shouldFadePlayer
		? playerBaseAlpha_ * kBackCarryPlayerAlphaMultiplier
		: playerBaseAlpha_;

	glm::vec4 tint = player->GetColorTint();
	if (tint.a != targetAlpha) {
		tint.a = targetAlpha;
		player->SetColorTint(tint);
	}
}

 /**
  * @brief Initializes the runtime state used by the split PlayerLogic implementation.
  * @param scene Active scene containing the player object and supporting systems.
  */
void PlayerLogic::Start(Scene& scene) {
	// Reset all runtime state so re-entering a scene starts from a clean baseline.
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
	// Reset lock, queued-action, and movement recovery state so no stale scene state survives reloads.
	movementLocked_ = false;
	lockedTableID_ = -1;
	queuedAction_ = {};
	clickIndicatorID_ = -1;
	clickIndicatorTimeLeft_ = 0.0f;
	suppressMouseUntilRelease_ = false;
	hasLastTrailPos_ = false;
	trailCarry_ = 0.0f;
	footstepDistanceAcc_ = 0.0f;
	wasMoving_ = false;
	footstepEmitTimer_ = 0.0f;

	pathPoints_.clear();
	pathIndex_ = 0;
	finalTarget_ = glm::vec2(0.0f, 0.0f);
	directPathCheckTimer_ = 0.0f;
	blockedMoveFrames_ = 0;

	if (GameObject* player = GetOwner(scene)) {
		playerBaseAlpha_ = player->GetColorTint().a;
		glm::vec4 tint = player->GetColorTint();
		tint.a = playerBaseAlpha_;
		player->SetColorTint(tint);
	}
}

/**
 * @brief Clears transient gameplay state when the scene enters a paused or blocked state.
 * @param scene Active scene used to remove indicators and queued actions.
 */
void PlayerLogic::EnterPauseState(Scene& scene) {
	// Clear transient interaction state so gameplay does not resume mid-click or mid-highlight.
	ClearInteractableVisualCues(scene);
	ClearClickMoveIndicator(scene);
	CancelQueuedTableMove(scene);
	ClearQueuedAction();
	ResetMouseDragState();
	if (GameObject* player = GetOwner(scene)) {
		// Zero the immediate velocity as well so physics-driven frames do not drift under pause.
		player->SetVelocity(Math::Vector2D(0.0f, 0.0f));
	}
	suppressMouseUntilRelease_ = true;
}

/**
 * @brief Executes the top-level per-frame PlayerLogic update flow.
 * @param dt Frame delta time in seconds.
 * @param scene Active scene containing the player and interactables.
 * @param input Centralized input snapshot for the current frame.
 */
void PlayerLogic::Update(float dt, Scene& scene, InputManager& input) {
	// Clamp large frame spikes so pathing and drag retarget logic remain stable after hitches.
	const float safeDt = std::clamp(dt, 0.0f, PlayerLogicDetail::kMaxPlayerUpdateDt);

	if (!scene.IsSimulationActive() || scene.IsPauseOverlayActive()) {
		// Pause immediately when simulation is suspended so queued actions do not leak across states.
		EnterPauseState(scene);
		return;
	}

	if (suppressMouseUntilRelease_) {
		// Wait for the mouse button to be released before accepting new clicks after pause/lock transitions.
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
	const bool forceLoseShortcut =
		((isLevel1 || isLevel2) && input.IsKeyJustPressed(GLFW_KEY_F9));
	const bool forceClearShortcut =
		((isLevel1 || isLevel2) && input.IsKeyJustPressed(GLFW_KEY_F10));

	if (forceLoseShortcut && !Economy::gTimerPaused) {
		// Mirror the instant-win debug shortcut with a direct fail-state trigger.
		Economy::gTimeRemaining = 0.0f;
		Economy::gTimeUp = true;
		Economy::gAwaitingFinalCustomerClear = false;
		Economy::SyncUI(&scene);
		Economy::OnTimeUp(scene);
		return;
	}

	if (forceClearShortcut && !Economy::gQuotaReached) {
		// Preserve the existing developer shortcut for quickly forcing quota completion in kitchen levels.
		Economy::gPlayerMoney = Economy::kQuota;
		Economy::SyncUI(&scene);
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
		// Resume any deferred click after a station-locked work animation finishes.
		ExecuteQueuedAction(scene);
	}

	// Refresh cursor-driven feedback before processing the current frame's movement and interaction.
	UpdateInteractableVisualCues(scene, input, safeDt);
	UpdateClickMoveIndicator(scene, safeDt);

	if (movementLocked_ && ShouldPlayChopAnimation(scene)) {
		// Locked workstations suppress locomotion while still allowing animation and carry visuals to update.
		ResetMouseDragState();
		EnsureChopAnimation(scene, player);
		UpdateCarriedItemTransform(scene);
		UpdateCarryBackTransparency(scene);
		return;
	}

	const glm::vec3 beforePos = player->GetPositionGLM();
	const float physicsDt = scene.GetLastPhysicsDt();
	const bool stepMode = scene.GetStepController().enabled;
	if (stepMode && physicsDt <= 0.0f) {
		// In step mode, keep click handling responsive even if simulation is paused between physics ticks.
		HandleClickInput(scene, input, safeDt);
		UpdateCarryBackTransparency(scene);
		return;
	}

	// Apply input, then advance movement, then derive post-move feedback from the final player position.
	HandleKeyboardMovement(safeDt, scene, input, player, beforePos);
	HandleClickInput(scene, input, safeDt);
	UpdateMovement(safeDt, scene);

	const glm::vec3 afterPos = player->GetPositionGLM();
	UpdateFootstepTrailAndAudio(safeDt, scene, input, player, beforePos, afterPos);
	UpdateCarriedItemTransform(scene);
	UpdateCarryBackTransparency(scene);
}
