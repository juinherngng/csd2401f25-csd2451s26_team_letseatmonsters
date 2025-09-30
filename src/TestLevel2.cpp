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
	//class TestLevel2 : public GameState {
	//	//Called Once when Game State changes to this
	//	void StateInit() override {
	//		std::cout << "Entered State: " << GetStateName() << std::endl;
	//	}
	//	//To be called once per frame by GameStateManager in Main update loop
	//	void StateUpdate(float dt) override {
	//		std::cout << "Update Frame at DeltaTime: " << dt << std::endl;
	//		std::cout << "Calling Exit" << dt << std::endl;
	//		StateExit();

	//	}
	//	//Called Once when Game State is changed to another dervived class
	//	void StateExit() override {
	//		std::cout << "Exiting State: " << GetStateName() << std::endl;
	//	}

	//	std::string GetStateName() override { return "TestLevel2"; }
	//};

	void Level2Init(float deltaTime) {
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