/*
----------------------------------------------------------------------------------------------------
FILE NAME:			Core.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		The core engine managing the game loop and systems.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "System.hpp"
#include "ImGuiDebugger.hpp"

#include <vector>
#include <chrono>
#include <deque>
#include <memory>
#include <utility>

namespace CoreFramework
{
	// how to access global dt:
	// float dt = CoreFramework::gDt;
	extern float gDt;			// global delta time

	class CoreEngine
	{
	public:
		CoreEngine();
		~CoreEngine();

		void GameLoop(DebuggerApp& debugApp);

		void DestroySystems();

		void BroadcastMessage(Message* msg);

		void AddSystem(SystemInterface* system);

		void Initialize();

		float GetFPS() const { return fps; }

		void UpdateSystemTimes(DebuggerApp& debugApp, float totalDt);

		template<typename T>
		T* GetSystem()
		{
			for (auto* s : Systems)
				if (auto* casted = dynamic_cast<T*>(s))
					return casted;
			return nullptr;
		}

		template<typename T>
		T* const GetSystem() const
		{
			for (auto* s : Systems)
				if (auto* casted = dynamic_cast<T*>(s))
					return casted;
			return nullptr;
		}

		template<typename T, typename... Args>
		void Post(Args&&... args)
		{
			messageQueue.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
		}

		void FlushMessages()
		{
			while (!messageQueue.empty())
			{
				BroadcastMessage(messageQueue.front().get());
				messageQueue.pop_front();
			}
		}

	private:
		using MessagePtr = std::unique_ptr<Message>;

		std::vector<SystemInterface*> Systems;
		std::deque<MessagePtr>		  messageQueue; // messages to be processed at the start of the next frame

		float fps = 0.f;		// fps counter
		bool gameActive;		// game running (true), game shutting down (false)
		std::chrono::high_resolution_clock::time_point lastTime; // time of last frame
	};

	extern CoreEngine* CORE;
}