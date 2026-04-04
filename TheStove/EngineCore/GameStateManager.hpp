/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameStateManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Darren Toh, darren.toh@digipen.edu		(10%)
 CO-AUTHORS:		Seah Wang Hua, wanghua.seah@digipen.edu (15%)
					Ng Juin Herng, juinherng.ng@digipen.edu (30%)
					Yat Chun Wee, y.chunwee@digipen.edu		(45%)

 DESCRIPTION:		This file defines the GameState enumeration and the GameStateManager system responsible for
					controlling the games high-level state machine (e.g. main menu, gameplay, quit). It declares
					the public API for initializing and updating game states, registering JSON-backed levels, and
					injecting engine services such as the Scene and AudioManager used during state transitions.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "EngineCore/MessageBus.hpp"
#include "EngineCore/System.hpp"

class Scene; // forward-declare Scene
class AudioManager; // forward-declare AudioManager

namespace Framework {
	enum class GameState : int {
		MainMenu,
		Tutorial,
		Kitchen01,
		Quit
	};

	/**
	 * @brief Returns a stable display name for a game state identifier.
	 * @param state State identifier to stringify.
	 * @return Null-terminated name for logs and debugging.
	 */
	const char* ToString(GameState state);

	class GameStateManager : public CoreFramework::SystemInterface {
	public:
		/**
		 * @brief Constructs the game-state manager and subscribes to quit messages.
		 * @param bus Shared message bus used for system communication.
		 */
		GameStateManager(CoreFramework::MessageBus& bus);
		/** @brief Unsubscribes from message bus events and releases owned state. */
		~GameStateManager();

		/** @brief Initializes the state manager system. */
		void Initialize() override;
		/**
		 * @brief Updates state logic, deferred simulation activation, and pause audio policy.
		 * @param dt Frame delta time in seconds.
		 */
		void Update(float dt) override;
		/** @brief Return the system name used by the engine registry and debugger. */
		std::string GetName() override;
		/**
		 * @brief Initializes the first game state at application startup.
		 * @param GS State identifier to initialize.
		 * @param dt Frame delta time available during startup.
		 */
		void InitializeGameState(GameState state, float dt);
		/**
		 * @brief Switches from the current state to a new state.
		 * @param newState Destination state identifier.
		 * @param dt Frame delta time used during the transition.
		 */
		void UpdateGameState(GameState newState, float dt);
		using StateAudioPolicy = std::function<void(GameState, Scene&, AudioManager*)>;
		using PauseAudioPolicy = std::function<void(bool, bool, GameState, Scene&, AudioManager*)>;

		// Inject Scene used for runtime level loading
		/**
		 * @brief Supplies the scene instance used for state-driven level loading.
		 * @param s Scene that should receive future state loads.
		 */
		void SetScene(Scene* s) {
			scene = s;
		}

		// Map a GameState to a JSON level path
		/**
		 * @brief Registers a JSON level file for a specific game state.
		 * @param state Game-state identifier.
		 * @param levelPath Path to the JSON level file.
		 */
		void RegisterJsonState(GameState state, const std::string& levelPath) {
			jsonStatePaths[state] = levelPath;
		}

		// Inject AudioManager for state-based audio control
		/**
		 * @brief Supplies the audio manager used by state transition policies.
		 * @param mgr Audio manager service owned by the engine.
		 */
		void SetAudioManager(AudioManager* mgr) {
			audioManager = mgr;
		}

		// Inject game-specific state/audio policies
		/**
		 * @brief Installs a callback that applies audio behavior whenever a state is entered.
		 * @param policy State-entry audio policy callback.
		 */
		void SetStateAudioPolicy(StateAudioPolicy policy) {
			stateAudioPolicy = std::move(policy);
		}
		/**
		 * @brief Installs a callback that applies audio behavior when gameplay is paused or resumed.
		 * @param policy Pause/resume audio policy callback.
		 */
		void SetPauseAudioPolicy(PauseAudioPolicy policy) {
			pauseAudioPolicy = std::move(policy);
		}

	private:
		void OnQuit(const CoreFramework::Message& msg);
		bool TrySwitchJsonState(GameState state, float dt);
		void PreloadJsonStateAssets(GameState activeState);

	private:
		CoreFramework::MessageBus& messageBus;
		CoreFramework::SubscriberId quitSubId;

		Scene* scene = nullptr;
		std::unordered_map<GameState, std::string> jsonStatePaths;
		GameState currentState = GameState::MainMenu;
		GameState nextState = GameState::MainMenu;
		bool initialized = false;
		bool pendingSimActivation = false;

		// Audio service
		AudioManager* audioManager = nullptr;
		bool wasPaused = false;    // Track pause state for audio
		StateAudioPolicy stateAudioPolicy;
		PauseAudioPolicy pauseAudioPolicy;
	};
}
