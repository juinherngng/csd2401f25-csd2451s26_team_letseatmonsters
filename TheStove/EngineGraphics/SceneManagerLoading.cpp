/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManagerLoading.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements Scene lifecycle, deferred level loading, and level-reset
					helpers extracted from SceneManager.cpp.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineCore/InputManager.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineCore/MessageBus.hpp"
#include "EngineCore/MovementManager.hpp"
#include "EngineCore/RuntimeLevelPipeline.hpp"
#include "EngineGraphics/AnimationManager.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/SceneManager.hpp"

 // -------------------------------------------------------------------------------------------------
 // Scene Flow State Helpers
 // -------------------------------------------------------------------------------------------------

 /**
  * @brief Updates the externally visible scene flow state and publishes change notifications.
  * @param newState New steady or transitional flow state for the Scene.
  */
void Scene::SetFlowState(FlowState newState) {
	if (flowState_ == newState) {
		return;
	}

	// Persist the new state first so observers receive the current scene status.
	flowState_ = newState;

	if (messageBus_) {
		messageBus_->Post<CoreFramework::SceneFlowStateChangedMessage>(GetFlowStateName(), simulationActive);
	}
}

/**
 * @brief Computes the non-transitional flow state implied by current pause and simulation flags.
 * @return The steady-state flow value that best matches the current scene flags.
 */
Scene::FlowState Scene::ComputeSteadyFlowState() const {
	// Pause always wins over simulation when deciding the externally visible steady state.
	if (pauseOverlayActive_) {
		return FlowState::Paused;
	}

	if (simulationActive) {
		return FlowState::Gameplay;
	}

	if (!currentLevelPath_.empty()) {
		return FlowState::NonSimulation;
	}

	return FlowState::Bootstrapping;
}

/**
 * @brief Recomputes flow state from pending loads, transitions, cutscenes, and pause state.
 */
void Scene::RefreshFlowState() {
	// Pending work takes precedence over the steady-state flags so external systems see transitions.
	if (pendingLevelLoad_.has_value()) {
		SetFlowState(FlowState::LoadingLevel);
		return;
	}

	if (levelTrans_.active || hasPendingStateChange_) {
		SetFlowState(FlowState::Transitioning);
		return;
	}

	if (cutscene_.active || cutTrans_.active) {
		SetFlowState(FlowState::Cutscene);
		return;
	}

	SetFlowState(ComputeSteadyFlowState());
}

// -------------------------------------------------------------------------------------------------
// Scene Loading And Reset
// -------------------------------------------------------------------------------------------------

/**
 * @brief Resets the Scene to its default empty state and applies any default setup hook.
 * @param sceneName Unused legacy scene name parameter retained for API compatibility.
 */
void Scene::LoadScene(const std::string& sceneName) {
	(void)sceneName;
	// Drop any previously tracked level path so the new scene starts from a blank boot state.
	currentLevelPath_.clear();

	// Ensure we start EMPTY per rubric (no auto-spawned objects).
	ClearAll();

	if (defaultSceneSetupHook_) {
		// Reapply any default scene scaffolding required by the current game bootstrap.
		defaultSceneSetupHook_(*this);
	}

	RefreshFlowState();
}

/**
 * @brief Performs any queued level load after the current frame's gameplay iteration completes.
 */
