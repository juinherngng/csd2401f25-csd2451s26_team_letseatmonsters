/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManagerPauseUi.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements Scene pause overlay controls and related pause-menu UI object
					spawning, teardown, and audio fade behavior.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/AudioManager.hpp"
#include "../Core/FilePaths.hpp"
#include "../Core/Logger.hpp"
#include "../Core/MessageBus.hpp"

#include "SceneManager.hpp"

// -------------------------------------------------------------------------------------------------
// Pause Overlay Entry
// -------------------------------------------------------------------------------------------------

/**
 * @brief Spawns the pause overlay and fades gameplay audio down while gameplay is halted.
 */
void Scene::ShowPauseOverlay() {
#ifndef _DEBUG
	if (pauseOverlayActive_) {
		return;
	}

	flowStateBeforePause_ = ComputeSteadyFlowState();
	pauseOverlayActive_ = true;
	SetFlowState(FlowState::Paused);

	if (messageBus_) {
		messageBus_->Post<CoreFramework::PauseOverlayChangedMessage>(true);
	}

	TS_LOG_DEBUG("[Scene] ShowPauseOverlay()");

	SetSimulationActive(false);

	if (audioManager_) {
		const float pauseFadeOut = 0.2f;

		pausedBgmVolume_ = audioManager_->GetBgmVolume();
		pausedAmbienceVolume_ = audioManager_->GetBgmVolume() * 0.5f;

		if (!pauseMusicChannel_.empty()) {
			audioManager_->FadeChannel(pauseMusicChannel_, 0.0f, pauseFadeOut);
		}

		if (!pauseAmbienceChannel_.empty()) {
			audioManager_->FadeChannel(pauseAmbienceChannel_, 0.0f, pauseFadeOut);
		}

		pauseAudioPending_ = true;
		pauseAudioTimer_ = pauseFadeOut;

		TS_LOG_DEBUG("[Scene] Fading out level BGM and ambience for pause menu");
	}

	const std::string uiLayer = "999999";

	// Keep the pause overlay on a dedicated top-most UI layer so gameplay objects remain untouched.
	if (GameObject* dim = SpawnStaticSprite(FilePaths::Textures::PAUSED_BG,
		{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f },
		{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) },
		uiLayer)) {
		pauseOverlayObjectIds_.push_back(dim->GetID());
		TS_LOG_DEBUG("[Scene] Pause background id=" << dim->GetID());
	}

	auto spawnPauseBtn = [&](const char* tex, const glm::vec2& pos, const std::string& action) {
		if (GameObject* b = SpawnStaticSprite(tex, { pos.x, pos.y, 0.0f }, { 350.0f, 100.0f }, uiLayer)) {
			const int id = b->GetID();
			pauseOverlayObjectIds_.push_back(id);
			SetObjectTexturePath(id, tex);

			if (pauseOverlayButtonBinder_) {
				pauseOverlayButtonBinder_(*this, id, action);
			}
		}
		else {
			TS_LOG_ERROR("[Scene] Failed to spawn pause button for action=" << action);
		}
		};

	spawnPauseBtn(FilePaths::Textures::BTN_RESUME, { 1300.f, 454.f }, "resume");
	spawnPauseBtn(FilePaths::Textures::BTN_HOW, { 1300.f, 584.f }, "howtoplay");
	spawnPauseBtn(FilePaths::Textures::BTN_QUIT, { 1300.f, 714.f }, "quit");
#endif
}

// -------------------------------------------------------------------------------------------------
// Pause Overlay Exit And Resume Handoff
// -------------------------------------------------------------------------------------------------

/**
 * @brief Requests gameplay resume once the pause overlay teardown has completed.
 */
void Scene::RequestResumeFromPauseOverlay() {
#ifndef _DEBUG
	resumeFromPausePending_ = true;
	if (!pauseOverlayActive_) {
		RefreshFlowState();
	}
#endif
}

/**
 * @brief Removes the pause overlay and restores the paused gameplay audio mix.
 */
void Scene::HidePauseOverlay() {
#ifndef _DEBUG
	if (!pauseOverlayActive_) return;
	for (int id : pauseOverlayObjectIds_) {
		DespawnByID(id);
	}
	pauseOverlayObjectIds_.clear();
	pauseOverlayActive_ = false;

	pauseAudioPending_ = false;

	if (audioManager_) {
		const float pauseFadeIn = 0.2f;

		if (!pauseMusicChannel_.empty()) {
			audioManager_->ResumeChannel(pauseMusicChannel_);
		}

		if (!pauseAmbienceChannel_.empty()) {
			audioManager_->ResumeChannel(pauseAmbienceChannel_);
		}

		if (!pauseMusicChannel_.empty()) {
			audioManager_->FadeChannel(pauseMusicChannel_, pausedBgmVolume_, pauseFadeIn);
		}

		if (!pauseAmbienceChannel_.empty()) {
			audioManager_->FadeChannel(pauseAmbienceChannel_, pausedAmbienceVolume_, pauseFadeIn);
		}

		TS_LOG_DEBUG("[Scene] Resumed and fading in level BGM and ambience after pause menu");
	}
	SetFlowState(resumeFromPausePending_ ? flowStateBeforePause_ : ComputeSteadyFlowState());

	if (messageBus_) {
		messageBus_->Post<CoreFramework::PauseOverlayChangedMessage>(false);
	}
#endif
}

