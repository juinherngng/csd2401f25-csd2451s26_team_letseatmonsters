/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameStateManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Darren Toh, darren.toh@digipen.edu		(20%)
 CO-AUTHORS:		Seah Wang Hua, wanghua.seah@digipen.edu (30%)
					Ng Juin Herng, juinherng.ng@digipen.edu (20%)
					Yat Chun Wee, y.chunwee@digipen.edu		(30%)

 DESCRIPTION:		This file implements the logic for first-time initialization, per-frame updates, and transitions
					between states, preferring JSON-driven runtime level loading when mappings are registered, with a
					fallback to legacy function-pointer-based level init/update/exit routines. Manages scene simulation
					activation timing, tracks pause state to pause/resume audio in gameplay, and controls state-based
					playback and cleanup of background music and ambience through the injected AudioManager instance.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"

#include "AudioManager.hpp"
#include "ApplicationShutdown.hpp"
#include "EngineRng.hpp"
#include "GameStateManager.hpp"
#include "LevelSerializer.hpp"
#include "Logger.hpp"
#include "RuntimeLevelPipeline.hpp"

namespace Framework {
	/**
	 * @brief Returns a stable display name for a game state identifier.
	 * @param state State identifier to stringify.
	 * @return Null-terminated name for logs and debugging.
	 */
	const char* ToString(GameState state) {
		switch (state) {
		case GameState::MainMenu:
			return "MainMenu";
		case GameState::Tutorial:
			return "Tutorial";
		case GameState::Kitchen01:
			return "Kitchen01";
		case GameState::Quit:
			return "Quit";
		default:
			return "Unknown";
		}
	}

	namespace {
		// Fixed seed for tutorial levels to ensure consistent RNG behavior 
		constexpr std::uint32_t kTutorialFixedSeed = 0x00C0FFEEu;
	}

	/**
	 * @brief Performs game state manager.
	 * @param bus Parameter for bus.
	 * @return Result produced by this operation.
	 */
	GameStateManager::GameStateManager(CoreFramework::MessageBus& bus)
		: messageBus(bus) {
		quitSubId = messageBus.Subscribe(
			CoreFramework::MessageType::QUIT,
			[this](const CoreFramework::Message& msg) { OnQuit(msg); }
		);
	}

	/**
	 * @brief Performs ~game state manager.
	 * @return Result produced by this operation.
	 */
	GameStateManager::~GameStateManager() {
		messageBus.Unsubscribe(CoreFramework::MessageType::QUIT, quitSubId);
	}

	/**
	 * @brief Initializes this object.
	 * @return Result produced by this operation.
	 */
	void GameStateManager::Initialize() {
		TS_LOG_INFO("[GameStateManager] System initialized.");
	}

	/**
	 * @brief Updates this object.
	 * @param dt Frame delta time in seconds.
	 * @return Result produced by this operation.
	 */
	void GameStateManager::Update(float dt) {
		if (!initialized) {
			InitializeGameState(GameState::MainMenu, dt);
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
			if (pauseAudioPolicy) {
				pauseAudioPolicy(isPaused, wasPaused, currentState, *scene, audioManager);
			}
			wasPaused = isPaused;
		}

		if (currentState == nextState && legacyUpdateFn) {
			legacyUpdateFn(dt);
		}
	}

	/**
	 * @brief Performs on quit.
	 * @param msg Parameter for msg.
	 * @return Result produced by this operation.
	 */
	void GameStateManager::OnQuit(const CoreFramework::Message& msg) {
		(void)msg;
		nextState = GameState::Quit;
		RequestApplicationShutdown();
	}

	/**
	 * @brief Returns the stable name for this object.
	 * @return Requested value.
	 */
	std::string GameStateManager::GetName() {
		return "GameStateManager";
	}

	/**
	 * @brief Initializes game state.
	 * @param GS Parameter for gs.
	 * @param dt Frame delta time in seconds.
	 * @return Result produced by this operation.
	 */
	void GameStateManager::InitializeGameState(GameState state, float dt) {
		nextState = currentState = state;

		// Warm up target-state textures before switching (menu->level, level->cutscene).
		if (jsonStatePaths.find(state) != jsonStatePaths.end()) {
			RuntimeLevelPipeline::PreloadLevelDependencies(jsonStatePaths[state]);
		}

		// Prefer JSON mapping if available
		if (TrySwitchJsonState(state, dt)) {
			initialized = true;
			return;
		}

		// Fallback to legacy function-pointer state
		legacyInitFn = Level1Init;
		legacyUpdateFn = Level1Update;
		legacyExitFn = Level1Exit;

		if (legacyInitFn) {
			legacyInitFn(dt);
		}
		initialized = true;
	}