void Scene::HandleDeferredLoads() {
	// Process deferred level load after logic iteration completes.
	if (!pendingLevelLoad_.has_value()) {
		return;
	}

	const DeferredLevelLoadRequest request = *pendingLevelLoad_;
	if (!request.path.empty()) {
		const RuntimeLevelPipeline::LevelLoadResult loadResult = RuntimeLevelPipeline::LoadLevelIntoSceneDetailed(request.path, *this);
		if (!loadResult.success) {
			TS_LOG_ERROR("[Scene] Deferred level load failed: " << request.path << " (" << loadResult.failureReason << ")");

			// Recover from a failed blackout transition so the app does not remain visually stranded.
			auto& gfx = GetGraphicsEngine();
			if (gfx.IsTransitionActive() && gfx.IsAtBlackout()) {
				gfx.ContinueTransitionFadeIn();
			}
			cutTrans_.fadeInAfterLoad = false;
		}
		else {
			// Promote the pending level to active state only after the build succeeds.
			SetCurrentLevelPath(request.path);
			RebuildColliders();
			SetSimulationActive(request.activateSimulation);
			inputManager.ClearState(); // Avoid stale click replay.

			// If we are coming from a cutscene, fade in the new level now.
			if (cutTrans_.fadeInAfterLoad) {
				auto& gfx = GetGraphicsEngine();

				// Ensure a fade is active; if not, start a fade-in-only transition.
				if (!gfx.IsTransitionActive() || !gfx.IsAtBlackout()) {
					gfx.StartSceneTransition(0.1f, cutTrans_.inSeconds);
				}

				gfx.ContinueTransitionFadeIn();
				cutTrans_.fadeInAfterLoad = false;

				if (postLevelLoadHook_) {
					postLevelLoadHook_(*this, request.activateSimulation);
				}
			}

			if (messageBus_) {
				// Broadcast the completed load once runtime state is consistent again.
				messageBus_->Post<CoreFramework::LevelLoadedMessage>(request.path, request.activateSimulation);
			}

			RefreshFlowState();
		}
	}

	pendingLevelLoad_.reset();
	RefreshFlowState();
}

/**
 * @brief Clears runtime objects, scripts, simulation state, and object-bound audio.
 */
void Scene::ClearAll() {
	// Stop all object-bound audio before tearing down the scene.
	StopAllObjectAudio();

	// Clear scripts first so they no longer reference objects.
	logicManager.Clear(*this);
	entityManager.Clear();
	animationManager.Clear();
	movementManager.Clear();
	npcSystem.Clear();
	if (customerResetHook_) {
		// Let gameplay-specific systems clear any state that lives outside the core scene containers.
		customerResetHook_(*this);
	}

	ResetLevelObjectState();
	RefreshFlowState();
}

/**
 * @brief Resets object-linked scene caches that should not survive a level rebuild.
 */
void Scene::ResetLevelObjectState() {
	// Clear object-linked caches and transient effect state.
	// Keep cutscene/deferred-load state intact because ClearAll() is used during
	// level transitions, including cutscene skip flows that still need to finish.
	runtimeAnimatedFx_.clear();
	floatingWorldTextFx_.clear();
	ClearRuntimeTextObjects();
	uiSlides_.clear();
	pauseOverlayObjectIds_.clear();
	pendingDespawns_.clear();
	howToPlayOverlayActive_ = false;
	menuModalActive_ = false;

	objectMetadata_.Clear();
	layers.clear();
	layerSortKeyCache_.clear();
	// Recreate the default base layer so newly spawned objects have a valid destination.
	AddLayer("1");

	spriteID = -1;
	dinoID = -1;
	otherID = -1;
	otherID2 = -1;
	exitGateID_ = -1;
	exitGateCached_ = false;
	exitGateWorld_ = Math::Vector2D(0.0f, 0.0f);
	editorSelectedId = -1;
	flowStateBeforePause_ = FlowState::Gameplay;
}

/**
 * @brief Schedules a full scene clear at the start of the next input phase.
 */
void Scene::RequestClearAll() {
	// Defer the destructive scene wipe until the next safe input-phase checkpoint.
	pendingClear_ = true;
}

/**
 * @brief Queues a level for deferred loading after gameplay iteration is complete.
 * @param path Path to the level JSON file to load.
 * @param activateSimulation Whether simulation should be re-enabled after the load finishes.
 */
void Scene::QueueLevelLoad(const std::string& path, bool activateSimulation) {
	// Cache the load request so the actual rebuild can happen after active iteration completes.
	pendingLevelLoad_ = DeferredLevelLoadRequest{ path, activateSimulation };
	SetFlowState(FlowState::LoadingLevel);

	if (messageBus_) {
		messageBus_->Post<CoreFramework::LevelLoadQueuedMessage>(path, activateSimulation);
	}
}

/**
 * @brief Executes the optional post-level-load hook for editor/runtime paths that load immediately.
 * @param activeSimulation Whether the scene should be treated as gameplay-active.
 */
void Scene::RunPostLevelLoadSetup(bool activeSimulation) {
	if (postLevelLoadHook_) {
		postLevelLoadHook_(*this, activeSimulation);
	}

	RefreshFlowState();
}
