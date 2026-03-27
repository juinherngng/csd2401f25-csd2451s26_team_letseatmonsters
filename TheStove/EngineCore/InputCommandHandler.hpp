/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         InputCommandHandler.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu (50%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(50%)

 DESCRIPTION:       Declares InputCommandHandler, which translates high-level keyboard inputs
					into debug toggles and physics-mode switches (forces vs. kinematic).
					It does not own any state; it simply reads input and calls other systems.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "EngineCore/DebugVisualizer.hpp"
#include "EngineCore/InputManager.hpp"
#include "EngineCore/MovementManager.hpp"
#include "EngineCore/PhysicsManager.hpp"
#include "EngineGraphics/DebugRenderer.hpp"
#include "EngineGraphics/EntityManager.hpp"

 // Forward declare Scene to avoid circular dependency.
class InputCommandHandler {
public:

	/**
	 * @brief Constructs a `InputCommandHandler` instance.
	 */
	InputCommandHandler() = default;

	/**
	 * @brief Destroys the `InputCommandHandler` instance and releases owned resources.
	 */
	~InputCommandHandler() = default;

	/**
	 * @brief Processes commands.
	 * @param inputManager Input manager for the current frame.
	 * @param physicsManager Physics manager used for physics updates.
	 * @param movementManager Movement manager used for movement updates.
	 * @param playerID Identifier of the player object.
	 * @param useForces Parameter for use forces.
	 * @param showAuxDebug Parameter for show aux debug.
	 */
	void ProcessCommands(InputManager& inputManager,
		PhysicsManager& physicsManager,
		MovementManager& movementManager,
		int playerID,
		bool& useForces,
		bool& showAuxDebug);

private:

	/**
	 * @brief Handles debug toggles.
	 * @param inputManager Input manager for the current frame.
	 * @param showAuxDebug Parameter for show aux debug.
	 */
	void HandleDebugToggles(InputManager& inputManager, bool& showAuxDebug);

	/**
	 * @brief Handles force toggle.
	 * @param inputManager Input manager for the current frame.
	 * @param physicsManager Physics manager used for physics updates.
	 * @param movementManager Movement manager used for movement updates.
	 * @param playerID Identifier of the player object.
	 * @param useForces Parameter for use forces.
	 */
	void HandleForceToggle(InputManager& inputManager,
		PhysicsManager& physicsManager,
		MovementManager& movementManager,
		int playerID,
		bool& useForces);
};
