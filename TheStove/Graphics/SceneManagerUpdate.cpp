/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManagerUpdate.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements Scene per-frame update phases, including cutscene stepping,
					input handling, simulation updates, UI updates, and end-of-frame cleanup.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/AudioManager.hpp"
#include "../Core/FilePaths.hpp"

#include "ResourceManager.hpp"
#include "SceneManager.hpp"

#include <iostream>

// -------------------------------------------------------------------------------------------------
// Top-Level Frame Driver
// -------------------------------------------------------------------------------------------------

/**
 * @brief Advances the Scene by one frame.
 * @param deltaTime Frame delta time in seconds.
 * @param window Window used by editor/debug UI helpers.
 */
void Scene::Update(float deltaTime, GLFWwindow* window) {
#ifdef _DEBUG
	UpdateAnimationControls();
#endif

	UpdateCutscenePhase(deltaTime);
	if (!UpdateInputPhase(deltaTime)) {
		return;
	}

	const float physicsDt = physicsStep_.resolveDt(inputManager, deltaTime);
	lastPhysicsDt_ = physicsDt;

	UpdateSimulationPhase(deltaTime, physicsDt);
	HandleDeferredLoads();
	UpdateUiPhase(deltaTime, window);
	FinalizeFramePhase(deltaTime);
}

// -------------------------------------------------------------------------------------------------
// Early-Frame Cutscene And Input Phases
// -------------------------------------------------------------------------------------------------

/**
 * @brief Advances cutscene playback and level-transition state machines.
 * @param deltaTime Frame delta time in seconds.
 */
void Scene::UpdateCutscenePhase(float deltaTime) {
	// Drive both cutscene players every frame so fade and blackout transitions keep progressing.
	UpdateCutsceneTransitioned(deltaTime);
	UpdateCutscene(deltaTime);
	UpdateLevelTransition();
}

/**
 * @brief Consumes frame input, debug shortcuts, and pause/cutscene controls.
 * @param deltaTime Frame delta time in seconds.
 * @return `true` when the rest of the frame should continue, `false` when the
 *         Scene was cleared and later phases should be skipped.
 */
bool Scene::UpdateInputPhase(float deltaTime) {
#if defined(_DEBUG) && !defined(ENABLE_DEBUG_UI)
	(void)deltaTime;
#endif

#ifndef _DEBUG
	// Pause audio channels only after the fade finishes so the fade itself remains audible.
	if (pauseAudioPending_ && audioManager_) {
		pauseAudioTimer_ -= deltaTime;
		if (pauseAudioTimer_ <= 0.0f) {
			if (!pauseMusicChannel_.empty()) {
				audioManager_->PauseChannel(pauseMusicChannel_);
			}

			if (!pauseAmbienceChannel_.empty()) {
				audioManager_->PauseChannel(pauseAmbienceChannel_);
			}

			pauseAudioPending_ = false;
			std::cout << "[Scene] Paused audio channels after fade" << std::endl;
		}
	}
#endif

	if (pendingClear_) {
		ClearAll();
		RebuildColliders();
		pendingClear_ = false;
		return false;
	}

	// Swallow gameplay input during cutscenes so only the skip action remains active.
	if (IsAnyCutsceneActive()) {
		const bool spaceHeld = inputManager.IsKeyPressed(GLFW_KEY_SPACE);
		if (spaceHeld && !cutsceneSkipSpaceHeld_ && !cutsceneSkipConsumed_) {
			SkipActiveCutscene();
			cutsceneSkipConsumed_ = true;
		}

		cutsceneSkipSpaceHeld_ = spaceHeld;
		inputManager.ClearState();
	}
	else {
		cutsceneSkipSpaceHeld_ = false;
		cutsceneSkipConsumed_ = false;
#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
		inputCommandHandler.ProcessCommands(inputManager, physicsManager, movementManager, spriteID, useForces_, showAuxDebug_);
#endif
	}

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	if (inputManager.IsKeyJustPressed(GLFW_KEY_L)) {
		mLevelEditor.Toggle();
	}
#endif

#ifndef _DEBUG
	if (inputManager.IsKeyJustPressed(GLFW_KEY_F1)) {
		showFPS_ = !showFPS_;
		if (showFPS_) {
			FontSystem::Font* font = ResourceManager::Instance().GetFont("fps_font");
			if (!font) {
				font = FontSystem::FontManager::Instance().LoadFont("fps_font", FilePaths::Fonts::TO_THE_POINT, 48);
			}

			if (font) {
				fpsText_.SetFont(font);
				fpsText_.SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
				fpsText_.SetScale(1.5f);
				fpsAccumTime_ = 0.0f;
				fpsAccumFrames_ = 0;
				fpsValue_ = 60;

				const std::string fpsStr = std::string("FPS: ") + std::to_string(fpsValue_);
				fpsText_.SetText(fpsStr);

				// Seed an initial placement so the overlay is correct before the next accumulator update.
				const float estimatedTextWidth = static_cast<float>(fpsStr.length()) * 20.0f * fpsText_.GetScale();
				const float rightPadding = 20.0f;
				const float topPadding = 60.0f;
				fpsText_.SetPosition(glm::vec2(static_cast<float>(GraphicsEngine::kRefW) - estimatedTextWidth - rightPadding, topPadding));
			}
		}
	}
#endif

	return true;
}

