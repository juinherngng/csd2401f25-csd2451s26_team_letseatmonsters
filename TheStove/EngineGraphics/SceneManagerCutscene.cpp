/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManagerCutscene.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements Scene cutscene playback, cutscene skipping, UI slide animation,
					and level transition handoff logic extracted from SceneManager.cpp.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <filesystem>

#include "EngineCore/FilePaths.hpp"
#include "EngineCore/MessageBus.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/Layer.hpp"
#include "EngineGraphics/SceneManager.hpp"

namespace {
	// -------------------------------------------------------------------------------------------------
	// Local Cutscene Constants And Shared State
	// -------------------------------------------------------------------------------------------------

	/**
	 * @brief Stores boundary markers for chapter-style transitioned cutscenes.
	 */
	std::vector<bool> sCutsceneBoundaryFlags;

	constexpr const char* kCutsceneSkipTexture = "../assets/UI/cutscene_skip.png";
	constexpr glm::vec2 kCutsceneSkipSize{ 320.0f, 156.0f };
	constexpr float kCutsceneSkipMarginRight = 24.0f;
	constexpr float kCutsceneSkipMarginTop = 20.0f;
	constexpr int kCutsceneSkipSortOrder = 5000;

	/**
	 * @brief Returns whether a cutscene asset path should be routed through the MP4 video player.
	 * @param path Candidate cutscene asset path.
	 * @return `true` when the file extension matches a supported video type.
	 */
	bool IsVideoCutscenePath(const std::string& path) {
		std::string ext = std::filesystem::path(path).extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
			});
		return ext == ".mp4" || ext == ".m4v" || ext == ".mov";
	}
}

// -------------------------------------------------------------------------------------------------
// Basic Fade-Based Cutscene Player
// -------------------------------------------------------------------------------------------------

/**
 * @brief Starts a simple fade-based cutscene that ends by loading a level.
 * @param imagePaths Ordered image paths shown by the cutscene.
 * @param holdSecondsPerImage Seconds each image remains fully visible.
 * @param fadeSeconds Seconds spent fading between images.
 * @param levelJsonPath Level JSON path queued once the cutscene completes.
 * @param activateSimulation Whether the loaded level should resume simulation.
 */
void Scene::StartCutscene(const std::vector<std::string>& imagePaths,
	float holdSecondsPerImage,
	float fadeSeconds,
	const std::string& levelJsonPath,
	bool activateSimulation) {
	if (imagePaths.size() == 1 && IsVideoCutscenePath(imagePaths.front())) {
		StartVideoCutscene(imagePaths.front(), levelJsonPath, activateSimulation, false);
		return;
	}

	if (imagePaths.empty()) {
		QueueLevelLoad(levelJsonPath, activateSimulation);
		return;
	}

	SetSimulationActive(false);
	HidePauseOverlay();
	SetFlowState(FlowState::Cutscene);
	SpawnCutsceneSkipPrompt();

	if (videoCutscene_.spriteId >= 0) {
		DespawnByID(videoCutscene_.spriteId);
		videoCutscene_.spriteId = -1;
	}
	videoCutscene_.player.Close();
	videoCutscene_.active = false;

	CleanupCutsceneObjects();
	cutscene_.active = true;
	cutscene_.images = imagePaths;
	cutscene_.current = 0;
	cutscene_.holdTime = std::max(0.0f, holdSecondsPerImage);
	cutscene_.fadeTime = std::max(0.0f, fadeSeconds);
	cutscene_.t = 0.0f;
	cutscene_.phase = CutsceneState::Phase::FadeIn;
	cutscene_.targetLevelJson = levelJsonPath;
	cutscene_.targetActivateSim = activateSimulation;
	cutscene_.queuedFinalLoad = false;

	const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
	const glm::vec2 fullSize{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };

	if (GameObject* s = SpawnStaticSprite(cutscene_.images[0], center, fullSize, cutscene_.uiLayer)) {
		cutscene_.spriteA = s->GetID();
		SetSpriteAlpha(s, 0.0f);
	}
	else {
		cutscene_.active = false;
		DespawnCutsceneSkipPrompt();
		QueueLevelLoad(levelJsonPath, activateSimulation);
	}
}

