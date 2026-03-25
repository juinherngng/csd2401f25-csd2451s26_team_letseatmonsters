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

#include "../Core/LevelEditorPanelFonts.hpp"
#include "../Core/MessageBus.hpp"
#include "../Core/RuntimeLevel.hpp"

#include "SceneManager.hpp"

#include <iostream>

void Scene::SetFlowState(FlowState newState) {
	if (flowState_ == newState) {
		return;
	}

	flowState_ = newState;

	if (messageBus_) {
		messageBus_->Post<CoreFramework::SceneFlowStateChangedMessage>(GetFlowStateName(), simulationActive);
	}
}

Scene::FlowState Scene::ComputeSteadyFlowState() const {
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

void Scene::RefreshFlowState() {
	if (hasPendingLevel_) {
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

void Scene::LoadScene(const std::string& sceneName) {
	(void)sceneName;
	currentLevelPath_.clear();

	// Ensure we start EMPTY per rubric (no auto-spawned objects).
	ClearAll();

	if (defaultSceneSetupHook_) {
		defaultSceneSetupHook_(*this);
	}

	RefreshFlowState();
}

void Scene::HandleDeferredLoads() {
	// Process deferred level load after logic iteration completes.
	if (!hasPendingLevel_) {
		return;
	}

	if (!pendingLevelPath_.empty()) {
		if (!RuntimeLevel::LoadAndBuild(pendingLevelPath_, *this)) {
			std::cerr << "[Scene] Deferred level load failed: " << pendingLevelPath_ << std::endl;
		}
		else {
			SetCurrentLevelPath(pendingLevelPath_);
			RebuildColliders();
			SetSimulationActive(pendingLevelSimActive_);
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
					postLevelLoadHook_(*this, pendingLevelSimActive_);
				}
			}

			LEPANELFONTS::EnsureFontsForTextObjectsLoaded();

			if (messageBus_) {
				messageBus_->Post<CoreFramework::LevelLoadedMessage>(pendingLevelPath_, pendingLevelSimActive_);
			}

			RefreshFlowState();
		}
	}

	hasPendingLevel_ = false;
	pendingLevelPath_.clear();
	RefreshFlowState();
}

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
		customerResetHook_(*this);
	}

	ResetLevelObjectState();
	RefreshFlowState();
}

void Scene::ResetLevelObjectState() {
	// Clear object-linked caches and transient effect state.
	// Keep cutscene/deferred-load state intact because ClearAll() is used during
	// level transitions, including cutscene skip flows that still need to finish.
	runtimeAnimatedFx_.clear();
	floatingWorldTextFx_.clear();
	uiSlides_.clear();
	pauseOverlayObjectIds_.clear();
	pendingDespawns_.clear();

	defaults_.clear();
	objectTags_.clear();
	mTexturePathByID.clear();
	layers.clear();
	layerSortKeyCache_.clear();
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

void Scene::RequestClearAll() {
	pendingClear_ = true;
}

void Scene::QueueLevelLoad(const std::string& path, bool activateSimulation) {
	pendingLevelPath_ = path;
	pendingLevelSimActive_ = activateSimulation;
	hasPendingLevel_ = true;
	SetFlowState(FlowState::LoadingLevel);

	if (messageBus_) {
		messageBus_->Post<CoreFramework::LevelLoadQueuedMessage>(path, activateSimulation);
	}
}
