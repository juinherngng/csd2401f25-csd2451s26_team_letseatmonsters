/*
----------------------------------------------------------------------------------------------------
FILE NAME:			Core.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		The core engine managing the game loop and systems.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <Core.hpp>

#include <chrono>
#include <thread>

namespace CoreFramework
{
	float gDt = 0.f;			// global delta time

	static float smoothedDt = 0.0f; // smoothed delta time for fps calc

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

	// not functional as of now
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

			// update all systems
			for (unsigned i = 0; i < Systems.size(); i++)
			{
				Systems[i]->Update(gDt);
			}

			// update system performance %tages
			UpdateSystemTimes(debugApp, gDt);

			// render the debugger
			debugApp.RunDebuggerApp();

			// Smoothing for gDt (for the fps)
			// Account for division by 0 on the first frame where gDt = 0
			// This controls how fast the fps counter reacts to changes
			// (higher value = smoother fps) else 
			// (lower value = faster fps change response but more jittery)
			smoothedDt = (smoothedDt == 0.0f) ? gDt : (0.96f * smoothedDt) + (0.04f * gDt);

			// Prevents division by 0 on the first frame where gDt = 0
			debugApp.fps = (smoothedDt > 0.f) ? (1.f / smoothedDt) : 0.f;
			debugApp.msperFrame = (smoothedDt * 1000.0f);

			// update lastUpdated to current time
			lastTime = currentTime;
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

	void CoreEngine::UpdateSystemTimes(DebuggerApp& debugApp, float totalDt)
	{
		debugApp.sysPerformance.clear();

		for (auto& sys : Systems)
		{
			float percent = (totalDt > 0.0f) ? (sys->lastDt / totalDt) * 100.0f : 0.0f;
			debugApp.sysPerformance.push_back({ sys->GetName(), percent });
		}
	}

}