/**
 * @brief Spawns the top-right "skip cutscene" prompt if it is not already present.
 */
void Scene::SpawnCutsceneSkipPrompt() {
	if (cutsceneSkipPromptId_ >= 0 && GetGameObjectByID(cutsceneSkipPromptId_)) {
		return;
	}

	const float x = static_cast<float>(GraphicsEngine::kRefW) - (kCutsceneSkipSize.x * 0.5f) - kCutsceneSkipMarginRight;
	const float y = (kCutsceneSkipSize.y * 0.5f) + kCutsceneSkipMarginTop;

	if (GameObject* prompt = SpawnStaticSprite(
		kCutsceneSkipTexture,
		glm::vec3(x, y, 0.0f),
		kCutsceneSkipSize,
		"999999")) {
		cutsceneSkipPromptId_ = prompt->GetID();
		SetObjectTexturePath(cutsceneSkipPromptId_, kCutsceneSkipTexture);
		prompt->SetRenderSortOrder(kCutsceneSkipSortOrder);
	}
}

/**
 * @brief Removes the active cutscene skip prompt, if present.
 */
void Scene::DespawnCutsceneSkipPrompt() {
	if (cutsceneSkipPromptId_ < 0) {
		return;
	}

	if (GetGameObjectByID(cutsceneSkipPromptId_)) {
		DespawnByID(cutsceneSkipPromptId_);
	}

	cutsceneSkipPromptId_ = -1;
}

/**
 * @brief Advances the simple fade-based cutscene state machine.
 * @param dt Frame delta time in seconds.
 */
void Scene::UpdateCutscene(float dt) {
	if (!cutscene_.active) {
		return;
	}

	auto getObj = [&](int id) -> GameObject* {
		return GetGameObjectByID(id);
		};

	cutscene_.t += dt;

	switch (cutscene_.phase) {
	case CutsceneState::Phase::FadeIn:
	{
		float alpha = (cutscene_.fadeTime > 0.0f) ? std::min(1.0f, cutscene_.t / cutscene_.fadeTime) : 1.0f;
		if (GameObject* a = getObj(cutscene_.spriteA)) {
			SetSpriteAlpha(a, alpha);
		}

		if (alpha >= 1.0f) {
			cutscene_.phase = CutsceneState::Phase::Hold;
			cutscene_.t = 0.0f;
		}

		break;
	}
	case CutsceneState::Phase::Hold:
	{
		if (cutscene_.t >= cutscene_.holdTime) {
			if (cutscene_.current + 1 < cutscene_.images.size()) {
				const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
				const glm::vec2 fullSize{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
				if (GameObject* b = SpawnStaticSprite(cutscene_.images[cutscene_.current + 1], center, fullSize, cutscene_.uiLayer)) {
					cutscene_.spriteB = b->GetID();
					SetSpriteAlpha(b, 0.0f);
					cutscene_.phase = CutsceneState::Phase::FadeOut;
					cutscene_.t = 0.0f;
				}
				else {
					cutscene_.current = static_cast<size_t>(cutscene_.images.size());
					cutscene_.phase = CutsceneState::Phase::FadeOut;
					cutscene_.t = 0.0f;
				}
			}
			else {
				CleanupCutsceneObjects();
				DespawnCutsceneSkipPrompt();
				cutscene_.active = false;
				if (!cutscene_.queuedFinalLoad) {
					cutscene_.queuedFinalLoad = true;
					QueueLevelLoad(cutscene_.targetLevelJson, cutscene_.targetActivateSim);
				}
			}
		}

		break;
	}
	case CutsceneState::Phase::FadeOut:
	{
		float tNorm = (cutscene_.fadeTime > 0.0f) ? std::min(1.0f, cutscene_.t / cutscene_.fadeTime) : 1.0f;
		float alphaA = 1.0f - tNorm;
		float alphaB = tNorm;

		if (GameObject* a = getObj(cutscene_.spriteA)) {
			SetSpriteAlpha(a, alphaA);
		}

		if (GameObject* b = getObj(cutscene_.spriteB)) {
			SetSpriteAlpha(b, alphaB);
		}

		if (tNorm >= 1.0f) {
			if (cutscene_.spriteA >= 0) DespawnByID(cutscene_.spriteA);
			cutscene_.spriteA = cutscene_.spriteB;
			cutscene_.spriteB = -1;
			cutscene_.current += 1;
			cutscene_.phase = CutsceneState::Phase::Hold;
			cutscene_.t = 0.0f;
		}

		break;
	}
	}
}

/**
 * @brief Despawns any transient sprites used by the simple cutscene player.
 */
void Scene::CleanupCutsceneObjects() {
	if (cutscene_.spriteA >= 0) {
		DespawnByID(cutscene_.spriteA);
		cutscene_.spriteA = -1;
	}

	if (cutscene_.spriteB >= 0) {
		DespawnByID(cutscene_.spriteB);
		cutscene_.spriteB = -1;
	}
}

/**
 * @brief Applies an alpha tint to a sprite while preserving its full UV rectangle.
 * @param obj Sprite object to modify.
 * @param alpha Alpha value in the range `[0, 1]`.
 */
void Scene::SetSpriteAlpha(GameObject* obj, float alpha) {
	if (!obj) {
		return;
	}

	float a = std::clamp(alpha, 0.0f, 1.0f);
	obj->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, a));
	obj->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
}