	/**
	 * @brief Updates game state.
	 * @param newState Parameter for new state.
	 * @param dt Frame delta time in seconds.
	 * @return Result produced by this operation.
	 */
	void GameStateManager::UpdateGameState(GameState newState, float dt) {
		nextState = newState;

		// If we were using legacy function pointers, call exit
		if (legacyExitFn) {
			legacyExitFn(dt);
		}

		currentState = newState;

		// Warm up textures for the destination state before scene build.
		if (jsonStatePaths.find(currentState) != jsonStatePaths.end()) {
			RuntimeLevelPipeline::PreloadLevelDependencies(jsonStatePaths[currentState]);
		}

		// Prefer JSON mapping if available
		if (TrySwitchJsonState(currentState, dt)) {
			return;
		}

		// Fallback to legacy hard-coded states
		switch (currentState) {
		case GameState::MainMenu:
			legacyInitFn = Level1Init;
			legacyUpdateFn = Level1Update;
			legacyExitFn = Level1Exit;
			if (legacyInitFn) legacyInitFn(dt);
			break;

		case GameState::Kitchen01:
			legacyInitFn = Level2Init;
			legacyUpdateFn = Level2Update;
			legacyExitFn = Level2Exit;
			if (legacyInitFn) legacyInitFn(dt);
			break;

		case GameState::Quit:
			// No-op, let app quit
			break;
		case GameState::Tutorial:
			break;
		}
	}

	/**
	 * @brief Attempts to switch json state.
	 * @param state Parameter for state.
	 * @param dt Frame delta time in seconds.
	 * @return Result produced by this operation.
	 */
	bool GameStateManager::TrySwitchJsonState(GameState state, float dt) {
		(void)dt;
		auto it = jsonStatePaths.find(state);
		if (it == jsonStatePaths.end()) {
			return false;
		}
		if (!scene) {
			TS_LOG_ERROR("[GameStateManager] Scene not set; cannot load JSON level for state " << ToString(state));
			return false;
		}

		// Force deterministic RNG only for tutorial state.
		if (state == Framework::GameState::Tutorial) {
			EngineRng::SetSeed(kTutorialFixedSeed);
			TS_LOG_INFO("[GameStateManager] Tutorial fixed seed set to " << kTutorialFixedSeed);
		}

		const std::string& path = it->second;
		const RuntimeLevelPipeline::LevelLoadResult loadResult = RuntimeLevelPipeline::LoadLevelIntoSceneDetailed(path, *scene);
		if (!loadResult.success) {
			TS_LOG_ERROR("[GameStateManager] Failed to build level from JSON: " << path << " (" << loadResult.failureReason << ")");
			return false;
		}

		if (!loadResult.validationWarnings.empty()) {
			TS_LOG_WARN("[GameStateManager] Loaded '" << path << "' with " << loadResult.validationWarnings.size() << " validation warning(s).");
		}

		// Auto-start simulation for gameplay only
		if (state == Framework::GameState::MainMenu) {
			scene->SetSimulationActive(false);  // main menu stays paused
			pendingSimActivation = false;
		}
		else {
			pendingSimActivation = true;        // other states (e.g., gameplay)
		}

		if (stateAudioPolicy && scene) {
			stateAudioPolicy(state, *scene, audioManager);
		}

		PreloadJsonStateAssets(state);

		legacyInitFn = nullptr;
		legacyUpdateFn = nullptr;
		legacyExitFn = nullptr;
		return true;
	}

	/**
	 * @brief Performs preload json state assets.
	 * @param activeState Parameter for active state.
	 * @return Result produced by this operation.
	 */
	void GameStateManager::PreloadJsonStateAssets(GameState activeState) {
		for (const auto& [state, levelPath] : jsonStatePaths) {
			if (state == activeState) {
				continue;
			}

			RuntimeLevelPipeline::PreloadLevelDependencies(levelPath);
		}
	}

	/**
	 * @brief Performs stop current audio.
	 * @return Result produced by this operation.
	 */
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
