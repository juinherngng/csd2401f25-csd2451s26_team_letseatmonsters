/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameStateManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Darren Toh, darren.toh@digipen.edu		(20%)
 CO-AUTHORS:		Seah Wang Hua, wanghua.seah@digipen.edu (40%)
					Ng Juin Herng, juinherng.ng@digipen.edu (40%)

 DESCRIPTION:		This file implements the logic for first-time initialization, per-frame updates, and transitions
					between states, preferring JSON-driven runtime level loading when mappings are registered, with a
					fallback to legacy function-pointer-based level init/update/exit routines. Manages scene simulation
					activation timing, tracks pause state to pause/resume audio in gameplay, and controls state-based
					playback and cleanup of background music and ambience through the injected AudioManager instance.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"

#include "AudioManager.hpp"
#include "GameStateManager.hpp"
#include "LevelSerializer.hpp"
#include "RuntimeLevel.hpp"

#include <unordered_set>
#include <vector>

namespace Framework {

	namespace {
		void AppendLevelTextures(const std::string& levelPath, std::vector<std::string>& inOutPaths, std::unordered_set<std::string>& seen) {
			LevelData levelData;
			if (!LevelSerializer::Load(levelPath, levelData)) {
				return;
			}

			if (!levelData.background.empty() && seen.insert(levelData.background).second) {
				inOutPaths.push_back(levelData.background);
			}

			for (const LevelObject& object : levelData.objects) {
				if (!object.texture.empty() && seen.insert(object.texture).second) {
					inOutPaths.push_back(object.texture);
				}
			}
		}
	}

	extern int currentGS = 0, nextGS = 0;
	extern bool init = false;

	typedef std::function<void(float dt)> FP;
	extern FP fpInit = nullptr, fpUpdate = nullptr, fpExit = nullptr;

	GameStateManager::GameStateManager(CoreFramework::MessageBus& bus)
		: messageBus(bus) {
		quitSubId = messageBus.Subscribe(
			CoreFramework::MessageType::QUIT,
			[this](const CoreFramework::Message& msg) { OnQuit(msg); }
		);
	}

	GameStateManager::~GameStateManager() {
		messageBus.Unsubscribe(CoreFramework::MessageType::QUIT, quitSubId);
	}

	void GameStateManager::Initialize() {
		std::cout << "GameStateManager system initialized." << std::endl;
	}

	void GameStateManager::Update(float dt) {
		if (!init) {
			InitializeGameState(0, dt);
		}
		// Deferred simulation activation (after scene construction & first system frame)
		if (pendingSimActivation && scene) {
			// Only enable simulation if the scene has objects
			if (!scene->GetAllObjectsRaw().empty()) {
				scene->SetSimulationActive(true);
				pendingSimActivation = false;
			}
		}

		// Handle audio pause/resume based on simulation state
		if (audioManager && scene) {
			bool isPaused = !scene->IsSimulationActive();
			// Check if we just entered pause (level 2 only - gameplay level)
			if (currentGS == GS_Level2 && isPaused && !wasPaused && !currentAudio.empty()) {
				audioManager->PauseAll();
			}
			// Check if we just exited pause
			else if (currentGS == GS_Level2 && !isPaused && wasPaused && !currentAudio.empty()) {
				audioManager->ResumeAll();
			}
			wasPaused = isPaused;
		}

		if (currentGS == nextGS && fpUpdate) {
			fpUpdate(dt);
		}
	}

	void GameStateManager::OnQuit(const CoreFramework::Message& msg) {
		(void)msg;
		nextGS = GS_Quit;
	}

	std::string GameStateManager::GetName() {
		return "GameStateManager";
	}

	void GameStateManager::InitializeGameState(int GS, float dt) {
		nextGS = currentGS = GS;

		// Warm up target-state textures before switching (menu->level, level->cutscene).
		if (jsonStatePaths.find(GS) != jsonStatePaths.end()) {
			std::vector<std::string> targetTextures;
			std::unordered_set<std::string> seen;
			AppendLevelTextures(jsonStatePaths[GS], targetTextures, seen);
			if (!targetTextures.empty()) {
				ResourceManager::Instance().PreloadTextures(targetTextures);
			}
		}

		// Prefer JSON mapping if available
		if (TrySwitchJsonState(GS, dt)) {
			init = true;
			return;
		}

		// Fallback to legacy function-pointer state
		fpInit = Level1Init;
		fpUpdate = Level1Update;
		fpExit = Level1Exit;

		if (fpInit) {
			fpInit(dt);
		}
		init = true;
	}

