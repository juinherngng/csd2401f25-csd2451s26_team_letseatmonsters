/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GameStateManager.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Darren Toh, darren.toh@digipen.edu

DESCRIPTION:		Game State Manager interface derived from System.hpp.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#include "System.hpp"
#include "TestLevel.cpp"
#include "TestLevel2.cpp"
#include <memory>
#include <functional> 

namespace Framework {
	enum GameState {
		GS_Level1,
		GS_Level2,
		GS_Quit
	};
	//Ints representing Game States being cycled on update
	extern int currentGS = 0, nextGS = 0;

	////Check if game state has been entered and initialised
	extern bool init = false;

	//Smart Function Pointers for interchanging functionality for game states
	typedef std::function<void(float dt)> FP;

	extern FP fpInit = nullptr, fpUpdate = nullptr, fpExit = nullptr; // Function pointers that changes depending on what state the game is in currently
	class GameStateManager : public Framework::SystemInterface
	{
	public:
		//Setup Manager Logic
		void Initialize() override {
		}
		//Manager Update loop
		void Update(float dt) override {
			if (!init) {
				InitializeGameState(0,dt);
			}
			if (currentGS == nextGS) {
				fpUpdate(dt);
			}
		}

		void SendMessage(Framework::Message* msg) override {

		}

		std::string GetName() override {
			return "GameStateManager";
		}

		void InitializeGameState(int GS, float dt) {
			nextGS = currentGS = GS;
			fpInit = Level1Init;
			fpUpdate = Level1Update;
			fpExit = Level1Exit;

			fpInit(dt);
			init = true;
		}

		void UpdateGameState(int newState, float dt) {
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
	};
}
