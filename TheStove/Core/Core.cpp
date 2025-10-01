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

#include <chrono>

namespace CoreFramework
{
	float gDt = 0.f;			// global delta time

	// global pointer to core
	CoreEngine* CORE;

	CoreEngine::CoreEngine()
	{
		gameActive = true;	// game is running
		CORE = this;		// set global pointer
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

	void CoreEngine::GameLoop(DebuggerApp& debugApp)
	{
		// add a currentTime variable to read system time
		using clock = std::chrono::high_resolution_clock;
		
		// this will store the time of the last frame
		auto lastTime = clock::now();

		while (gameActive)
		{
			// get the current time
			auto currentTime = clock::now();
			
			// compute the time elapsed since last frame
			std::chrono::duration<float> elapsed = currentTime - lastTime;

			// set global dt variable
			gDt = elapsed.count();

			// update fps counter
			fps = (gDt > 0.f) ? (1.f / gDt) : 0.f;

			// Prevents division by 0 on the first frame where gDt = 0
			debugApp.fps = (CoreFramework::gDt > 0.f) ? (1.f / CoreFramework::gDt) : 0.f;
			debugApp.msperFrame = (CoreFramework::gDt * 1000.0f);

			// update lastUpdated to current time
			lastTime = currentTime;

			// update all systems
			for (unsigned i = 0; i < Systems.size(); i++)
			{
				Systems[i]->Update(gDt);
			}

			debugApp.RunDebuggerApp();
		}
	}

	void CoreEngine::BroadcastMessage(Message *message)
	{
		// print out message for debugging purposes
		std::cout << "CoreEngine broadcasting message " << MsgIdToString(message->MessageId) << std::endl;

		//The message that tells the game to quit
		if (message->MessageId == MsgId::QUIT)
			gameActive = false;

		//Send the message to every system--each
		//system can figure out whether it cares
		//about a given message or not
		for (unsigned i = 0; i < Systems.size(); ++i)
			Systems[i]->SendMessage(message);
	}

	void CoreEngine::AddSystem(SystemInterface* system)
	{
		// add a new system to the list of systems
		Systems.push_back(system);
	}

	void CoreEngine::DestroySystems()
	{
		//Delete all the systems in reverse order
		for (unsigned i = 0; i < Systems.size(); i++)
		{
			delete Systems[Systems.size() - i - 1];
		}
	}

}