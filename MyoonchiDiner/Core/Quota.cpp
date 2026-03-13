/*
----------------------------------------------------------------------------------------------------
 FILE NAME:         Quota.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu   (80%)
 CO-AUTHOR:         Ng Juin Herng, juinherng.ng@digipen.edu (20%)

 DESCRIPTION:       Defines the Economy namespace, which tracks player money,
					win quota, and remaining time, and synchronizes these values
					with the game UI and win/lose conditions.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "Core/AudioManager.hpp"
#include "Core/Quota.hpp"
#include "Graphics/SceneManager.hpp"

#include "FilePaths.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

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

				std::cout << "[Economy] Fading gameplay BGM for win/lose cutscene" << std::endl;
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

		// Stop all gameplay audio immediately
		StopAllGameplayAudio(scene, 2.0f);

		// Note: bgm_win_cutscene will be started by Scene after the initial fade-in completes

		std::vector<std::string> frames;
		std::vector<bool> boundaries;

		// If no frames, go straight to MAIN MENU
		if (frames.empty()) {
			scene.RequestStateChange(0); // GS_Level1 = main menu
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
			// Default: keep existing day-clear button flow pointing at level 2.
			gWinScreenNextGoesToMainMenu = false;
		}

		const char* nextScenePath = FilePaths::Levels::WIN;

		scene.StartCutsceneTransitionedBounded(
			frames,
			boundaries,
			nextScenePath,
			true,
			2.0f,   // fadeOutSeconds - 2 second fade out for win cutscene
			0.35f,  // fadeInSeconds
			1.5f, // hold each chapter image steadily (no per-subframe cadence)
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
				std::cout << "[Economy] Playing game over sound effect at 50% volume" << std::endl;
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
			scene.RequestStateChange(0); // GS_Level1 = main menu
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