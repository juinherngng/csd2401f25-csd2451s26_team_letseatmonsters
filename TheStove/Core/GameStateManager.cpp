/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameStateManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Darren Toh, darren.toh@digipen.edu

 DESCRIPTION:		Game State Manager interface derived from System.hpp. Uses 3 Function pointers
					and redirects them to level/scene-specific init, update and exit functions.
					These function pointers are then called in main by the game state manager.
					This is a header file for definitions.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "GameStateManager.hpp"
#include "../Graphics/SceneManager.hpp"
#include "RuntimeLevel.hpp"
#include "AudioManager.hpp"

namespace Framework {

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
		} else {
			pendingSimActivation = true;        // other states (e.g., gameplay)
		}

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
				// Gameplay level state - play level music
				currentAudio = "bgm_MyoonchiDiner_LevelTheme";
				audioManager->PlaySound(currentAudio, audioManager->GetBgmVolume(), false);
				std::cout << "[GameStateManager] Playing level theme music" << std::endl;
			}
			else {
				currentAudio.clear();
			}
		}

		fpInit = nullptr;
		fpUpdate = nullptr;
		fpExit = nullptr;
		return true;
	}

	void GameStateManager::StopCurrentAudio() {
		if (audioManager && !currentAudio.empty()) {
			audioManager->StopSound(currentAudio);
			currentAudio.clear();
		}
	}
}