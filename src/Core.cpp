#include <Core.hpp>

#include <chrono>

namespace Framework
{
	// global pointer to core
	CoreEngine* CORE;

	CoreEngine::CoreEngine()
	{
		lastUpdated = 0;
		gameActive = true;
		CORE = this;		// set global pointer
	}

	CoreEngine::~CoreEngine() {}

	void CoreEngine::Initialize()
	{
		for (unsigned i = 0; i < Systems.size(); i++)
		{
			Systems[i]->Initialize();
		}
	}

	// not functional as of now
	void CoreEngine::GameLoop()
	{
		// need to fix this
		auto currentTime = std::chrono::system_clock::now();

		//lastUpdated = &currentTime;

		while (gameActive)
		{
			float dt = lastUpdated / 1000.f;

			//lastupdated = currenttime

			for (unsigned i = 0; i < Systems.size(); i++)
			{
				Systems[i]->Update(dt);
			}
		}
	}

	void CoreEngine::BroadcastMessage(Message *message)
	{
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