	void GameStateManager::UpdateGameState(int newState, float dt) {
		nextGS = newState;

		// If we were using legacy function pointers, call exit
		if (fpExit) {
			fpExit(dt);
		}

		currentGS = newState;

		// Warm up textures for the destination state before scene build.
		if (jsonStatePaths.find(currentGS) != jsonStatePaths.end()) {
			std::vector<std::string> targetTextures;
			std::unordered_set<std::string> seen;
			AppendLevelTextures(jsonStatePaths[currentGS], targetTextures, seen);
			if (!targetTextures.empty()) {
				ResourceManager::Instance().PreloadTextures(targetTextures);
			}
		}

		// Prefer JSON mapping if available
		if (TrySwitchJsonState(currentGS, dt)) {
			return;
		}

		// Fallback to legacy hard-coded states
		switch (currentGS) {
		case GS_Level1:
			fpInit = Level1Init;
			fpUpdate = Level1Update;
			fpExit = Level1Exit;
			if (fpInit) fpInit(dt);
			break;

		case GS_Level2:
			fpInit = Level2Init;
			fpUpdate = Level2Update;
			fpExit = Level2Exit;
			if (fpInit) fpInit(dt);
			break;

		case GS_Quit:
			// No-op, let app quit
			break;
		}
	}

	bool GameStateManager::TrySwitchJsonState(int state, float dt) {
		(void)dt;
		auto it = jsonStatePaths.find(state);
		if (it == jsonStatePaths.end()) {
			return false;
		}
		if (!scene) {
			std::cerr << "[GameStateManager] Scene not set; cannot load JSON level for state " << state << std::endl;
			return false;
		}

		const std::string& path = it->second;
		if (!RuntimeLevel::LoadAndBuild(path, *scene)) {
			std::cerr << "[GameStateManager] Failed to build level from JSON: " << path << std::endl;
			return false;
		}

		// Auto-start simulation for gameplay only
		if (state == Framework::GS_Level1) {
			scene->SetSimulationActive(false);  // main menu stays paused
			pendingSimActivation = false;
		}
		else {
			pendingSimActivation = true;        // other states (e.g., gameplay)
		}

#ifndef _DEBUG
		// Handle state-based audio
		if (audioManager) {
			// Stop current audio before switching
			StopCurrentAudio();

			// Play appropriate audio for the new state
			if (state == Framework::GS_Level1) {
				// Main menu state - play menu music
				currentAudio = "bgm_MyoonchiDiner_MainMenu";
				audioManager->PlaySound(currentAudio, audioManager->GetBgmVolume(), false);
				std::cout << "[GameStateManager] Playing main menu music" << std::endl;
			}
			else if (state == Framework::GS_Level2) {
				// Gameplay level state - play level music with fade-in (synced with visual transition)
				currentAudio = "bgm_MyoonchiDiner_LevelTheme";
				// Start at volume 0 and fade in over 1 second to sync with visual fade-in
				audioManager->PlaySound(currentAudio, 0.0f, false);
				const float levelBgmFadeIn = 1.0f;
				audioManager->FadeChannel(currentAudio, audioManager->GetBgmVolume(), levelBgmFadeIn);
				std::cout << "[GameStateManager] Playing level theme music with fade-in" << std::endl;

				// Play kitchen ambience at 50% of BGM volume, also with fade-in
				currentAmbience = "bgm_KitchenAmbience";
				audioManager->PlaySound(currentAmbience, 0.0f, false);
				audioManager->FadeChannel(currentAmbience, audioManager->GetBgmVolume() * 0.5f, levelBgmFadeIn);
				std::cout << "[GameStateManager] Playing kitchen ambience with fade-in" << std::endl;
			}
			else {
				currentAudio.clear();
				currentAmbience.clear();
			}
		}
#endif

		PreloadJsonStateAssets(state);

		fpInit = nullptr;
		fpUpdate = nullptr;
		fpExit = nullptr;
		return true;
	}

	void GameStateManager::PreloadJsonStateAssets(int activeState) {
		std::vector<std::string> texturePaths;
		std::unordered_set<std::string> seen;

		for (const auto& [state, levelPath] : jsonStatePaths) {
			if (state == activeState) {
				continue;
			}

			AppendLevelTextures(levelPath, texturePaths, seen);
		}

		if (!texturePaths.empty()) {
			ResourceManager::Instance().PreloadTextures(texturePaths);
		}
	}

	void GameStateManager::StopCurrentAudio() {
		if (audioManager) {
			if (!currentAudio.empty()) {
				audioManager->StopSound(currentAudio);
				currentAudio.clear();
			}
			if (!currentAmbience.empty()) {
				audioManager->StopSound(currentAmbience);
				currentAmbience.clear();
			}
		}
	}
}