#include <Core.hpp>

#include <chrono>

namespace Framework
{
	// global pointer to core
	CoreEngine* CORE;

	CoreEngine::CoreEngine()
	{
		lastUpdated = 0;
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
	void CoreEngine::GameLoop()
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

			// convert to float seconds
			float dt = elapsed.count();

			// update lastUpdated to current time
			lastTime = currentTime;

			// update all systems
			for (unsigned i = 0; i < Systems.size(); i++)
			{
				Systems[i]->Update(dt);
			}
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