/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameStateManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Darren Toh, darren.toh@digipen.edu

 DESCRIPTION:		Game State Manager interface derived from System.hpp. Uses 3 Function pointers
					and redirects them to level/scene-specific init, update and exit functions.
					These function pointers are then called in main by the game state manager.
					This is a header file for declarations.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <functional> 
#include <memory>
#include <unordered_map>
#include <string>

#include "MessageBus.hpp"
#include "System.hpp"
#include "TestLevel.hpp"
#include "TestLevel2.hpp"

class Scene; // forward-declare Scene
class AudioManager; // forward-declare AudioManager

namespace Framework {
	enum GameState {
		GS_Level1,
		GS_Level2,
		GS_Quit
	};

	extern int currentGS, nextGS;
	extern bool init;

	typedef std::function<void(float dt)> FP;

	extern FP fpInit, fpUpdate, fpExit;

	class GameStateManager : public CoreFramework::SystemInterface {
	public:
		GameStateManager(CoreFramework::MessageBus& bus);
		~GameStateManager();

		void Initialize() override;
		void Update(float dt) override;
		std::string GetName() override;
		void InitializeGameState(int GS, float dt);
		void UpdateGameState(int newState, float dt);

		// Inject Scene used for runtime level loading
		void SetScene(Scene* s) {
			scene = s;
		}

		// Map a GameState to a JSON level path
		void RegisterJsonState(int state, const std::string& levelPath) {
			jsonStatePaths[state] = levelPath;
		}

		// Inject AudioManager for state-based audio control
		void SetAudioManager(AudioManager* mgr) {
			audioManager = mgr;
		}

	private:
		void OnQuit(const CoreFramework::Message& msg);

		// Switch to a JSON-backed state if mapping exists; returns true if handled
		bool TrySwitchJsonState(int state, float dt);

	private:
		CoreFramework::MessageBus& messageBus;
		CoreFramework::SubscriberId quitSubId;

		Scene* scene = nullptr;
		std::unordered_map<int, std::string> jsonStatePaths;
		bool pendingSimActivation = false; // NEW

		// Audio management
		AudioManager* audioManager = nullptr;
		std::string currentAudio; // Track currently playing background music
		bool wasPaused = false;    // Track pause state for audio

		void StopCurrentAudio();
	};
}
