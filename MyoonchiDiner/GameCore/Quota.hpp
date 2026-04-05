/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         Quota.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu   (60%)
 CO-AUTHORS:        Ng Juin Herng, juinherng.ng@digipen.edu (10%)
					Yat Chun Wee, y.chunwee@digipen.edu		(30%)

 DESCRIPTION:       Defines the Economy namespace, which tracks player money,
					win quota, and remaining time, and synchronizes these values
					with the game UI and win/lose conditions.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <iomanip>
#include <sstream>

#include "EngineGraphics/SceneManager.hpp"

class AudioManager;

namespace Economy {
	// Global money the player currently has
	inline int gPlayerMoney = 0;

	// Win quota
	inline int kQuota = 200;

	// How much a correct dish pays (tweak anytime)
	inline constexpr int kCorrectDishPay = 50;

	// Prevent triggering win multiple times
	inline bool gQuotaReached = false;

	// Total time allowed (seconds). Adjust as you like.
	inline float kTimeLimitSeconds = 180.0f;

	inline bool gAwaitingFinalCustomerClear = false;
	inline float kStopSpawningThresholdSeconds = 10.0f;


	/**
	 * @brief Sets the win quota for the current round.
	 * @param quota Target amount of money required to reach the quota.
	 */
	inline void SetQuota(int quota) {
		// Update the live quota target so later UI syncs and win checks use the new value.
		kQuota = quota;
	}

	/**
	 * @brief Sets the round time limit in seconds.
	 * @param seconds Total time available for the round.
	 */
	inline void SetTimeLimitSeconds(float seconds) {
		// Store the authored time limit used by Reset() when a new round begins.
		kTimeLimitSeconds = seconds;
	}

	// Remaining time (seconds)
	inline float gTimeRemaining = kTimeLimitSeconds;

	// Prevent triggering lose multiple times
	inline bool gTimeUp = false;

	// Optional: pause timer (e.g., in win/lose screen)
	inline bool gTimerPaused = false;

	// Determines what "Next" on the win screen should do.
	// false: continue to level 2, true: return to main menu.
	inline bool gWinScreenNextGoesToMainMenu = false;
	inline Scene* gBoundUIScene = nullptr;

	// Track which time-based sounds have been played
	inline bool gPlayed10SecWarning = false;
	inline bool gPlayed3SecBeep = false;
	inline bool gPlayed2SecBeep = false;
	inline bool gPlayed1SecBeep = false;
	inline bool gPlayedTimeUp = false;

	void ResetHudPresentation();
	void UpdateHudPresentation(float dt, Scene& scene);
	void TriggerMoneyTextPulse(Scene& scene);

	/**
	 * @brief Binds the scene that owns the HUD text updated by the economy system.
	 * @param scene Scene whose runtime text objects should mirror economy state.
	 */
	inline void BindUIScene(Scene& scene) {
		// Remember the bound HUD scene and reset presentation state for the new scene.
		gBoundUIScene = &scene;
		ResetHudPresentation();
	}

	/**
	 * @brief Pushes current money, quota, and timer values onto the bound HUD scene.
	 * @param scene Optional scene override that should become the new bound HUD scene.
	 */
	inline void SyncUI(Scene* scene = nullptr) {
		// Update the bound scene first when callers provide an explicit scene pointer.
		if (scene != nullptr) {
			gBoundUIScene = scene;
		}

		if (gBoundUIScene == nullptr) {
			return;
		}

		// Refresh the live money display immediately.
		gBoundUIScene->SetRuntimeTextByName("MoneyText", "$" + std::to_string(gPlayerMoney));

		// Refresh the split quota-card text so the label and value can be styled independently.
		gBoundUIScene->SetRuntimeTextByName("QuotaText", "");
		gBoundUIScene->SetRuntimeTextByName("QuotaLabelText", "TODAY'S GOAL");
		gBoundUIScene->SetRuntimeTextByName("QuotaValueText", "$" + std::to_string(kQuota));

		// Format the remaining time as `mm:ss` using ceil-like behavior for the display.
		int total = static_cast<int>(gTimeRemaining + 0.999f);
		int mm = total / 60;
		int ss = total % 60;

		std::ostringstream oss;
		oss << std::setw(2) << std::setfill('0') << mm
			<< ":" << std::setw(2) << std::setfill('0') << ss;

		gBoundUIScene->SetRuntimeTextByName("TimerText", oss.str());
	}

	/**
	 * @brief Resets all economy and timer state back to the round defaults.
	 */
	inline void Reset() {
		// Restore all round-progress state so retries and new levels start cleanly.
		gPlayerMoney = 0;
		gQuotaReached = false;

		gTimeRemaining = kTimeLimitSeconds;
		gTimeUp = false;
		gTimerPaused = false;
		gAwaitingFinalCustomerClear = false;

		// Reset all one-shot HUD and countdown sound flags for the new round.
		gPlayed10SecWarning = false;
		gPlayed3SecBeep = false;
		gPlayed2SecBeep = false;
		gPlayed1SecBeep = false;
		gPlayedTimeUp = false;
		ResetHudPresentation();

		// Push the freshly reset values back into the HUD immediately.
		SyncUI();
	}

	/**
	 * @brief Handles the win flow after the round quota has been reached.
	 * @param scene Active scene used to trigger the win cutscene and transition.
	 */
	void OnQuotaReached(Scene& scene);

	/**
	 * @brief Handles the lose flow after time has expired.
	 * @param scene Active scene used to trigger the lose cutscene and transition.
	 */
	void OnTimeUp(Scene& scene);

	/**
	 * @brief Adds money to the player's total and refreshes the HUD.
	 * @param scene Active scene whose HUD should be updated.
	 * @param amount Money to add.
	 */
	inline void AddMoney(Scene& scene, int amount) {
		// Ignore zero or negative changes because this helper only models income.
		if (amount <= 0)
			return;

		gPlayerMoney += amount;

		// Mark quota completion once the player's money crosses the configured target.
		if (!gQuotaReached && gPlayerMoney >= kQuota) {
			gQuotaReached = true;
		}

		// Refresh the HUD and trigger the money pulse as soon as income is awarded.
		SyncUI(&scene);
		TriggerMoneyTextPulse(scene);
	}

	/**
	 * @brief Advances the round timer and updates the HUD.
	 * @param dt Delta time for the current frame.
	 * @param scene Active scene whose HUD should be updated.
	 */
	inline void Update(float dt, Scene& scene) {
		// Timer updates stop once time is up, the timer is paused, or the frame delta is invalid.
		if (gTimeUp) return;
		if (gTimerPaused) return;
		if (dt <= 0.0f) return;

		gTimeRemaining -= dt;

		if (gTimeRemaining <= 0.0f) {
			gTimeRemaining = 0.0f;
			gTimeUp = true;

			// Defer the final outcome until all active customer flows have resolved cleanly.
			gAwaitingFinalCustomerClear = true;
		}

		// Refresh the visible timer every frame alongside the current money and quota values.
		SyncUI(&scene);
	}

	/**
	 * @brief Returns the current remaining round time.
	 * @return Remaining time in seconds.
	 */
	inline float GetTimeRemaining() {
		// Expose the live timer value for gameplay and HUD systems that need it.
		return gTimeRemaining;
	}
}
