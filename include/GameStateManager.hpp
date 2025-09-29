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
#include "GameState.hpp"
#include <memory>

namespace Framework {
	////Ints representing Game States being cycled on update
	//extern int currentGS, previousGS, nextGS;

	//Check if game state has been entered and initialised
	extern bool init;

	//Smart Function Pointers for interchanging functionality for game states
	typedef std::unique_ptr<GameState> FP;
	class GameStateManager : public Framework::SystemInterface
	{
		public:
			//Setup Manager Logic
			void Initialize() override;
			//Manager Update loop
			void Update(float dt) override;
			//Message Sending
			void SendMessage(Framework::Message* msg) override;
			//Get Name of Manager
			std::string GetName() override;
			//Get Name of Current State
			std::string GetGameState();

			//Check if there is a current state
			bool HasState() const;

			//Calls StateExit function of Current State
			//Sets Current State to param
			//Then Calls StateInit function of Current State
			void SetGameState(FP gameState);

			//Call StateUpdate Function of Current State
			//Should be done each frame
			void UpdateGameState();

			//Should only be called to exit out of game loop
			void QuitGame();

		private:
			FP currentState; //Function Pointer to hold GameState functions
	};
}