// -------------------------------------------------------------------------------------------------
// Transitioned Cutscene Player
// -------------------------------------------------------------------------------------------------

/**
 * @brief Starts the transitioned cutscene player that uses blackout fades between images.
 * @param imagePaths Ordered image paths shown by the cutscene.
 * @param levelJsonPath Level JSON path queued once the cutscene completes.
 * @param activateSimulation Whether the loaded level should resume simulation.
 * @param fadeOutSeconds Seconds spent fading out to blackout.
 * @param fadeInSeconds Seconds spent fading back in from blackout.
 * @param holdSeconds Seconds each image remains visible between transitions.
 * @param crossfadeFromIndex Optional boundary index that should crossfade instead of black out.
 * @param crossfadeSeconds Seconds spent on the optional crossfade.
 */
void Scene::StartCutsceneTransitioned(const std::vector<std::string>& imagePaths,
	const std::string& levelJsonPath,
	bool activateSimulation,
	float fadeOutSeconds,
	float fadeInSeconds,
	float holdSeconds,
	int crossfadeFromIndex,
	float crossfadeSeconds) {
	if (imagePaths.size() == 1 && IsVideoCutscenePath(imagePaths.front())) {
		StartVideoCutscene(imagePaths.front(), levelJsonPath, activateSimulation, false);
		return;
	}

	if (imagePaths.empty()) {
		QueueLevelLoad(levelJsonPath, activateSimulation);
		return;
	}

	SetSimulationActive(false);
	HidePauseOverlay();
	SetFlowState(FlowState::Cutscene);
	SpawnCutsceneSkipPrompt();

	if (videoCutscene_.spriteId >= 0) {
		DespawnByID(videoCutscene_.spriteId);
		videoCutscene_.spriteId = -1;
	}
	videoCutscene_.player.Close();
	videoCutscene_.active = false;

	if (cutTrans_.currentSpriteId >= 0) {
		DespawnByID(cutTrans_.currentSpriteId);
	}

	cutTrans_ = {};
	cutTrans_.active = true;
	cutTrans_.images = imagePaths;
	cutTrans_.index = 0;
	cutTrans_.targetLevelJson = levelJsonPath;
	cutTrans_.targetActivateSim = activateSimulation;
	cutTrans_.outSeconds = fadeOutSeconds;
	cutTrans_.inSeconds = fadeInSeconds;
	cutTrans_.holdSeconds = std::max(0.0f, holdSeconds);
	cutTrans_.holdElapsed = 0.0f;
	cutTrans_.holding = false;
	cutTrans_.awaitingBlackout = false;
	cutTrans_.awaitingInitialFadeIn = false;

	cutTrans_.useCrossfade = (crossfadeFromIndex >= 0);
	cutTrans_.crossfadeSeconds = std::max(0.05f, crossfadeSeconds);
	cutTrans_.crossfadeFromIndex = crossfadeFromIndex;

	auto& gfx = GetGraphicsEngine();
	gfx.StartSceneTransition(cutTrans_.outSeconds, cutTrans_.inSeconds);
	cutTrans_.awaitingBlackout = true;
}

