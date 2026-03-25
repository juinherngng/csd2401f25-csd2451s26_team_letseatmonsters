/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameBootstrap.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		Declares the game-side bootstrap interface that the engine
					calls at startup. Each function is implemented by the active
					game project (MyoonchiDiner) and allows the game to inject
					its bindings, state mappings, and audio policies without
					the engine depending on any game-specific types.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

class Scene;

namespace Framework {
	class GameStateManager;
}

/************************************************************************/
/*!
\brief
	Registers all game-specific logic bindings, hooks, and policies
	onto the given Scene. Delegates to the active game module.
\param scene
	The engine Scene to bind game hooks into.
*/
/************************************************************************/
void RegisterGameBindings(Scene& scene);

/************************************************************************/
/*!
\brief
	Maps engine game-state IDs (GS_Level1, GS_Level2, etc.) to their
	corresponding JSON level files so the GameStateManager can load
	them at runtime.
\param gsm
	The GameStateManager to register state-to-level mappings on.
*/
/************************************************************************/
void ConfigureGameStates(Framework::GameStateManager& gsm);

/************************************************************************/
/*!
\brief
	Injects the audio playback and pause/resume policies that the
	GameStateManager uses when transitioning between states or when
	the simulation is paused/resumed.
\param gsm
	The GameStateManager to attach audio policies to.
*/
/************************************************************************/
void ConfigureGameStateAudioPolicy(Framework::GameStateManager& gsm);

