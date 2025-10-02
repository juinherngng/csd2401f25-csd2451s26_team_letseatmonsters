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

#include <vector>
#include <chrono>

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

		void GameLoop();

		void DestroySystems();

		void BroadcastMessage(Message* msg);

		void AddSystem(SystemInterface* system);

		void Initialize();

		float GetFPS() const { return fps; }

		// Accessor to Read-Only values of Systems
		const std::vector<SystemInterface*>& GetSystems() const { return Systems; }

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

	private:
		std::vector<SystemInterface*> Systems;

		float fps = 0.f;		// fps counter

		bool gameActive;		// game running (true), game shutting down (false)

		std::chrono::high_resolution_clock::time_point lastTime; // time of last frame
	};

	class MessageQuit : public Message
	{
		MessageQuit() : Message(MsgId::QUIT) {}
	};

	class ToggleDebug : public Message
	{
		bool debugActive;

		ToggleDebug(bool debug) : Message(MsgId::TOGGLE_DEBUG_INFO) , debugActive(debug) {} 
	};

	extern CoreEngine* CORE;
}