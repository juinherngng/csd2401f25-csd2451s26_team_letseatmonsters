/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         InputCommandHandler.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu (50%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(50%)

 DESCRIPTION:       Implements InputCommandHandler. Maps keyboard input to engine/debug actions:
					collider/debug visibility toggles and force-mode switching for the player.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "InputCommandHandler.hpp"

#include <iostream>

void InputCommandHandler::ProcessCommands(InputManager& inputManager,
	PhysicsManager& physicsManager,
	MovementManager& movementManager,
	int playerID,
	bool& useForces,
	bool& showAuxDebug) {
	HandleDebugToggles(inputManager, showAuxDebug);
	HandleForceToggle(inputManager, physicsManager, movementManager, playerID, useForces);
}

void InputCommandHandler::HandleDebugToggles(InputManager& inputManager, bool& showAuxDebug) {
	// Toggle collider visualization
	if (inputManager.IsKeyJustPressed(GLFW_KEY_G)) {
		DebugRenderer::SetEnabled(!DebugRenderer::IsEnabled());

		std::cout << "[DebugRenderer] Collider visibility: "
			<< (DebugRenderer::IsEnabled() ? "ON" : "OFF")
			<< std::endl;
	}

	// Toggle auxiliary debug visuals
	if (inputManager.IsKeyJustPressed(GLFW_KEY_H)) {
		showAuxDebug = !showAuxDebug;

		std::cout << "[Debug] Auxiliary visuals: "
			<< (showAuxDebug ? "ON" : "OFF")
			<< std::endl;
	}
}

void InputCommandHandler::HandleForceToggle(InputManager& inputManager,
	PhysicsManager& physicsManager,
	MovementManager& movementManager,
	int playerID,
	bool& useForces) {
	// Toggle physics forces vs. kinematic
	if (inputManager.IsKeyJustPressed(GLFW_KEY_F)) {
		useForces = !useForces;
		std::cout << "[Forces] " << (useForces ? "ON" : "OFF") << std::endl;

		// Hide click-to-move path line while force mode is ON
		DebugVisualizer::SetDrawPathLine(!useForces);

		if (playerID >= 0) {
			if (useForces) {
				physicsManager.EnablePhysics(playerID, 1.0f);

				// Cancel click-to-move so the line disappears immediately
				movementManager.ClearMoveTarget(playerID);
			}
			else {
				physicsManager.DisablePhysics(playerID);
				// (no need to do anything to click target here)
			}
		}
	}
}
