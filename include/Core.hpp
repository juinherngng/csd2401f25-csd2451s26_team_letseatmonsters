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