/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         Quota.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu   (90%)
 CO-AUTHOR:         Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:       Defines the Economy namespace, which tracks player money,
					win quota, and remaining time, and synchronizes these values
					with the game UI and win/lose conditions.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "LevelEditorPanelFonts.hpp"

#include <iomanip>
#include <sstream>

class Scene;
class AudioManager;

namespace Economy {
	// Global money the player currently has
	inline int gPlayerMoney = 0;

	// Win quota
	inline constexpr int kQuota = 200;

	// How much a correct dish pays (tweak anytime)
	inline constexpr int kCorrectDishPay = 50;

	// Prevent triggering win multiple times
	inline bool gQuotaReached = false;

	// Total time allowed (seconds). Adjust as you like.
	inline constexpr float kTimeLimitSeconds = 180.0f;

	// Remaining time (seconds)
	inline float gTimeRemaining = kTimeLimitSeconds;

	// Prevent triggering lose multiple times
	inline bool gTimeUp = false;

	// Optional: pause timer (e.g., in win/lose screen)
	inline bool gTimerPaused = false;

	// Track which time-based sounds have been played
	inline bool gPlayed10SecWarning = false;
	inline bool gPlayed3SecBeep = false;
	inline bool gPlayed2SecBeep = false;
	inline bool gPlayed1SecBeep = false;
	inline bool gPlayedTimeUp = false;

	// Put near the top of Economy.hpp/cpp (where Economy lives)
	inline void SyncUI() {
		// Money
		LEPANELFONTS::SetTextByName("MoneyText", "$" + std::to_string(gPlayerMoney));

		// Quota: "current / target"
		LEPANELFONTS::SetTextByName("QuotaText", "$" + std::to_string(kQuota));

		// Timer: format mm:ss
		int total = static_cast<int>(gTimeRemaining + 0.999f); // ceil-ish
		int mm = total / 60;
		int ss = total % 60;

		std::ostringstream oss;
		oss << std::setw(2) << std::setfill('0') << mm
			<< ":" << std::setw(2) << std::setfill('0') << ss;

		LEPANELFONTS::SetTextByName("TimerText", oss.str());
	}

	// Call this to reset all economy values to their initial state (e.g., at level start or retry)
	inline void Reset() {
		gPlayerMoney = 0;
		gQuotaReached = false;

		gTimeRemaining = kTimeLimitSeconds;
		gTimeUp = false;
		gTimerPaused = false;

		// Reset sound effect flags
		gPlayed10SecWarning = false;
		gPlayed3SecBeep = false;
		gPlayed2SecBeep = false;
		gPlayed1SecBeep = false;
		gPlayedTimeUp = false;

		SyncUI();
	}

	// Callbacks to trigger when quota is reached or time is up. Implement these in SceneManager.cpp to show win/lose screens.
	void OnQuotaReached(Scene& scene);
	void OnTimeUp(Scene& scene);

	// Call this to add money when a dish is served. It updates the UI immediately and checks for win condition.
	inline void AddMoney(Scene& scene, int amount) {
		if (amount <= 0)
			return;

		gPlayerMoney += amount;

		SyncUI(); // <--- update UI immediately

		if (!gQuotaReached && gPlayerMoney >= kQuota) {
			gQuotaReached = true;
			OnQuotaReached(scene);
		}
	}

	// Call this every frame with the delta time to update the timer. It checks for time-up condition and updates the UI.
	inline void Update(float dt, Scene& scene) {
		if (gQuotaReached) return;
		if (gTimeUp) return;
		if (gTimerPaused) return;
		if (dt <= 0.0f) return;

		gTimeRemaining -= dt;

		if (gTimeRemaining <= 0.0f) {
			gTimeRemaining = 0.0f;
			gTimeUp = true;
			OnTimeUp(scene);
		}

		SyncUI(); // <--- update timer every frame (and quota/money too)
	}

	// Helpers (optional)
	inline float GetTimeRemaining() {
		return gTimeRemaining;
	}
}
