/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GameStateManager.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Darren Toh, darren.toh@digipen.edu

DESCRIPTION:		Game State Manager interface derived from System.hpp. Uses 3 Function pointers
					and redirects them to level/scene-specific init, update and exit functions.
					These function pointers are then called in main by the game state manager.
					This is a header file for declarations.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#include "System.hpp"
#include "TestLevel.hpp"
#include "TestLevel2.hpp"
#include <memory>
#include <functional> 

namespace Framework {
	enum GameState {
		GS_Level1,
		GS_Level2,
		GS_Quit
	};
	//Ints representing Game States being cycled on update
	extern int currentGS, nextGS;

	////Check if game state has been entered and initialised
	extern bool init;

	//Smart Function Pointers for interchanging functionality for game states
	typedef std::function<void(float dt)> FP;

	extern FP fpInit, fpUpdate, fpExit; // Function pointers that changes depending on what state the game is in currently
	class GameStateManager : public Framework::SystemInterface
	{
	public:
		//Setup Manager Logic
		void Initialize() override;
		//Manager Update loop
		void Update(float dt) override;

		void SendMessage(Framework::Message* msg);

		std::string GetName() override;

		void InitializeGameState(int GS, float dt);

		void UpdateGameState(int newState, float dt);
	};
}
