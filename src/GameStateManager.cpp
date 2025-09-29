/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GameStateManager.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Darren Toh, darren.toh@digipen.edu

DESCRIPTION:		Game State Manager interface derived from System.hpp.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#include "GameStateManager.hpp"

namespace Framework {
	//Setup Manager Logic
	//Find and Register all Game States
	//To be updated with Deserializer
	void GameStateManager::Initialize() {}
	//Manager Update loop
	void GameStateManager::Update(float dt){}
	//Message Sending
	void GameStateManager::SendMessage(Framework::Message* msg){}
	//Get Name of Manager
	std::string GameStateManager::GetName() { return "GameStateManager"; }
	
	//Initialize default values
	bool init = false;

	std::string GameStateManager::GetGameState() {
		return currentState->GetStateName();
	}

	bool GameStateManager::HasState() const {
		return static_cast<bool>(currentState);
	}

	void GameStateManager::SetGameState(FP gameState) {
		if (currentState) {
			currentState->StateExit();
		}
		currentState = std::move(gameState);
		if (currentState) {
			currentState->StateInit();
		}
	}

	void GameStateManager::UpdateGameState() {
		if (currentState) {
			currentState->StateUpdate();
		}
	}

	void GameStateManager::QuitGame() {
		GameStateManager::SetGameState(nullptr);
	}
}