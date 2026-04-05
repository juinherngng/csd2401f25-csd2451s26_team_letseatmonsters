/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Core.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (50%)
 CO-AUTHOR:         Yat Chun Wee, y.chunwee@digipen.edu	    (50%)

 DESCRIPTION:		The core engine managing the game loop, systems, and messaging.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <chrono>
#include <memory>
#include <utility>
#include <vector>

#include "EngineCore/ApplicationShutdown.hpp"
#include "EngineCore/MessageBus.hpp"
#include "EngineCore/System.hpp"

namespace CoreFramework {
	class CoreEngine {
	public:
		/**
		 * @brief Constructs the CoreEngine and prepares its initial runtime state.
		 */
		CoreEngine();
		/**
		 * @brief Destroys the CoreEngine and releases all registered systems.
		 */
		~CoreEngine();

		/**
		 * @brief Runs one frame of the game loop using measured delta time.
		 */
		void GameLoop();

		/**
		 * @brief Runs one frame of the game loop using a caller-supplied delta time.
		 * @param dtOverride Delta time to use for this frame.
		 */
		void GameLoop(float dtOverride);

		/**
		 * @brief Destroys all registered systems in reverse order of addition.
		 */
		void DestroySystems();

		/**
		 * @brief Registers a new system with the engine.
		 * @param system Unique pointer to the system being added.
		 */
		void AddSystem(std::unique_ptr<SystemInterface> system);

		/**
		 * @brief Initializes all registered systems.
		 */
		void Initialize();

		/**
		 * @brief Returns the last computed frames-per-second value.
		 * @return FPS as a float.
		 */
		float GetFPS() const {
			// Expose the cached FPS value computed by the engine loop.
			return fps;
		}

		/**
		 * @brief Returns the last computed delta time value.
		 * @return Delta time in seconds.
		 */
		float GetDeltaTime() const {
			// Expose the timestep used for the most recent frame update.
			return deltaTime;
		}

		/**
		 * @brief Returns a read-only view of all registered systems.
		 * @return Const reference to the system list.
		 */
		const std::vector<std::unique_ptr<SystemInterface>>& GetSystems() const {
			// Allow debug and tooling code to inspect the live system list without taking ownership.
			return Systems;
		}

		/**
		 * @brief Fetches the first registered system matching the requested type.
		 * @tparam T Concrete system type derived from SystemInterface.
		 * @return Pointer to the matching system, or nullptr if absent.
		 */
		template<typename T>
		T* GetSystem() {
			// Search linearly because the system list is small and ordered by engine bootstrap.
			for (auto& s : Systems)
				if (auto* casted = dynamic_cast<T*>(s.get()))
					return casted;
			return nullptr;
		}

		/**
		 * @brief Const-qualified overload of GetSystem().
		 * @tparam T Concrete system type derived from SystemInterface.
		 * @return Const pointer to the matching system, or nullptr if absent.
		 */
		template<typename T>
		T const* GetSystem() const {
			// Provide const access for inspector code that should not mutate systems.
			for (auto const& s : Systems)
				if (auto const* casted = dynamic_cast<T const*>(s.get()))
					return casted;
			return nullptr;
		}

		/**
		 * @brief Returns whether the application is still running.
		 * @return True if the application is active, otherwise false.
		 */
		bool IsGameActive() const {
			// Delegate shutdown state to the shared application-shutdown helper.
			return !IsApplicationShutdownRequested();
		}

		/**
		 * @brief Returns mutable access to the engine message bus.
		 * @return Reference to the internal MessageBus.
		 */
		MessageBus& GetMessageBus() {
			// Systems and tools use the shared bus for decoupled communication.
			return messageBus;
		}

		/**
		 * @brief Returns const access to the engine message bus.
		 * @return Const reference to the internal MessageBus.
		 */
		const MessageBus& GetMessageBus() const {
			// Provide read-only bus access for inspectors and const engine contexts.
			return messageBus;
		}

	private:
		using SystemPtr = std::unique_ptr<SystemInterface>;

		std::vector<SystemPtr>		  Systems;
		MessageBus					  messageBus;   // pub/sub message bus

		float deltaTime = 0.f;	// delta time (per frame)
		float fps = 0.f;		// fps counter
		std::chrono::high_resolution_clock::time_point lastTime; // time of last frame
	};
}
