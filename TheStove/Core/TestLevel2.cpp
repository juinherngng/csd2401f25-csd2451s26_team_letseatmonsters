/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			TestLevel2.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Darren Toh, darren.toh@digipen.edu (100%)

 DESCRIPTION:		Source file Test state for GameStateManager. This script contains definitions for Init, Update
					and Exit functions specific to this level. To be Updated with calls to more components.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "TestLevel2.hpp"

namespace Framework {

	void Level2Init(float deltaTime) {
		(void)deltaTime; // Suppress unused parameter warning
		std::cout << "Entered Level 1" << std::endl;
	}

	void Level2Update(float deltaTime) {
		std::cout << "Update Frame at DeltaTime: " << deltaTime << std::endl;
		std::cout << "Transiting to new State" << std::endl;
	}

	void Level2Exit(float deltaTime) {
		std::cout << "Exiting Level 2 at DeltaTime: " << deltaTime << std::endl;
	}
}