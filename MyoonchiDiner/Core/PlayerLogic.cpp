/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (50%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu   (30%)
					Seah Wang Hua, wanghua.seah@digipen.edu (20%)

 DESCRIPTION:		Implements the high-level PlayerLogic lifecycle entry points.
					- Resets per-scene runtime player state on start
					- Handles the top-level per-frame gameplay update flow
					- Coordinates pause-state cleanup and input suppression
					- Delegates detailed movement, interaction, hover, and carry work

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "Core/InputManager.hpp"
#include "Core/Quota.hpp"

#include "PlayerLogicShared.hpp"

/**
 * @brief Initializes the runtime state used by the split PlayerLogic implementation.
 * @param scene Active scene containing the player object and supporting systems.
 */
void PlayerLogic::Start(Scene& scene) {
	// Reset all runtime state so re-entering a scene starts from a clean baseline.
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
	const bool forceClearShortcut =
		((isLevel1 || isLevel2) && input.IsKeyJustPressed(GLFW_KEY_F10));

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

	UpdateInteractableVisualCues(scene, input, safeDt);
	UpdateClickMoveIndicator(scene, safeDt);

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
		// In step mode, keep click handling responsive even if simulation is paused between physics ticks.
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

