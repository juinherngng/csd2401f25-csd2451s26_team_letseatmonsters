/*
----------------------------------------------------------------------------------------------------
FILE NAME:			TestLevel.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Darren Toh, darren.toh@digipen.edu

DESCRIPTION:		Source file Test state for GameStateManager. This script contains definitions for Init, Update
					and Exit functions specific to this level. To be Updated with calls to more components.

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
