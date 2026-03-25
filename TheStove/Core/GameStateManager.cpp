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
#include "EngineRng.hpp"
#include "GameStateManager.hpp"
#include "LevelSerializer.hpp"
#include "RuntimeLevel.hpp"

#include <unordered_set>
#include <vector>

namespace Framework {

	namespace {
		// Fixed seed for tutorial levels to ensure consistent RNG behavior 
		constexpr std::uint32_t kTutorialFixedSeed = 0x00C0FFEEu;

		/**
		 * @brief Appends level textures.
		 * @param levelPath Path to the level resource.
		 * @param inOutPaths Output value for in out paths.
		 * @param seen Parameter for seen.
		 */
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
		std::cout << "GameStateManager system initialized." << std::endl;
	}

	/**
	 * @brief Updates this object.
	 * @param dt Frame delta time in seconds.
	 * @return Result produced by this operation.
	 */
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
			if (pauseAudioPolicy) {
				pauseAudioPolicy(isPaused, wasPaused, currentGS, *scene, audioManager);
			}
			wasPaused = isPaused;
		}

		if (currentGS == nextGS && fpUpdate) {
			fpUpdate(dt);
		}
	}

	/**
	 * @brief Performs on quit.
	 * @param msg Parameter for msg.
	 * @return Result produced by this operation.
	 */
	void GameStateManager::OnQuit(const CoreFramework::Message& msg) {
		(void)msg;
		nextGS = GS_Quit;
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

	/**
	 * @brief Updates game state.
	 * @param newState Parameter for new state.
	 * @param dt Frame delta time in seconds.
	 * @return Result produced by this operation.
	 */
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

	/**
	 * @brief Attempts to switch json state.
	 * @param state Parameter for state.
	 * @param dt Frame delta time in seconds.
	 * @return Result produced by this operation.
	 */
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

		// Force deterministic RNG only for tutorial state.
		if (state == Framework::GS_Tutorial) {
			EngineRng::SetSeed(kTutorialFixedSeed);
			std::cout << "[GameStateManager] Tutorial fixed seed set to " << kTutorialFixedSeed << std::endl;
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

		if (stateAudioPolicy && scene) {
			stateAudioPolicy(state, *scene, audioManager);
		}

		PreloadJsonStateAssets(state);

		fpInit = nullptr;
		fpUpdate = nullptr;
		fpExit = nullptr;
		return true;
	}

	/**
	 * @brief Performs preload json state assets.
	 * @param activeState Parameter for active state.
	 * @return Result produced by this operation.
	 */
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
