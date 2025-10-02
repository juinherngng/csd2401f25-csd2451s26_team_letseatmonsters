/*
----------------------------------------------------------------------------------------------------
FILE NAME:			Core.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		The core engine managing the game loop and systems.

		All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "Core.hpp"
#include "ImGuiDebugger.hpp"
#include "GameStateManager.hpp"


#include <chrono>
#include <thread>
#include <iostream>

namespace CoreFramework
{
	float gDt = 0.f;			// global delta time

	// global pointer to core
	//CoreEngine* CORE;

	CoreEngine::CoreEngine()
	{
		gameActive = true;	// game is running
		//CORE = this;		// set global pointer
	}

	CoreEngine::~CoreEngine() {}

	void CoreEngine::Initialize()
	{
		// initialize all systems
		for (unsigned i = 0; i < Systems.size(); i++)
		{
			Systems[i]->Initialize();
		}
	}

	// game loop is being called every frame in main in update()
	void CoreEngine::GameLoop(DebuggerApp& debugApp)
	{
		// add a currentTime variable to read system time
		using clock = std::chrono::high_resolution_clock;
		
		// this will store the time of the last frame
		//auto lastTime = clock::now();

		// gameloop is already updating every frame in main.cpp
		/*while (gameActive)
		{*/
			// get the current time
			auto currentTime = clock::now();
			
			// compute the time elapsed since last frame
			std::chrono::duration<float> elapsed = currentTime - lastTime;

			// set global dt variable
			gDt = elapsed.count();

			// update all systems
			for (auto* s : Systems)
			{
				auto sysStart = clock::now();
				s->Update(gDt);

				auto sysEnd = clock::now();
				std::chrono::duration<float> sysElapsed = sysEnd - sysStart;
				s->lastDt = sysElapsed.count();
			}

			FlushMessages();

			// update system performance %tages
			UpdateSystemTimes(debugApp, gDt);

			// update lastUpdated to current time
			lastTime = currentTime;
		//}
	}

	void CoreEngine::BroadcastMessage(Message *message)
	{
		// print out message for debugging purposes
		std::cout << "CoreEngine broadcasting message " << MsgIdToString(message->MessageId) << std::endl;

		//The message that tells the game to quit
		if (message->MessageId == MsgId::QUIT)
			gameActive = false;

		for (auto* s : Systems)
		{
			s->SendMessage(message);
		}
	}

	void CoreEngine::AddSystem(SystemInterface* system)
	{
		// add a new system to the list of systems
		Systems.push_back(system);
		std::cout << "Added system: " << system->GetName() << std::endl;
	}

	void CoreEngine::DestroySystems()
	{
		std::cout << "DestroySystems called, system count: " << Systems.size() << std::endl;
		//Delete all the systems in reverse order
		for (size_t i = 0; i < Systems.size(); i++)
		{
			size_t index = Systems.size() - i - 1;
			std::cout << "Deleted system: " << Systems[index]->GetName() << std::endl;

			delete Systems[index];
		}

		Systems.clear();
	}

	void CoreEngine::UpdateSystemTimes(DebuggerApp& debugApp, float totalDt)
	{
		debugApp.sysPerformance.clear();

		for (auto& sys : Systems)
		{
			float percent = (totalDt > 0.f) ? (sys->lastDt / totalDt) * 100.f : 0.f;
			debugApp.sysPerformance.push_back({ sys->GetName(), percent });
		}
	}

}