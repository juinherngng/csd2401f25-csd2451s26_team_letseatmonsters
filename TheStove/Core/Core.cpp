/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Core.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		The core engine managing the game loop and systems.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "Core.hpp"
#include "GameStateManager.hpp"
#include "Logger.hpp"

#include <chrono>
#include <thread>

namespace CoreFramework {
	CoreEngine::CoreEngine() {
		// Initialize lastTime so the first deltaTime is ~0
		lastTime = std::chrono::high_resolution_clock::now();

		// Subscribe to PLAY_AUDIO messages for logging purposes (optional)
		// This allows Core to see audio message traffic for debugging
		messageBus.Subscribe(MessageType::PLAY_AUDIO,
			[](const Message& msg) {
				const auto& audioMsg = static_cast<const PlayAudioMessage&>(msg);
				TS_LOG_DEBUG("[Core] PLAY_AUDIO message: sound='" << audioMsg.soundName
					<< "', volume=" << audioMsg.volume
					<< ", paused=" << (audioMsg.paused ? "true" : "false"));
			});

		// Subscribe to STOP_AUDIO messages for logging
		messageBus.Subscribe(MessageType::STOP_AUDIO,
			[](const Message& msg) {
				const auto& stopMsg = static_cast<const StopAudioMessage&>(msg);
				if (stopMsg.soundName.empty()) {
					TS_LOG_DEBUG("[Core] STOP_AUDIO message: stopping ALL sounds");
				}
				else {
					TS_LOG_DEBUG("[Core] STOP_AUDIO message: sound='" << stopMsg.soundName << "'");
				}
			});

		messageBus.Subscribe(MessageType::SCENE_FLOW_STATE_CHANGED,
			[](const Message& msg) {
				const auto& flowMsg = static_cast<const SceneFlowStateChangedMessage&>(msg);
				TS_LOG_DEBUG("[Core] SCENE_FLOW_STATE_CHANGED: state='" << flowMsg.stateName
					<< "', simulationActive=" << (flowMsg.simulationActive ? "true" : "false"));
			});

		messageBus.Subscribe(MessageType::LEVEL_LOAD_QUEUED,
			[](const Message& msg) {
				const auto& levelMsg = static_cast<const LevelLoadQueuedMessage&>(msg);
				TS_LOG_DEBUG("[Core] LEVEL_LOAD_QUEUED: path='" << levelMsg.levelPath
					<< "', activateSimulation=" << (levelMsg.activateSimulation ? "true" : "false"));
			});

		messageBus.Subscribe(MessageType::LEVEL_LOADED,
			[](const Message& msg) {
				const auto& levelMsg = static_cast<const LevelLoadedMessage&>(msg);
				TS_LOG_DEBUG("[Core] LEVEL_LOADED: path='" << levelMsg.levelPath
					<< "', simulationActive=" << (levelMsg.simulationActive ? "true" : "false"));
			});

		messageBus.Subscribe(MessageType::PAUSE_OVERLAY_CHANGED,
			[](const Message& msg) {
				const auto& pauseMsg = static_cast<const PauseOverlayChangedMessage&>(msg);
				TS_LOG_DEBUG("[Core] PAUSE_OVERLAY_CHANGED: isOpen=" << (pauseMsg.isOpen ? "true" : "false"));
			});

		messageBus.Subscribe(MessageType::CUTSCENE_SKIPPED,
			[](const Message& msg) {
				const auto& skipMsg = static_cast<const CutsceneSkippedMessage&>(msg);
				TS_LOG_DEBUG("[Core] CUTSCENE_SKIPPED: transitioned=" << (skipMsg.transitionedCutscene ? "true" : "false")
					<< ", target='" << skipMsg.targetLevelPath << "'");
			});
	}

	CoreEngine::~CoreEngine() {
		// Ensure proper cleanup order
		messageBus.ClearQueue();   // Clear MessageBus queue
		DestroySystems();          // Destroy all systems
		Systems.clear();           // Explicitly clear the vector
	}

	void CoreEngine::Initialize() {
		// initialize all systems
		for (auto& s : Systems) {
			s->Initialize();
		}
	}

	// game loop is being called every frame in main in update()
	void CoreEngine::GameLoop() {
		// add a currentTime variable to read system time
		using clock = std::chrono::high_resolution_clock;

		// get the current time
		auto currentTime = clock::now();

		// compute the time elapsed since last frame
		std::chrono::duration<float> elapsed = currentTime - lastTime;

		// set delta time
		deltaTime = elapsed.count();

		// update all systems
		for (auto& s : Systems) {
			auto sysStart = clock::now();
			s->Update(deltaTime);

			auto sysEnd = clock::now();
			std::chrono::duration<float> sysElapsed = sysEnd - sysStart;
			s->lastDt = sysElapsed.count();
		}

		// Process pub/sub messages from MessageBus
		messageBus.ProcessQueue();

		// update lastUpdated to current time
		lastTime = currentTime;
	}

	void CoreEngine::GameLoop(float dtOverride) {
		using clock = std::chrono::high_resolution_clock;

		deltaTime = dtOverride;

		for (auto& s : Systems) {
			auto sysStart = clock::now();
			s->Update(deltaTime);

			auto sysEnd = clock::now();
			std::chrono::duration<float> sysElapsed = sysEnd - sysStart;
			s->lastDt = sysElapsed.count();
		}

		messageBus.ProcessQueue();
		lastTime = clock::now();
	}

	void CoreEngine::AddSystem(std::unique_ptr<SystemInterface> system) {
		// add a new system to the list of systems
		TS_LOG_INFO("Added system: " << system->GetName());
		Systems.push_back(std::move(system));
	}

	void CoreEngine::DestroySystems() {
		TS_LOG_INFO("DestroySystems called, system count: " << Systems.size());

		// Delete all the systems in reverse order
		// unique_ptr handles automatic deletion
		for (size_t i = 0; i < Systems.size(); i++) {
			size_t index = Systems.size() - i - 1;
			TS_LOG_DEBUG("Destroying system: " << Systems[index]->GetName());
		}

		Systems.clear(); // unique_ptrs will automatically clean up
	}
}

