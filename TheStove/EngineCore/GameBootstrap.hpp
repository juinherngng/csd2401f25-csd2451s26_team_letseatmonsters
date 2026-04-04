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

/**
 * @brief Registers all game-specific logic bindings, hooks, and policies onto a scene.
 * @param scene Engine scene to bind game hooks into.
 */
void RegisterGameBindings(Scene& scene);

/**
 * @brief Maps engine game states to their corresponding JSON level files.
 * @param gsm GameStateManager receiving the state-to-level mappings.
 */
void ConfigureGameStates(Framework::GameStateManager& gsm);

/**
 * @brief Configures the game-state audio and pause/resume policies.
 * @param gsm GameStateManager receiving the audio policies.
 */
void ConfigureGameStateAudioPolicy(Framework::GameStateManager& gsm);
