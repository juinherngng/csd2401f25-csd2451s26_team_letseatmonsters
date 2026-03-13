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
		// Supports BOTH patterns:
		//   A) baseFolder/prefix + chapter + "." + frame + ".png"   (e.g. Cutscene_gameover1.1.png)
		//   B) baseFolder/prefix + chapter + ".png"                  (e.g. Cutscene_gameover1.png)
		static std::array<std::vector<std::string>, 6>
			CollectChapterFrames(const std::string& baseFolder, const std::string& prefix) {
			namespace fs = std::filesystem;

			std::array<std::vector<std::string>, 6> chapters{};
			for (int ch = 1; ch <= 6; ++ch) {
				auto& list = chapters[ch - 1];
				bool any = false;

				// Try multi-frame: prefix + chapter + "." + frame
				for (int f = 1; f <= 300; ++f) {
					std::string path =
						baseFolder + "/" + prefix + std::to_string(ch) + "." + std::to_string(f) + ".png";

					if (fs::exists(path)) {
						list.push_back(path);
						any = true;
					}
					else {
						break;
					}
				}

				// Fallback single image: prefix + chapter
				if (!any) {
					std::string fallback =
						baseFolder + "/" + prefix + std::to_string(ch) + ".png";
					if (fs::exists(fallback)) list.push_back(fallback);
				}
			}
			return chapters;
		}

		// Use only the first frame of each chapter (no subchapter looping) and mark each as a boundary.
		static void BuildChapterFirstFramesAndBoundaries(std::vector<std::string>& outFrames,
			std::vector<bool>& outFlags,
			const std::string& baseFolder,
			const std::string& prefix) {
			outFrames.clear();
			outFlags.clear();

			const auto chapters = CollectChapterFrames(baseFolder, prefix);
			for (int ch = 1; ch <= 6; ++ch) {
				const auto& raw = chapters[ch - 1];
				if (raw.empty()) continue;

				outFrames.push_back(raw.front()); // lock to chapter first frame only
				outFlags.push_back(true);         // chapter boundary
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
	}

	void OnQuotaReached(Scene& scene) {
		if (gTimerPaused) return;
		gTimerPaused = true;

		// Stop all gameplay audio immediately
		StopAllGameplayAudio(scene, 2.0f);

		// Note: bgm_win_cutscene will be started by Scene after the initial fade-in completes

		std::vector<std::string> frames;
		std::vector<bool> boundaries;

		// No looping subchapters: first frame per chapter only.
		BuildChapterFirstFramesAndBoundaries(
			frames, boundaries,
			"../assets/Win",
			"Cutscene_daychange_"
		);

		// If no frames, go straight to MAIN MENU
		if (frames.empty()) {
			scene.RequestStateChange(0); // GS_Level1 = main menu
			return;
		}

		const std::string levelPath = scene.GetCurrentLevelPath();
		const bool isLevel1 = levelPath.find("kitchen01") != std::string::npos;
		const char* nextScenePath = isLevel1 ? FilePaths::Levels::WIN : FilePaths::Levels::KITCHEN_02;

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

		// Same behavior as win: first frame per chapter only, no subchapter loop.
		BuildChapterFirstFramesAndBoundaries(
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
			1.0f,
			-1,
			0.0f
		);
	}
}