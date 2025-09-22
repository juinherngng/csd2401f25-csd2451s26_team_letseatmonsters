#pragma once

#include "System.hpp"

#include <vector>

namespace Framework
{
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

	private:
		std::vector<SystemInterface*> Systems;

		unsigned lastUpdated;	// the last time game was updated

		bool gameActive;		// game running (true), game shutting down (false)
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