/**
 * @brief Starts a transitioned cutscene with explicit boundary flags for each image.
 * @param imagePaths Ordered image paths shown by the cutscene.
 * @param boundaryFlags Boundary flags used to decide blackout versus instant swap transitions.
 * @param levelJsonPath Level JSON path queued once the cutscene completes.
 * @param activateSimulation Whether the loaded level should resume simulation.
 * @param fadeOutSeconds Seconds spent fading out to blackout.
 * @param fadeInSeconds Seconds spent fading back in from blackout.
 * @param holdSeconds Seconds each image remains visible between transitions.
 * @param crossfadeFromIndex Optional boundary index that should crossfade instead of black out.
 * @param crossfadeSeconds Seconds spent on the optional crossfade.
 */
void Scene::StartCutsceneTransitionedBounded(const std::vector<std::string>& imagePaths,
	const std::vector<bool>& boundaryFlags,
	const std::string& levelJsonPath,
	bool activateSimulation,
	float fadeOutSeconds,
	float fadeInSeconds,
	float holdSeconds,
	int crossfadeFromIndex,
	float crossfadeSeconds) {
	sCutsceneBoundaryFlags = boundaryFlags;

	StartCutsceneTransitioned(imagePaths,
		levelJsonPath,
		activateSimulation,
		fadeOutSeconds,
		fadeInSeconds,
		holdSeconds,
		crossfadeFromIndex,
		crossfadeSeconds);
}

/**
 * @brief Starts a fullscreen video cutscene that ends by queueing a follow-up level load.
 * @param videoPath Path to the MP4/video file that should be decoded and displayed.
 * @param levelJsonPath Level JSON path queued when playback completes or is skipped.
 * @param activateSimulation Whether the follow-up level should resume active simulation.
 * @param loop Whether the video should restart automatically instead of ending.
 */
