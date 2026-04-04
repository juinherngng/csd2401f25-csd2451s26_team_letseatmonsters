/*
----------------------------------------------------------------------------------------------------
 FILE NAME:         Quota.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu   (70%)
 CO-AUTHORS:        Ng Juin Herng, juinherng.ng@digipen.edu (10%)
					Yat Chun Wee, y.chunwee@digipen.edu		(20%)

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

		/**
		 * @brief Captures the authored baseline transform and color for a runtime text object.
		 * @param scene Active scene containing the runtime text objects.
		 * @param textName Name of the runtime text object to capture.
		 * @param outBase Output record populated with the captured base presentation state.
		 */
		static void CaptureRuntimeTextBase(const Scene& scene, const char* textName, HudTextBase& outBase) {
			// Skip empty text names so callers can guard optional HUD bindings cheaply.
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

		/**
		 * @brief Returns the whole-second timer value currently shown to the player.
		 * @return Ceil-like whole-second countdown value.
		 */
		static int GetDisplayedWholeSeconds() {
			// Use ceil-like rounding so the displayed countdown matches the HUD formatting in SyncUI().
			return std::max(0, static_cast<int>(gTimeRemaining + 0.999f));
		}

		/**
		 * @brief Ensures HUD presentation state is bound to the current scene and base text poses.
		 * @param scene Active scene whose HUD text should be animated.
		 */
		static void EnsureHudPresentationBindings(Scene& scene) {
			// Reset presentation state whenever the bound level changes so cached text poses do not leak.
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

		/**
		 * @brief Returns a damped pulse value for a time-limited HUD animation.
		 * @param remaining Remaining animation time.
		 * @param duration Total animation duration.
		 * @param cycles Number of half-cycles used by the pulse.
		 * @return Damped pulse magnitude in the range `[0, 1]`.
		 */
		static float ComputeDampedPulse(float remaining, float duration, float cycles) {
			// Return zero cleanly once the animation is no longer active.
			if (remaining <= 0.0f || duration <= 0.0f) {
				return 0.0f;
			}

			const float phase = 1.0f - (remaining / duration);
			const float envelope = 1.0f - phase;
			return std::abs(std::sin(phase * cycles * kPi)) * envelope;
		}

		/**
		 * @brief Computes the shake offset used for the minute-warning timer alert.
		 * @param remaining Remaining animation time for the minute alert.
		 * @return World-space text offset for the timer HUD.
		 */
		static glm::vec2 ComputeMinuteAlertOffset(float remaining) {
			// Stop offsetting the timer once the minute-alert animation has expired.
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

		/**
		 * @brief Computes the shake offset used during the final ten-second countdown.
		 * @param animationClock Running HUD animation clock.
		 * @param wholeSeconds Currently displayed whole-second countdown value.
		 * @return World-space text offset for the timer HUD.
		 */
		static glm::vec2 ComputeFinalCountdownOffset(float animationClock, int wholeSeconds) {
			// Only the final ten seconds get the aggressive countdown shake treatment.
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

		/**
		 * @brief Computes the scale boost used during the final ten-second countdown.
		 * @param animationClock Running HUD animation clock.
		 * @param wholeSeconds Currently displayed whole-second countdown value.
		 * @return Additional timer scale multiplier contribution.
		 */
		static float ComputeFinalCountdownScaleBoost(float animationClock, int wholeSeconds) {
			// Ignore the scale pulse unless the timer is inside the final warning window.
			if (wholeSeconds <= 0 || wholeSeconds > 10) {
				return 0.0f;
			}

			const float warning01 = std::clamp((10.0f - static_cast<float>(wholeSeconds)) / 9.0f, 0.0f, 1.0f);
			const float pulseWave = 0.25f + 0.75f * std::pow(
				0.5f + 0.5f * std::sin(animationClock * (9.0f + 2.0f * warning01) + 0.55f),
				1.85f);
			return (0.18f + 0.14f * warning01) * pulseWave;
		}

		/**
		 * @brief Builds a sequential cutscene frame list from authored image files.
		 * @param outFrames Output frame list populated from disk.
		 * @param outFlags Output boundary flags populated alongside the frame list.
		 * @param baseFolder Asset folder containing the cutscene images.
		 * @param prefix Shared filename prefix for the sequence.
		 */
		static void BuildSequentialFramesAndBoundaries(std::vector<std::string>& outFrames,
			std::vector<bool>& outFlags,
			const std::string& baseFolder,
			const std::string& prefix) {
			namespace fs = std::filesystem;

			// Rebuild the full frame list from scratch every time a cutscene is requested.
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

					// Preserve compatibility with the existing comma-separated authored typo variant.
					if (fs::exists(commaPath)) {
						outFrames.push_back(commaPath);
						outFlags.push_back(subframe == 1);
						continue;
					}

					// Missing the first subframe means there are no more chapters to load.
					if (subframe == 1) {
						return;
					}

					// Missing a later subframe simply ends the current chapter and advances to the next.
					break;
				}
			}
		}

		/**
		 * @brief Stops or fades gameplay audio before a win or lose cutscene begins.
		 * @param scene Active scene whose audio should be faded or stopped.
		 * @param bgmFadeSeconds Fade duration used for looping background channels.
		 */
		static void StopAllGameplayAudio(Scene& scene, float bgmFadeSeconds) {
			// Only runtime parity mode uses the authored cutscene audio transition behavior.
			if (!scene.ShouldUseRuntimeParityMode()) {
				return;
			}

			if (AudioManager* audioMgr = scene.GetAudioManager()) {
				// Fade looping ambience instead of hard-cutting it for a cleaner transition.
				audioMgr->FadeChannel("bgm_MyoonchiDiner_LevelTheme", 0.0f, bgmFadeSeconds);
				audioMgr->FadeChannel("bgm_KitchenAmbience", 0.0f, bgmFadeSeconds);
				audioMgr->FadeChannel("bgm_forest_ambience", 0.0f, bgmFadeSeconds);

				// Stop active station-processing loops that should not bleed into cutscenes.
				audioMgr->StopSound("sfx_chopping");
				audioMgr->StopSound("sfx_grilling_sizzle");
				audioMgr->StopSound("sfx_boiling_sound");

				// Stop any remaining object-bound audio emitters still active in the scene.
				scene.StopAllObjectAudio();

				TS_LOG_DEBUG("[Economy] Fading gameplay BGM for win/lose cutscene");
			}
		}

		/**
		 * @brief Builds the authored three-frame lose cutscene sequence.
		 * @param outFrames Output frame list populated from disk.
		 * @param outFlags Output boundary flags populated alongside the frame list.
		 * @param baseFolder Asset folder containing the lose cutscene images.
		 * @param prefix Shared filename prefix for the sequence.
		 */
		static void BuildLoseFramesExact3(std::vector<std::string>& outFrames,
			std::vector<bool>& outFlags,
			const std::string& baseFolder,
			const std::string& prefix) {
			namespace fs = std::filesystem;

			// Rebuild the exact three-step lose sequence from scratch each time.
			outFrames.clear();
			outFlags.clear();

			for (int i = 1; i <= 3; ++i) {
				const std::string p = baseFolder + "/" + prefix + std::to_string(i) + ".png";
				if (!fs::exists(p)) {
					break;
				}

				outFrames.push_back(p);
				outFlags.push_back(true);
			}
		}
	}

	/**
	 * @brief Resets cached HUD animation presentation state.
	 */
	void ResetHudPresentation() {
		// Drop the cached text bindings so the next update can recapture fresh scene data.
		gHudPresentation = HudPresentationState{};
	}

	/**
	 * @brief Triggers the money-text pulse animation on the HUD.
	 * @param scene Active scene containing the bound HUD text.
	 */
	void TriggerMoneyTextPulse(Scene& scene) {
		// Ensure the HUD bindings exist before extending the pulse timer.
		EnsureHudPresentationBindings(scene);
		gHudPresentation.moneyPulseRemaining = std::max(gHudPresentation.moneyPulseRemaining, kMoneyPulseDuration);
	}

	/**
	 * @brief Updates timer and money HUD presentation effects for one frame.
	 * @param dt Delta time for the frame.
	 * @param scene Active scene containing the HUD text.
	 */
	void UpdateHudPresentation(float dt, Scene& scene) {
		// HUD animation effects are only authored for runtime parity mode.
		if (!scene.ShouldUseRuntimeParityMode()) {
			return;
		}

		// Ensure the presentation cache is bound to the current scene before applying animation.
		EnsureHudPresentationBindings(scene);
		gHudPresentation.animationClock += std::max(0.0f, dt);

		const int wholeSeconds = GetDisplayedWholeSeconds();
		if (wholeSeconds != gHudPresentation.lastWholeSeconds) {
			// Start the minute alert when the timer crosses to an exact minute boundary while counting down.
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

		// Animate timer text position, scale, and alert color based on countdown state.
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

		// Animate money text with a short pulse whenever the player earns income.
		const float moneyPulse = ComputeDampedPulse(gHudPresentation.moneyPulseRemaining, kMoneyPulseDuration, 4.0f);
		const float moneyScaleMultiplier = 1.0f + (0.55f * moneyPulse);
		if (gHudPresentation.moneyText.captured) {
			scene.SetRuntimeTextScaleByName("MoneyText", gHudPresentation.moneyText.scale * moneyScaleMultiplier);
			scene.SetRuntimeTextPositionByName("MoneyText", gHudPresentation.moneyText.x, gHudPresentation.moneyText.y);
		}

		// Color the money total green when the quota is met and red otherwise.
		if (gPlayerMoney >= kQuota) {
			scene.SetRuntimeTextColorByName("MoneyText", kMoneyTextGreenR, kMoneyTextGreenG, kMoneyTextGreenB, 1.0f);
		}
		else {
			scene.SetRuntimeTextColorByName("MoneyText", kMoneyTextRedR, kMoneyTextRedG, kMoneyTextRedB, 1.0f);
		}
	}

	/**
	 * @brief Handles the win flow after the round quota has been reached.
	 * @param scene Active scene used to trigger the win cutscene and transition.
	 */
	void OnQuotaReached(Scene& scene) {
		// Ignore duplicate win triggers once the timer flow has already been paused.
		if (gTimerPaused) return;
		gTimerPaused = true;
		gAwaitingFinalCustomerClear = false;

		// Fade or stop gameplay audio before transitioning into the day-clear cutscene.
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

		// Decide where the win screen's "Next" action should route based on the current kitchen level.
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

		// Launch the authored day-clear cutscene sequence with chapter-boundary fades.
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

	/**
	 * @brief Handles the lose flow after time has expired.
	 * @param scene Active scene used to trigger the lose cutscene and transition.
	 */
	void OnTimeUp(Scene& scene) {
		// Ignore duplicate lose triggers once the timer flow has already been paused.
		if (gTimerPaused) return;
		gTimerPaused = true;

		// Fade or stop gameplay audio immediately before the game-over cutscene begins.
		StopAllGameplayAudio(scene, 0.35f);

		if (scene.ShouldUseRuntimeParityMode()) {
			// Play the authored game-over stinger once while entering the lose sequence.
			if (AudioManager* audioMgr = scene.GetAudioManager()) {
				if (audioMgr->HasSound("sfx_game_over")) {
					audioMgr->PlaySound("sfx_game_over", audioMgr->GetVfxVolume() * 1.0f, false);
					TS_LOG_DEBUG("[Economy] Playing game over sound effect at 50% volume");
				}
			}
		}

		std::vector<std::string> frames;
		std::vector<bool> boundaries;

		// Load the fixed three-frame lose cutscene sequence.
		BuildLoseFramesExact3(
			frames, boundaries,
			"../assets/Lose",
			"Cutscene_gameover"
		);

		if (frames.empty()) {
			scene.RequestMainMenuStateChange();
			return;
		}

		// Route kitchen levels to the lose screen and other contexts back to the main menu.
		const std::string levelPath = scene.GetCurrentLevelPath();
		const bool isLevel1 = levelPath.find("kitchen01") != std::string::npos;
		const bool isLevel2 = levelPath.find("kitchen02") != std::string::npos;
		const char* nextScenePath = (isLevel1 || isLevel2) ? FilePaths::Levels::LOSE : FilePaths::Levels::MAIN_MENU;

		// Launch the authored lose cutscene sequence and transition afterward.
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
