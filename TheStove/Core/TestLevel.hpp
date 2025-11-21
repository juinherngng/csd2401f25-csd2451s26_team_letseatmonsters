/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			TestLevel.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Darren Toh, darren.toh@digipen.edu

 DESCRIPTION:		Header file Test state for GameStateManager. This script contains definitions for Init, Update
					and Exit functions specific to this level. To be Updated with calls to more components.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <iostream>

namespace Framework {

	void Level1Init(float deltaTime);

	void Level1Update(float deltaTime);

	void Level1Exit(float deltaTime);
}