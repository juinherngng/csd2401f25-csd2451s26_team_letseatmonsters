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
		// Collect one frame per sequential index:
		// tries .../prefix1.png then .../prefix1.1.png, then 2, 3, ...
		static void BuildSequentialFramesAndBoundaries(std::vector<std::string>& outFrames,
			std::vector<bool>& outFlags,
			const std::string& baseFolder,
			const std::string& prefix) {
			namespace fs = std::filesystem;

			outFrames.clear();
			outFlags.clear();

			for (int i = 1; i <= 3; ++i) {
				const std::string pDirect = baseFolder + "/" + prefix + std::to_string(i) + ".png";
				const std::string pDotOne = baseFolder + "/" + prefix + std::to_string(i) + ".1.png";

				if (fs::exists(pDirect)) {
					outFrames.push_back(pDirect);
					outFlags.push_back(true);
					continue;
				}

				if (fs::exists(pDotOne)) {
					outFrames.push_back(pDotOne);
					outFlags.push_back(true);
					continue;
				}

				break;
			}
		}

		// Helper to stop/fade gameplay audio before cutscene
		static void StopAllGameplayAudio(Scene& scene, float bgmFadeSeconds) {
#ifndef _DEBUG
			if (AudioManager* audioMgr = scene.GetAudioManager()) {
				// Fade level BGM and ambience instead of hard stop.
				audioMgr->FadeChannel("bgm_MyoonchiDiner_LevelTheme", 0.0f, bgmFadeSeconds);
				audioMgr->FadeChannel("bgm_KitchenAmbience", 0.0f, bgmFadeSeconds);

				// Stop all processing sounds (work table sounds)
				audioMgr->StopSound("sfx_chopping");
				audioMgr->StopSound("sfx_grill");
				audioMgr->StopSound("sfx_boiling_sound");

				// Stop any other looping gameplay sounds
				scene.StopAllObjectAudio();

				TS_LOG_DEBUG("[Economy] Fading gameplay BGM for win/lose cutscene");
			}
#endif
#ifdef _DEBUG
			(void)scene;
			(void)bgmFadeSeconds;
#endif
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
			2.0f,
			0.35f,
			1.5f,
			-1,
			0.0f
		);
	}

	void OnTimeUp(Scene& scene) {
		if (gTimerPaused) return;
		gTimerPaused = true;

		// Stop all gameplay audio immediately
		StopAllGameplayAudio(scene, 0.35f);

#ifndef _DEBUG
		// Play game over sound effect at 50% volume
		if (AudioManager* audioMgr = scene.GetAudioManager()) {
			if (audioMgr->HasSound("sfx_game_over")) {
				audioMgr->PlaySound("sfx_game_over", audioMgr->GetVfxVolume() * 1.0f, false);
				TS_LOG_DEBUG("[Economy] Playing game over sound effect at 50% volume");
			}
		}
#endif

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
		const char* nextScenePath = isLevel1 ? FilePaths::Levels::LOSE : FilePaths::Levels::MAIN_MENU;

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
