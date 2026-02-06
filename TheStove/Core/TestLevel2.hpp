/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			TestLevel2.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Darren Toh, darren.toh@digipen.edu (100%)

 DESCRIPTION:		Header file Test state for GameStateManager. This script contains definitions for Init, Update
					and Exit functions specific to this level. To be Updated with calls to more components.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <iostream>

namespace Framework {

	void Level2Init(float deltaTime);

	void Level2Update(float deltaTime);

	void Level2Exit(float deltaTime);
}