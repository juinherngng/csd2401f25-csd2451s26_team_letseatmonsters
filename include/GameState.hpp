/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GameState.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Darren Toh, darren.toh@digipen.edu

DESCRIPTION:		Game State class used to call state-specific Init, Update loop and Exit code
					from GameStateManager in Main.cpp.
					All Game States should be derived from this file and their functions overloaded.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#pragma once

#include <iostream>
namespace Framework {
	class GameState {
	public:
		virtual ~GameState() = default;

		//Called Once when Game State changes to this
		virtual void StateInit() = 0;
		//To be called once per frame by GameStateManager in Main update loop
		virtual void StateUpdate() = 0;
		//Called Once when Game State is changed to another dervived class
		virtual void StateExit() = 0;

		virtual std::string GetStateName() = 0;
	};
}