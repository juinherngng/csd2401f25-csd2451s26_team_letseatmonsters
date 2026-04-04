/*
----------------------------------------------------------------------------------------------------
 FILE NAME:         Quota.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu   (80%)
 CO-AUTHOR:         Ng Juin Herng, juinherng.ng@digipen.edu (20%)

 DESCRIPTION:       Defines the Economy namespace, which tracks player money,
					win quota, and remaining time, and synchronizes these values
					with the game UI and win/lose conditions.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/FilePaths.hpp"
#include "EngineCore/GameStateManager.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/Quota.hpp"

namespace Economy {
	namespace {
		struct HudTextBase {
			bool captured = false;
			float x = 0.0f;
			float y = 0.0f;
			float scale = 1.0f;
			float colorR = 1.0f;
			float colorG = 1.0f;
			float colorB = 1.0f;
			float colorA = 1.0f;
		};

		struct HudPresentationState {
			std::string levelPath;
			int lastWholeSeconds = -1;
			float animationClock = 0.0f;
			float minuteAlertRemaining = 0.0f;
			float moneyPulseRemaining = 0.0f;
			HudTextBase timerText{};
			HudTextBase moneyText{};
		};

		HudPresentationState gHudPresentation;

		static constexpr float kMinuteAlertDuration = 0.78f;
		static constexpr float kMoneyPulseDuration = 0.58f;
		static constexpr float kPi = 3.14159265358979323846f;
		static constexpr float kMoneyTextRedR = 0.86f;
		static constexpr float kMoneyTextRedG = 0.16f;
		static constexpr float kMoneyTextRedB = 0.16f;
		static constexpr float kMoneyTextGreenR = 0.18f;
		static constexpr float kMoneyTextGreenG = 0.78f;
		static constexpr float kMoneyTextGreenB = 0.24f;
		static constexpr float kTimerTextAlertRedR = 0.95f;
		static constexpr float kTimerTextAlertRedG = 0.18f;
		static constexpr float kTimerTextAlertRedB = 0.18f;

		static void CaptureRuntimeTextBase(const Scene& scene, const char* textName, HudTextBase& outBase) {
			if (textName == nullptr || textName[0] == '\0') {
				return;
			}

			for (const RuntimeTextData& textObj : scene.GetRuntimeTextObjects()) {
				if (textObj.name == textName) {
					outBase.captured = true;
					outBase.x = textObj.x;
					outBase.y = textObj.y;
					outBase.scale = textObj.scale;
					outBase.colorR = textObj.colorR;
					outBase.colorG = textObj.colorG;
					outBase.colorB = textObj.colorB;
					outBase.colorA = textObj.colorA;
					return;
				}
			}
		}

		static int GetDisplayedWholeSeconds() {
			return std::max(0, static_cast<int>(gTimeRemaining + 0.999f));
		}

		static void EnsureHudPresentationBindings(Scene& scene) {
			const std::string currentLevelPath = scene.GetCurrentLevelPath();
			if (gHudPresentation.levelPath != currentLevelPath) {
				gHudPresentation = HudPresentationState{};
				gHudPresentation.levelPath = currentLevelPath;
				gHudPresentation.lastWholeSeconds = GetDisplayedWholeSeconds();
			}

			if (!gHudPresentation.timerText.captured) {
				CaptureRuntimeTextBase(scene, "TimerText", gHudPresentation.timerText);
			}

			if (!gHudPresentation.moneyText.captured) {
				CaptureRuntimeTextBase(scene, "MoneyText", gHudPresentation.moneyText);
			}
		}

		static float ComputeDampedPulse(float remaining, float duration, float cycles) {
			if (remaining <= 0.0f || duration <= 0.0f) {
				return 0.0f;
			}

			const float phase = 1.0f - (remaining / duration);
			const float envelope = 1.0f - phase;
			return std::abs(std::sin(phase * cycles * kPi)) * envelope;
		}

		static glm::vec2 ComputeMinuteAlertOffset(float remaining) {
			if (remaining <= 0.0f) {
				return glm::vec2(0.0f, 0.0f);
			}

			const float phase = 1.0f - (remaining / kMinuteAlertDuration);
			const float envelope = 1.0f - phase;
			const float shakeAmplitude = 7.0f * envelope;
			return glm::vec2(
				std::sin(phase * 58.0f) * shakeAmplitude,
				std::cos(phase * 51.0f) * shakeAmplitude * 0.85f);
		}

		static glm::vec2 ComputeFinalCountdownOffset(float animationClock, int wholeSeconds) {
			if (wholeSeconds <= 0 || wholeSeconds > 10) {
				return glm::vec2(0.0f, 0.0f);
			}

			const float warning01 = std::clamp((10.0f - static_cast<float>(wholeSeconds)) / 9.0f, 0.0f, 1.0f);
			const float trauma = 0.42f + 0.58f * warning01;
			const float shakeStrength = trauma * trauma;
			const float xAmp = 0.75f + 1.15f * shakeStrength;
			const float yAmp = 0.45f + 0.85f * shakeStrength;
			return glm::vec2(
				std::sin(animationClock * (13.5f + 2.5f * warning01)) * xAmp,
				std::cos(animationClock * (11.0f + 2.0f * warning01) + 0.45f) * yAmp);
		}

		static float ComputeFinalCountdownScaleBoost(float animationClock, int wholeSeconds) {
			if (wholeSeconds <= 0 || wholeSeconds > 10) {
				return 0.0f;
			}

			const float warning01 = std::clamp((10.0f - static_cast<float>(wholeSeconds)) / 9.0f, 0.0f, 1.0f);
			const float pulseWave = 0.25f + 0.75f * std::pow(
				0.5f + 0.5f * std::sin(animationClock * (9.0f + 2.0f * warning01) + 0.55f),
				1.85f);
			return (0.18f + 0.14f * warning01) * pulseWave;
		}

		// Collect a sequential cutscene authored as either:
		//   prefix1.png, prefix2.png, ...
		// or chapter/subframe groups:
		//   prefix1.1.png, prefix1.2.png, prefix1.3.png, prefix2.1.png, ...
		//
		// Boundary flags are only set on the first frame of each chapter so
		// in-chapter subframes swap directly while chapter changes fade through
		// black using the transitioned cutscene player.
		static void BuildSequentialFramesAndBoundaries(std::vector<std::string>& outFrames,
			std::vector<bool>& outFlags,
			const std::string& baseFolder,
			const std::string& prefix) {
			namespace fs = std::filesystem;

			outFrames.clear();
			outFlags.clear();

			for (int chapter = 1;; ++chapter) {
				const std::string chapterBase = baseFolder + "/" + prefix + std::to_string(chapter);
				const std::string directPath = chapterBase + ".png";

				if (fs::exists(directPath)) {
					outFrames.push_back(directPath);
					outFlags.push_back(true);
					continue;
				}

				for (int subframe = 1;; ++subframe) {
					const std::string dottedPath = chapterBase + "." + std::to_string(subframe) + ".png";
					const std::string commaPath = chapterBase + "," + std::to_string(subframe) + ".png";

					if (fs::exists(dottedPath)) {
						outFrames.push_back(dottedPath);
						outFlags.push_back(subframe == 1);
						continue;
					}

					// Backward-compatible fallback for the authored asset typo:
					// Cutscene_daychange_2,3.png
					if (fs::exists(commaPath)) {
						outFrames.push_back(commaPath);
						outFlags.push_back(subframe == 1);
						continue;
					}

					// No frame found for ".1" means the sequence is over.
					if (subframe == 1) {
						return;
					}

					// Later subframe missing: move on to the next chapter.
					break;
				}
			}
		}

		// Helper to stop/fade gameplay audio before cutscene
		static void StopAllGameplayAudio(Scene& scene, float bgmFadeSeconds) {
			if (!scene.ShouldUseRuntimeParityMode()) {
				return;
			}

			if (AudioManager* audioMgr = scene.GetAudioManager()) {
				// Fade level BGM and ambience instead of hard stop.
				audioMgr->FadeChannel("bgm_MyoonchiDiner_LevelTheme", 0.0f, bgmFadeSeconds);
				audioMgr->FadeChannel("bgm_KitchenAmbience", 0.0f, bgmFadeSeconds);
				audioMgr->FadeChannel("bgm_forest_ambience", 0.0f, bgmFadeSeconds);

				// Stop all processing sounds (work table sounds)
				audioMgr->StopSound("sfx_chopping");
				audioMgr->StopSound("sfx_grilling_sizzle");
				audioMgr->StopSound("sfx_boiling_sound");

				// Stop any other looping gameplay sounds
				scene.StopAllObjectAudio();

				TS_LOG_DEBUG("[Economy] Fading gameplay BGM for win/lose cutscene");
			}
		}

		static void BuildLoseFramesExact3(std::vector<std::string>& outFrames,
			std::vector<bool>& outFlags,
			const std::string& baseFolder,
			const std::string& prefix) {
			namespace fs = std::filesystem;

			outFrames.clear();
			outFlags.clear();

			for (int i = 1; i <= 3; ++i) {
				const std::string p = baseFolder + "/" + prefix + std::to_string(i) + ".png";
				if (!fs::exists(p)) {
					break;
				}

				outFrames.push_back(p);
				outFlags.push_back(true); // fade between frames
			}
		}
	}

	void ResetHudPresentation() {
		gHudPresentation = HudPresentationState{};
	}

	void TriggerMoneyTextPulse(Scene& scene) {
		EnsureHudPresentationBindings(scene);
		gHudPresentation.moneyPulseRemaining = std::max(gHudPresentation.moneyPulseRemaining, kMoneyPulseDuration);
	}

	void UpdateHudPresentation(float dt, Scene& scene) {
		if (!scene.ShouldUseRuntimeParityMode()) {
			return;
		}

		EnsureHudPresentationBindings(scene);
		gHudPresentation.animationClock += std::max(0.0f, dt);

		const int wholeSeconds = GetDisplayedWholeSeconds();
		if (wholeSeconds != gHudPresentation.lastWholeSeconds) {
			if (gHudPresentation.lastWholeSeconds > 0 &&
				wholeSeconds > 0 &&
				wholeSeconds < gHudPresentation.lastWholeSeconds &&
				(wholeSeconds % 60) == 0) {
				gHudPresentation.minuteAlertRemaining = kMinuteAlertDuration;
			}

			gHudPresentation.lastWholeSeconds = wholeSeconds;
		}

		gHudPresentation.minuteAlertRemaining =
			std::max(0.0f, gHudPresentation.minuteAlertRemaining - std::max(0.0f, dt));
		gHudPresentation.moneyPulseRemaining =
			std::max(0.0f, gHudPresentation.moneyPulseRemaining - std::max(0.0f, dt));

		const float minutePulse = ComputeDampedPulse(gHudPresentation.minuteAlertRemaining, kMinuteAlertDuration, 5.5f);
		const glm::vec2 minuteOffset = ComputeMinuteAlertOffset(gHudPresentation.minuteAlertRemaining);
		const glm::vec2 countdownOffset = ComputeFinalCountdownOffset(gHudPresentation.animationClock, wholeSeconds);
		const float timerScaleMultiplier =
			1.0f +
			(0.34f * minutePulse) +
			ComputeFinalCountdownScaleBoost(gHudPresentation.animationClock, wholeSeconds);
		const glm::vec2 timerOffset = minuteOffset + countdownOffset;

		if (gHudPresentation.timerText.captured) {
			scene.SetRuntimeTextScaleByName("TimerText", gHudPresentation.timerText.scale * timerScaleMultiplier);
			scene.SetRuntimeTextPositionByName(
				"TimerText",
				gHudPresentation.timerText.x + timerOffset.x,
				gHudPresentation.timerText.y + timerOffset.y);
			if (wholeSeconds > 0 && wholeSeconds <= 10) {
				scene.SetRuntimeTextColorByName(
					"TimerText",
					kTimerTextAlertRedR,
					kTimerTextAlertRedG,
					kTimerTextAlertRedB,
					gHudPresentation.timerText.colorA);
			}
			else {
				scene.SetRuntimeTextColorByName(
					"TimerText",
					gHudPresentation.timerText.colorR,
					gHudPresentation.timerText.colorG,
					gHudPresentation.timerText.colorB,
					gHudPresentation.timerText.colorA);
			}
		}

		const float moneyPulse = ComputeDampedPulse(gHudPresentation.moneyPulseRemaining, kMoneyPulseDuration, 4.0f);
		const float moneyScaleMultiplier = 1.0f + (0.55f * moneyPulse);
		if (gHudPresentation.moneyText.captured) {
			scene.SetRuntimeTextScaleByName("MoneyText", gHudPresentation.moneyText.scale * moneyScaleMultiplier);
			scene.SetRuntimeTextPositionByName("MoneyText", gHudPresentation.moneyText.x, gHudPresentation.moneyText.y);
		}

		if (gPlayerMoney >= kQuota) {
			scene.SetRuntimeTextColorByName("MoneyText", kMoneyTextGreenR, kMoneyTextGreenG, kMoneyTextGreenB, 1.0f);
		}
		else {
			scene.SetRuntimeTextColorByName("MoneyText", kMoneyTextRedR, kMoneyTextRedG, kMoneyTextRedB, 1.0f);
		}
	}

	void OnQuotaReached(Scene& scene) {
		if (gTimerPaused) return;
		gTimerPaused = true;
		gAwaitingFinalCustomerClear = false;

		StopAllGameplayAudio(scene, 2.0f);

		std::vector<std::string> frames;
		std::vector<bool> boundaries;

		BuildSequentialFramesAndBoundaries(
			frames, boundaries,
			"../assets/Win",
			"Cutscene_daychange_"
		);

		if (frames.empty()) {
			scene.RequestMainMenuStateChange();
			return;
		}

		const std::string levelPath = scene.GetCurrentLevelPath();
		const bool isLevel1 = levelPath.find("kitchen01") != std::string::npos;
		const bool isLevel2 = levelPath.find("kitchen02") != std::string::npos;

		if (isLevel2) {
			gWinScreenNextGoesToMainMenu = true;
		}
		else if (isLevel1) {
			gWinScreenNextGoesToMainMenu = false;
		}
		else {
			gWinScreenNextGoesToMainMenu = false;
		}

		const char* nextScenePath = FilePaths::Levels::WIN;

		scene.StartCutsceneTransitionedBounded(
			frames,
			boundaries,
			nextScenePath,
			true,
            1.0f,
			0.175f,
			0.75f,
			-1,
			0.0f
		);
	}

	void OnTimeUp(Scene& scene) {
		if (gTimerPaused) return;
		gTimerPaused = true;

		// Stop all gameplay audio immediately
		StopAllGameplayAudio(scene, 0.35f);

		if (scene.ShouldUseRuntimeParityMode()) {
			// Play game over sound effect at 50% volume
			if (AudioManager* audioMgr = scene.GetAudioManager()) {
				if (audioMgr->HasSound("sfx_game_over")) {
					audioMgr->PlaySound("sfx_game_over", audioMgr->GetVfxVolume() * 1.0f, false);
					TS_LOG_DEBUG("[Economy] Playing game over sound effect at 50% volume");
				}
			}
		}

		std::vector<std::string> frames;
		std::vector<bool> boundaries;

		// One visual frame per step (no chapter triplication)
		BuildLoseFramesExact3(
			frames, boundaries,
			"../assets/Lose",
			"Cutscene_gameover"
		);

		if (frames.empty()) {
			scene.RequestMainMenuStateChange();
			return;
		}

		const std::string levelPath = scene.GetCurrentLevelPath();
		const bool isLevel1 = levelPath.find("kitchen01") != std::string::npos;
		const bool isLevel2 = levelPath.find("kitchen02") != std::string::npos;
		const char* nextScenePath = (isLevel1 || isLevel2) ? FilePaths::Levels::LOSE : FilePaths::Levels::MAIN_MENU;

		scene.StartCutsceneTransitionedBounded(
			frames,
			boundaries,
			nextScenePath,
			false,
			0.35f,
			0.35f,
			1.5f,
			-1,
			0.0f
		);
	}
}
