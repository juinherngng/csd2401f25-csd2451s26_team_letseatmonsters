/*
----------------------------------------------------------------------------------------------------
FILE NAME:			Core.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		The core engine managing the game loop and systems.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "Core.hpp"
#include "GameStateManager.hpp"

#include <chrono>
#include <thread>
#include <iostream>

namespace CoreFramework
{
	CoreEngine::CoreEngine()
	{
		gameActive = true;	// game is running
	}

	CoreEngine::~CoreEngine() 
	{
		// Ensure proper cleanup order
		messageQueue.clear();  // Clear any remaining messages
		DestroySystems();      // Destroy all systems
		Systems.clear();       // Explicitly clear the vector
	}

	void CoreEngine::Initialize()
	{
		// initialize all systems
		for (auto& s : Systems)
		{
			s->Initialize();
		}
	}

	// game loop is being called every frame in main in update()
	void CoreEngine::GameLoop()
	{
		// add a currentTime variable to read system time
		using clock = std::chrono::high_resolution_clock;

		// get the current time
		auto currentTime = clock::now();
			
		// compute the time elapsed since last frame
		std::chrono::duration<float> elapsed = currentTime - lastTime;

		// set delta time
		deltaTime = elapsed.count();

		// update all systems
		for (auto& s : Systems)
		{
			auto sysStart = clock::now();
			s->Update(deltaTime);

			auto sysEnd = clock::now();
			std::chrono::duration<float> sysElapsed = sysEnd - sysStart;
			s->lastDt = sysElapsed.count();
		}

		FlushMessages();	// process any queued messages

		// update lastUpdated to current time
		lastTime = currentTime;
	}

	void CoreEngine::BroadcastMessage(Message *message)
	{
		// print out message for debugging purposes
		//std::cout << "CoreEngine broadcasting message " << MsgIdToString(message->MessageId) << std::endl;

		//The message that tells the game to quit
		if (message->MessageId == MsgId::QUIT)
			gameActive = false;

		for (auto& s : Systems)
		{
			s->SendMessage(message);
		}
	}

	void CoreEngine::AddSystem(std::unique_ptr<SystemInterface> system)
	{
		// add a new system to the list of systems
		std::cout << "Added system: " << system->GetName() << std::endl;
		Systems.push_back(std::move(system));
	}

	void CoreEngine::DestroySystems()
	{
		std::cout << "DestroySystems called, system count: " << Systems.size() << std::endl;
		
		// Delete all the systems in reverse order
		// unique_ptr handles automatic deletion
		for (size_t i = 0; i < Systems.size(); i++)
		{
			size_t index = Systems.size() - i - 1;
			std::cout << "Destroying system: " << Systems[index]->GetName() << std::endl;
		}

		Systems.clear(); // unique_ptrs will automatically clean up
	}
}