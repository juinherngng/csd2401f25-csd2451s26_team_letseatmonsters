/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameStateManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Darren Toh, darren.toh@digipen.edu		(33.33%)
 CO-AUTHORS:		Seah Wang Hua, wanghua.seah@digipen.edu (33.33%)
					Ng Juin Herng, juinherng.ng@digipen.edu (33.33%)

 DESCRIPTION:		This file defines the GameState enumeration and the GameStateManager system responsible for
					controlling the game’s high-level state machine (e.g. main menu, gameplay, quit). It declares
					the public API for initializing and updating game states, registering JSON-backed levels, and
					injecting engine services such as the Scene and AudioManager used during state transitions.

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
		using StateAudioPolicy = std::function<void(int, Scene&, AudioManager*)>;
		using PauseAudioPolicy = std::function<void(bool, bool, int, Scene&, AudioManager*)>;

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

		// Inject game-specific state/audio policies
		void SetStateAudioPolicy(StateAudioPolicy policy) {
			stateAudioPolicy = std::move(policy);
		}
		void SetPauseAudioPolicy(PauseAudioPolicy policy) {
			pauseAudioPolicy = std::move(policy);
		}

	private:
		void OnQuit(const CoreFramework::Message& msg);

		// Switch to a JSON-backed state if mapping exists; returns true if handled
		bool TrySwitchJsonState(int state, float dt);
		void PreloadJsonStateAssets(int activeState);

	private:
		CoreFramework::MessageBus& messageBus;
		CoreFramework::SubscriberId quitSubId;

		Scene* scene = nullptr;
		std::unordered_map<int, std::string> jsonStatePaths;
		bool pendingSimActivation = false; // NEW

		// Audio service
		AudioManager* audioManager = nullptr;
		bool wasPaused = false;    // Track pause state for audio
		StateAudioPolicy stateAudioPolicy;
		PauseAudioPolicy pauseAudioPolicy;
	};
}