// -------------------------------------------------------------------------------------------------
// Mid-Frame Simulation And UI Phases
// -------------------------------------------------------------------------------------------------

/**
 * @brief Runs game logic, simulation hooks, physics, NPC movement, and collision resolution.
 * @param deltaTime Frame delta time in seconds.
 * @param physicsDt Fixed-step delta time used for simulation systems.
 */
void Scene::UpdateSimulationPhase(float deltaTime, float physicsDt) {
	// Logic always runs so menu and overlay scripts continue working with simulation disabled.
	logicManager.StartAll(*this);
	logicManager.UpdateAll(deltaTime, *this, inputManager);

	if (customerUpdateHook_) {
		customerUpdateHook_(physicsDt, *this);
	}

	if (simulationActive && simulationUpdateHook_) {
		simulationUpdateHook_(deltaTime, *this);
	}

	if (simulationActive) {
		if (useForces_) {
			physicsManager.UpdatePhysics(physicsDt, entityManager, inputManager);
		}

		const collision::WalkArea walk = GetWalkArea();
		npcSystem.Update(physicsDt, entityManager, collisionManager, walk);
		HandlePlayerCollisions(physicsDt, entityManager);
		ApplyFinalConstraints(entityManager);

		if (audioManager_) {
			const float listenerX = static_cast<float>(GraphicsEngine::kRefW) * 0.5f;
			const float listenerY = static_cast<float>(GraphicsEngine::kRefH) * 0.5f;
			audioManager_->SetListenerPosition(listenerX, listenerY, 0.0f);
		}
	}
}

/**
 * @brief Updates runtime UI-facing systems after gameplay simulation.
 * @param deltaTime Frame delta time in seconds.
 * @param window Window passed through for debug/editor integrations.
 */
void Scene::UpdateUiPhase(float deltaTime, GLFWwindow* window) {
	particleSystem_.Update(deltaTime, entityManager);
	UpdateUiSlides(deltaTime);
	UpdateRuntimeAnimatedFx(deltaTime);
	UpdateFloatingWorldTextFx(deltaTime);

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	debugVisualizer.DrawDebugInfo(entityManager, collisionManager, movementManager, spriteID, showAuxDebug_);
#endif

	(void)window;
}

// -------------------------------------------------------------------------------------------------
// End-Of-Frame Cleanup And Overlay State
// -------------------------------------------------------------------------------------------------

/**
 * @brief Performs end-of-frame cleanup, overlays, and pause/resume handoff.
 * @param deltaTime Frame delta time in seconds.
 */
void Scene::FinalizeFramePhase(float deltaTime) {
#ifdef _DEBUG
	(void)deltaTime;
#endif

	for (int id : pendingDespawns_) {
		DespawnByID(id);
	}
	pendingDespawns_.clear();

#ifndef _DEBUG
	if (showFPS_) {
		fpsAccumTime_ += deltaTime;
		fpsAccumFrames_ += 1;
		if (fpsAccumTime_ >= fpsUpdateInterval_) {
			const float avg = static_cast<float>(fpsAccumFrames_) / fpsAccumTime_;
			fpsValue_ = static_cast<int>(avg + 0.5f);
			fpsAccumTime_ = 0.0f;
			fpsAccumFrames_ = 0;

			const std::string fpsStr = std::string("FPS: ") + std::to_string(fpsValue_);
			fpsText_.SetText(fpsStr);

			const float estimatedTextWidth = static_cast<float>(fpsStr.length()) * 20.0f * fpsText_.GetScale();
			const float rightPadding = 20.0f;
			const float topPadding = 60.0f;
			fpsText_.SetPosition(glm::vec2(static_cast<float>(GraphicsEngine::kRefW) - estimatedTextWidth - rightPadding, topPadding));
		}
	}

	if (inputManager.IsKeyJustPressed(GLFW_KEY_ESCAPE)) {
		if (IsPauseOverlayActive()) {
			HidePauseOverlay();
			RequestResumeFromPauseOverlay();
			inputManager.ConsumeNextKeyPress(GLFW_KEY_ESCAPE);
		}
		else if (IsSimulationActive()) {
			ShowPauseOverlay();
			inputManager.ConsumeNextKeyPress(GLFW_KEY_ESCAPE);
		}
	}

	// Resume simulation only after the overlay has been fully torn down.
	if (resumeFromPausePending_ && !pauseOverlayActive_) {
		SetSimulationActive(true);
		resumeFromPausePending_ = false;
		RefreshFlowState();
	}
#endif
}
