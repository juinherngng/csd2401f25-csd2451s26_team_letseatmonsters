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

#include "TestLevel.hpp"
namespace Framework {

	void Level1Init(float deltaTime) {
		std::cout << "Entered Level 1" << std::endl;
	}

	void Level1Update(float deltaTime) {
		std::cout << "Update Frame at DeltaTime: " << deltaTime << std::endl;
	}

	void Level1Exit(float deltaTime){
		std::cout << "Exiting Level 1 at DeltaTime: " << deltaTime << std::endl;
	}
}
