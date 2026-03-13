/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         InputCommandHandler.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu (50%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(50%)

 DESCRIPTION:       Declares InputCommandHandler, which translates high-level keyboard inputs
					into debug toggles and physics-mode switches (forces vs. kinematic).
					It does not own any state; it simply reads input and calls other systems.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "../Graphics/DebugRenderer.hpp"
#include "../Graphics/EntityManager.hpp"

#include "DebugVisualizer.hpp"
#include "InputManager.hpp"
#include "MovementManager.hpp"
#include "PhysicsManager.hpp"

 // Forward declare Scene to avoid circular dependency.
class InputCommandHandler {
public:
	InputCommandHandler() = default;
	~InputCommandHandler() = default;

	// Entry point to process per-frame command inputs.
	void ProcessCommands(InputManager& inputManager,
		PhysicsManager& physicsManager,
		MovementManager& movementManager,
		int playerID,
		bool& useForces,
		bool& showAuxDebug);

private:
	// Handles keys that toggle debug state (G, H).
	void HandleDebugToggles(InputManager& inputManager, bool& showAuxDebug);

	// Handles the physics mode toggle (F) and updates the player's physics component.
	void HandleForceToggle(InputManager& inputManager,
		PhysicsManager& physicsManager,
		MovementManager& movementManager,
		int playerID,
		bool& useForces);
};
