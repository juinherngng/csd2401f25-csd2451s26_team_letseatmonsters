/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GameStateManager.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Darren Toh, darren.toh@digipen.edu

DESCRIPTION:		Game State Manager interface derived from System.hpp. Uses 3 Function pointers
					and redirects them to level/scene-specific init, update and exit functions.
					These function pointers are then called in main by the game state manager.
					This is a header file for definitions.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#include "GameStateManager.hpp"
#include "TestLevel.hpp"
#include "TestLevel2.hpp"

namespace Framework {

	//Ints representing Game States being cycled on update
	extern int currentGS = 0, nextGS = 0;

	////Check if game state has been entered and initialised
	extern bool init = false;

	//Smart Function Pointers for interchanging functionality for game states
	typedef std::function<void(float dt)> FP;

	extern FP fpInit = nullptr, fpUpdate = nullptr, fpExit = nullptr; // Function pointers that changes depending on what state the game is in currently
	//Setup Manager Logic
	void GameStateManager::Initialize() {
		std::cout << "GameStateManagerSystem initialized." << std::endl;
	}
	//Manager Update loop
	void GameStateManager::Update(float dt) {
		if (!init) {
			InitializeGameState(0, dt);
		}
		if (currentGS == nextGS) {
			fpUpdate(dt);
		}
	}

	void GameStateManager::SendMessage(CoreFramework::Message* msg) {
		
	}
	//Get string of manager for debugging
	std::string GameStateManager::GetName() {
		return "GameStateManager";
	}
	//Initialize first state to run on load
	void GameStateManager::InitializeGameState(int GS, float dt) {
		nextGS = currentGS = GS;
		fpInit = Level1Init;
		fpUpdate = Level1Update;
		fpExit = Level1Exit;

		fpInit(dt);
		init = true;
	}
	//Update Game Manager with a new State
	void GameStateManager::UpdateGameState(int newState, float dt) {
		nextGS = newState;
		fpExit(dt);
		currentGS = newState;
		switch (currentGS) {
		case GS_Level1:
			fpInit = Level1Init;
			fpUpdate = Level1Update;
			fpExit = Level1Exit;

			fpInit(dt);
			break;
		case GS_Level2:
			fpInit = Level2Init;
			fpUpdate = Level2Update;
			fpExit = Level2Exit;

			fpInit(dt);
			break;
		case GS_Quit:
			break;
		}
	}
}