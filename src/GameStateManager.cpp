/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GameStateManager.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Darren Toh, darren.toh@digipen.edu

DESCRIPTION:		Game State Manager interface derived from System.hpp.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
//#include "GameStateManager.hpp"
//#include "TestLevel.cpp"
//#include "TestLevel2.cpp"

//namespace Framework {
//
//	void GameStateManager::InitializeGameState(int GS)
//	{
//		nextGS = currentGS = GS;
//	}
//
//	void GameStateManager::Initialize() {
//		GameStateManager::InitializeGameState(0);
//	}
//	//Manager Update loop
//	void GameStateManager::Update(float dt)
//	{
//		if (currentGS == nextGS) {
//			fpUpdate(dt);
//		}
//	}
//	//Message Sending
//	void GameStateManager::SendMessage(Framework::Message* msg) {}
//	//Get Name of Manager
//	std::string GameStateManager::GetName() { return "GameStateManager"; }
//
//	//Initialize default values
//	bool init = false;
//
//	void UpdateGameState(int newState, float dt) {
//		nextGS = newState;
//		fpExit(dt);
//		currentGS = newState;
//		switch (currentGS) {
//		case GS_Level1:
//			fpInit = Level1Init;
//			fpUpdate = Level1Update;
//			fpExit = Level1Exit;
//
//			fpInit(dt);
//			break;
//		case GS_Level2:
//			fpInit = Level2Init;
//			fpUpdate = Level2Update;
//			fpExit = Level2Exit;
//
//			fpInit(dt);
//			break;
//		case GS_Quit:
//			break;
//		}
//	}
//}