void Scene::StartVideoCutscene(const std::string& videoPath,
	const std::string& levelJsonPath,
	bool activateSimulation,
	bool loop) {
	if (videoPath.empty()) {
		QueueLevelLoad(levelJsonPath, activateSimulation);
		return;
	}

	SetSimulationActive(false);
	HidePauseOverlay();
	SetFlowState(FlowState::Cutscene);
	SpawnCutsceneSkipPrompt();

	CleanupCutsceneObjects();
	if (cutTrans_.currentSpriteId >= 0) {
		DespawnByID(cutTrans_.currentSpriteId);
		cutTrans_.currentSpriteId = -1;
	}

	if (videoCutscene_.spriteId >= 0) {
		DespawnByID(videoCutscene_.spriteId);
		videoCutscene_.spriteId = -1;
	}

	videoCutscene_.player.Close();
	videoCutscene_.active = false;
	videoCutscene_.videoPath.clear();
	videoCutscene_.targetLevelJson.clear();
	videoCutscene_.targetActivateSim = true;
	videoCutscene_.queuedFinalLoad = false;
	videoCutscene_.loop = false;

	if (!videoCutscene_.player.Open(videoPath, loop)) {
		// Fall back to the next level immediately if the video cannot be opened.
		DespawnCutsceneSkipPrompt();
		QueueLevelLoad(levelJsonPath, activateSimulation);
		return;
	}

	videoCutscene_.active = true;
	videoCutscene_.videoPath = videoPath;
	videoCutscene_.targetLevelJson = levelJsonPath;
	videoCutscene_.targetActivateSim = activateSimulation;
	videoCutscene_.loop = loop;

	// Render the decoded frames through a normal fullscreen sprite so the existing renderer can stay unchanged.
	const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
	const glm::vec2 fullSize{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
	if (GameObject* sprite = SpawnStaticSprite(FilePaths::Textures::PLACEHOLDER, center, fullSize, videoCutscene_.uiLayer)) {
		videoCutscene_.spriteId = sprite->GetID();
		sprite->SetTexture(videoCutscene_.player.GetTexture());
		SetObjectTexturePath(videoCutscene_.spriteId, videoPath);
	}
	else {
		videoCutscene_.player.Close();
		videoCutscene_.active = false;
		videoCutscene_.videoPath.clear();
		videoCutscene_.targetLevelJson.clear();
		videoCutscene_.targetActivateSim = true;
		videoCutscene_.queuedFinalLoad = false;
		videoCutscene_.loop = false;
		DespawnCutsceneSkipPrompt();
		QueueLevelLoad(levelJsonPath, activateSimulation);
	}
}

/**
 * @brief Advances active video playback and hands off to the next level when the video ends.
 * @param dt Frame delta time in seconds.
 */
void Scene::UpdateVideoCutscene(float dt) {
	if (!videoCutscene_.active) {
		return;
	}

	// Advance decode/upload so the bound sprite texture stays in sync with playback.
	videoCutscene_.player.Update(dt);

	if (GameObject* sprite = GetGameObjectByID(videoCutscene_.spriteId)) {
		sprite->SetTexture(videoCutscene_.player.GetTexture());
	}

	if (videoCutscene_.player.HasEnded()) {
		if (videoCutscene_.spriteId >= 0) {
			DespawnByID(videoCutscene_.spriteId);
			videoCutscene_.spriteId = -1;
		}

		// Once playback is done, tear down the transient cutscene state and continue the normal flow.
#ifndef _DEBUG
		if (cutsceneBeforeFinalLoadHook_) {
			cutsceneBeforeFinalLoadHook_(*this, 0.15f);
		}
#endif
		videoCutscene_.player.Close();
		videoCutscene_.active = false;
		DespawnCutsceneSkipPrompt();

		if (!videoCutscene_.queuedFinalLoad) {
			videoCutscene_.queuedFinalLoad = true;
			QueueLevelLoad(videoCutscene_.targetLevelJson, videoCutscene_.targetActivateSim);
		}
	}
}

/**
 * @brief Advances the blackout/crossfade cutscene player and its final level handoff.
 * @param dt Frame delta time in seconds.
 */
void Scene::UpdateCutsceneTransitioned(float dt) {
	if (!cutTrans_.active) {
		return;
	}

	auto* gfx = &GetGraphicsEngine();
	if (!gfx) {
		return;
	}

	if (cutTrans_.useCrossfade && cutTrans_.crossfading) {
		cutTrans_.crossfadeT += dt;
		float tNorm = std::min(1.0f, cutTrans_.crossfadeT / cutTrans_.crossfadeSeconds);

		if (GameObject* a = GetGameObjectByID(cutTrans_.currentSpriteId)) {
			SetSpriteAlpha(a, 1.0f - tNorm);
		}

		if (GameObject* b = GetGameObjectByID(cutTrans_.nextSpriteId)) {
			SetSpriteAlpha(b, tNorm);
		}

		if (tNorm >= 1.0f) {
			if (cutTrans_.currentSpriteId >= 0) {
				DespawnByID(cutTrans_.currentSpriteId);
			}

			cutTrans_.currentSpriteId = cutTrans_.nextSpriteId;
			cutTrans_.nextSpriteId = -1;
			cutTrans_.crossfading = false;
			cutTrans_.holding = true;
			cutTrans_.holdElapsed = 0.0f;
		}

		return;
	}

	if (cutTrans_.holding && !gfx->IsTransitionActive()) {
		cutTrans_.holdElapsed += dt;
		if (cutTrans_.holdElapsed >= cutTrans_.holdSeconds) {
			const size_t nextIndex = cutTrans_.index + 1;
			if (nextIndex < cutTrans_.images.size()) {
				const bool isBoundary = (nextIndex < sCutsceneBoundaryFlags.size())
					? sCutsceneBoundaryFlags[nextIndex]
					: true;

				// Boundary markers decide whether the next image should black out, crossfade, or swap instantly.
				if (isBoundary) {
					if (cutTrans_.useCrossfade && static_cast<int>(nextIndex) == cutTrans_.crossfadeFromIndex) {
						if (Layer* menuLayer = GetLayer("10")) {
							menuLayer->SetVisible(false);
							menuLayer->SetEnabled(false);
						}

						const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
						const glm::vec2 full{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
						const glm::vec3 topCenter{ center.x, center.y, 0.001f };
						if (GameObject* b = SpawnStaticSprite(cutTrans_.images[nextIndex], topCenter, full, cutTrans_.uiLayer)) {
							cutTrans_.nextSpriteId = b->GetID();
							SetSpriteAlpha(b, 0.0f);
							cutTrans_.crossfading = true;
							cutTrans_.crossfadeT = 0.0f;
							cutTrans_.index = nextIndex;
							cutTrans_.holding = false;
						}
						else {
							gfx->StartSceneTransition(cutTrans_.outSeconds, cutTrans_.inSeconds);
							cutTrans_.awaitingBlackout = true;
							cutTrans_.holding = false;
							cutTrans_.holdElapsed = 0.0f;
						}
					}
					else {
						gfx->StartSceneTransition(cutTrans_.outSeconds, cutTrans_.inSeconds);
						cutTrans_.awaitingBlackout = true;
						cutTrans_.holding = false;
						cutTrans_.holdElapsed = 0.0f;
					}
				}
				else {
					if (cutTrans_.currentSpriteId >= 0) {
						DespawnByID(cutTrans_.currentSpriteId);
					}

					const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
					const glm::vec2 full{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
					if (GameObject* s = SpawnStaticSprite(cutTrans_.images[nextIndex], center, full, cutTrans_.uiLayer)) {
						cutTrans_.currentSpriteId = s->GetID();
					}
					cutTrans_.index = nextIndex;
					cutTrans_.holdElapsed = 0.0f;
					cutTrans_.holding = true;
				}
			}
			else {
				// Final boundary fades back to the gameplay level instead of another cutscene image.
				float finalOutSeconds = cutTrans_.outSeconds;
				if (cutTrans_.targetLevelJson.find("win") != std::string::npos) {
					finalOutSeconds = std::max(finalOutSeconds, 2.5f);
				}

				gfx->StartSceneTransition(finalOutSeconds, cutTrans_.inSeconds);
				cutTrans_.awaitingBlackout = true;
				cutTrans_.holding = false;

#ifndef _DEBUG
				if (cutsceneFadeOutHook_) {
					cutsceneFadeOutHook_(*this, finalOutSeconds);
				}
#endif
			}
		}
	}

	if (cutTrans_.awaitingBlackout && gfx->IsAtBlackout()) {
		cutTrans_.awaitingBlackout = false;

		if (cutTrans_.currentSpriteId < 0 && cutTrans_.index == 0) {
			const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
			const glm::vec2 full{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
			if (GameObject* s = SpawnStaticSprite(cutTrans_.images[0], center, full, cutTrans_.uiLayer)) {
				cutTrans_.currentSpriteId = s->GetID();
			}

			gfx->ContinueTransitionFadeIn();
			cutTrans_.holding = true;
			cutTrans_.holdElapsed = 0.0f;

#ifndef _DEBUG
			if (cutsceneFirstFrameHook_ && !cutTrans_.images.empty()) {
				cutsceneFirstFrameHook_(*this, cutTrans_.images[0]);
			}
#endif
			return;
		}

		const size_t nextIndex = cutTrans_.index + 1;
		if (nextIndex < cutTrans_.images.size()) {
			if (cutTrans_.currentSpriteId >= 0) {
				DespawnByID(cutTrans_.currentSpriteId);
			}

			const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
			const glm::vec2 full{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
			if (GameObject* s = SpawnStaticSprite(cutTrans_.images[nextIndex], center, full, cutTrans_.uiLayer)) {
				cutTrans_.currentSpriteId = s->GetID();
			}

			cutTrans_.index = nextIndex;

			gfx->ContinueTransitionFadeIn();
			cutTrans_.holding = true;
			cutTrans_.holdElapsed = 0.0f;
		}
		else {
			if (cutTrans_.currentSpriteId >= 0) {
				DespawnByID(cutTrans_.currentSpriteId);
				cutTrans_.currentSpriteId = -1;
			}

#ifndef _DEBUG
			if (cutsceneBeforeFinalLoadHook_) {
				cutsceneBeforeFinalLoadHook_(*this, cutTrans_.outSeconds);
			}
#endif
			DespawnCutsceneSkipPrompt();
			cutTrans_.active = false;
			QueueLevelLoad(cutTrans_.targetLevelJson, cutTrans_.targetActivateSim);
			cutTrans_.fadeInAfterLoad = true;
		}
	}
}

// -------------------------------------------------------------------------------------------------
// UI Slide Helpers
// -------------------------------------------------------------------------------------------------

/**
 * @brief Spawns a UI sprite above the screen and registers it for slide-in animation.
 * @param targetPos Final on-screen position for the UI sprite.
 * @param size Sprite size.
 * @param layer Layer used to render the sprite.
 * @param texturePath Texture used for the sprite.
 * @param duration Slide duration in seconds.
 * @return Identifier of the spawned UI object, or `-1` when spawning fails.
 */
int Scene::TriggerOrderUiSlideIn(const glm::vec2& targetPos,
	const glm::vec2& size,
	const std::string& layer,
	const std::string& texturePath,
	float duration) {
	glm::vec2 startPos = { targetPos.x, -size.y * 0.5f };

	GameObject* ui = SpawnStaticSprite(texturePath.c_str(),
		{ startPos.x, startPos.y, 0.f },
		size,
		layer);
	if (!ui) {
		return -1;
	}

	const int id = ui->GetID();

	SetObjectTexturePath(id, texturePath);

	UiSlide s;
	s.objectId = id;
	s.startPos = startPos;
	s.targetPos = targetPos;
	s.t = 0.0f;
	s.duration = std::max(0.001f, duration);
	s.active = true;
	uiSlides_.push_back(s);

	return id;
}

/**
 * @brief Advances active UI slide animations and removes finished entries.
 * @param dt Frame delta time in seconds.
 */
void Scene::UpdateUiSlides(float dt) {
	if (uiSlides_.empty()) {
		return;
	}

	for (auto& s : uiSlides_) {
		if (!s.active) {
			continue;
		}

		s.t += dt;
		const float norm = std::clamp(s.t / s.duration, 0.0f, 1.0f);
		const float eased = EaseOutCubic(norm);

		const float x = s.startPos.x + (s.targetPos.x - s.startPos.x) * eased;
		const float y = s.startPos.y + (s.targetPos.y - s.startPos.y) * eased;

		if (GameObject* obj = GetGameObjectByID(s.objectId)) {
			glm::vec3 p = obj->GetPositionGLM();
			p.x = x;
			p.y = y;
			obj->SetPosition(p);
		}

		if (norm >= 1.0f) {
			s.active = false;
			if (GameObject* obj = GetGameObjectByID(s.objectId)) {
				glm::vec3 p = obj->GetPositionGLM();
				p.x = s.targetPos.x;
				p.y = s.targetPos.y;
				obj->SetPosition(p);
			}
		}
	}

	uiSlides_.erase(
		std::remove_if(uiSlides_.begin(), uiSlides_.end(),
			[](const UiSlide& s) {
				return !s.active;
			}),

		uiSlides_.end()
	);
}

// -------------------------------------------------------------------------------------------------
// Cutscene And Level Skip/Transition Control
// -------------------------------------------------------------------------------------------------

/**
 * @brief Starts a fade-to-black level transition without a cutscene image sequence.
 * @param levelJsonPath Level JSON path queued once blackout is reached.
 * @param activateSimulation Whether the loaded level should resume simulation.
 * @param fadeOutSeconds Seconds spent fading out to blackout.
 * @param fadeInSeconds Seconds spent fading back in from blackout.
 */
void Scene::StartLevelTransition(const std::string& levelJsonPath,
	bool activateSimulation,
	float fadeOutSeconds,
	float fadeInSeconds) {
	auto& gfx = GetGraphicsEngine();

	if (levelTrans_.active) {
		// Keep the latest destination so rapid menu clicks do not leave the scene stuck
		// waiting on an outdated target while the fade is already in progress.
		levelTrans_.targetJson = levelJsonPath;
		levelTrans_.targetActivateSim = activateSimulation;
		levelTrans_.outSec = fadeOutSeconds;
		levelTrans_.inSec = fadeInSeconds;
		cutTrans_.inSeconds = fadeInSeconds;

		// Recover if the renderer has already cleared its transition while the scene still
		// believes a blackout handoff is pending.
		if (!gfx.IsTransitionActive()) {
			gfx.StartSceneTransition(levelTrans_.outSec, levelTrans_.inSec);
		}
		return;
	}

	// Fast menu-to-menu clicks can arrive while the previous level is still fading in.
	// Restart the renderer transition cleanly so the new request can reach blackout.
	if (gfx.IsTransitionActive() && !gfx.IsAtBlackout()) {
		gfx.CancelSceneTransition();
	}

#ifndef _DEBUG
	SetSimulationActive(false);
#endif
	HidePauseOverlay();

	levelTrans_.active = true;
	levelTrans_.awaitingBlackout = true;
	levelTrans_.targetJson = levelJsonPath;
	levelTrans_.targetActivateSim = activateSimulation;
	levelTrans_.outSec = fadeOutSeconds;
	levelTrans_.inSec = fadeInSeconds;
	SetFlowState(FlowState::Transitioning);

	cutTrans_.inSeconds = fadeInSeconds;
	gfx.StartSceneTransition(levelTrans_.outSec, levelTrans_.inSec);
}

/**
 * @brief Finalizes a direct level transition once the renderer reaches blackout.
 */
void Scene::UpdateLevelTransition() {
	if (!levelTrans_.active) {
		return;
	}

	auto& gfx = GetGraphicsEngine();

	if (levelTrans_.awaitingBlackout && gfx.IsAtBlackout()) {
		levelTrans_.awaitingBlackout = false;

		QueueLevelLoad(levelTrans_.targetJson, levelTrans_.targetActivateSim);

		cutTrans_.fadeInAfterLoad = true;

		levelTrans_.active = false;
		SetFlowState(FlowState::LoadingLevel);
	}
}

/**
 * @brief Skips the currently active cutscene and publishes the appropriate skip event.
 */
void Scene::SkipActiveCutscene() {
	bool publishedSkipEvent = false;

	if (videoCutscene_.active) {
#ifndef _DEBUG
		if (skipCutsceneAudioHook_) {
			skipCutsceneAudioHook_(*this, 0.15f);
		}
#endif

		if (videoCutscene_.spriteId >= 0) {
			DespawnByID(videoCutscene_.spriteId);
			videoCutscene_.spriteId = -1;
		}

		videoCutscene_.player.Close();
		videoCutscene_.active = false;
		DespawnCutsceneSkipPrompt();

		if (!videoCutscene_.queuedFinalLoad) {
			videoCutscene_.queuedFinalLoad = true;
			QueueLevelLoad(videoCutscene_.targetLevelJson, videoCutscene_.targetActivateSim);
		}

		if (messageBus_) {
			messageBus_->Post<CoreFramework::CutsceneSkippedMessage>(false, videoCutscene_.targetLevelJson);
		}
		return;
	}

	if (cutTrans_.active) {
		auto& gfx = GetGraphicsEngine();
		const float skipFadeOutSeconds = cutTrans_.outSeconds * 2.0f;

		if (gfx.IsTransitionActive()) {
			gfx.CancelSceneTransition();
		}

		gfx.StartSceneTransition(skipFadeOutSeconds, cutTrans_.inSeconds);
		cutTrans_.outSeconds = skipFadeOutSeconds;

		cutTrans_.awaitingBlackout = true;
		cutTrans_.holding = false;
		cutTrans_.crossfading = false;
		cutTrans_.holdElapsed = 0.0f;
		SetFlowState(FlowState::Transitioning);

		if (!cutTrans_.images.empty()) {
			cutTrans_.index = cutTrans_.images.size() - 1;
		}

#ifndef _DEBUG
		if (skipCutsceneAudioHook_) {
			skipCutsceneAudioHook_(*this, skipFadeOutSeconds);
		}
#endif

		if (messageBus_) {
			messageBus_->Post<CoreFramework::CutsceneSkippedMessage>(true, cutTrans_.targetLevelJson);
			publishedSkipEvent = true;
		}
	}

	if (cutscene_.active) {
		CleanupCutsceneObjects();
		DespawnCutsceneSkipPrompt();
		cutscene_.active = false;

		if (!cutscene_.queuedFinalLoad) {
			cutscene_.queuedFinalLoad = true;
			QueueLevelLoad(cutscene_.targetLevelJson, cutscene_.targetActivateSim);
		}

		if (messageBus_ && !publishedSkipEvent) {
			messageBus_->Post<CoreFramework::CutsceneSkippedMessage>(false, cutscene_.targetLevelJson);
		}
	}
}
