/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameBootstrap.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (80%)
 CO-AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (20%)

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

/**
 * @brief Returns the startup splash texture path, or an empty string to disable.
 * @return UTF-8 texture path consumed by the scene background loader.
 */
const char* GetStartupSplashTexturePath();

/**
 * @brief Returns how long the startup splash should be shown in seconds.
 * @return Splash duration in seconds. Values <= 0 disable timed display.
 */
float GetStartupSplashDurationSeconds();
