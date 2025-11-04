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
#include "MessageBus.hpp"
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
	
	class GameStateManager : public CoreFramework::SystemInterface
	{
	public:
		/************************************************************************/
		/*!
		\brief
		Constructs the GameStateManager with MessageBus reference.
		\param bus
		Reference to the MessageBus for pub/sub messaging.
		*/
		/************************************************************************/
		GameStateManager(CoreFramework::MessageBus& bus);
		
		/************************************************************************/
		/*!
		\brief
		Destroys the GameStateManager and unsubscribes from messages.
		*/
		/************************************************************************/
		~GameStateManager();
		
		//Setup Manager Logic
		void Initialize() override;
		//Manager Update loop
		void Update(float dt) override;
		//Get System Name
		std::string GetName() override;
		//Set Default Game State before use in Update
		void InitializeGameState(int GS, float dt);
		//Call function pointer to state update
		void UpdateGameState(int newState, float dt);
		
	private:
		// Message handlers
		void OnQuit(const CoreFramework::Message& msg);
		
		// Pub/sub
		CoreFramework::MessageBus& messageBus;
		CoreFramework::SubscriberId quitSubId;
	};
}
