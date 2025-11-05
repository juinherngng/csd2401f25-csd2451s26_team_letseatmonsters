#pragma once

#include "InputManager.hpp"
#include "PhysicsManager.hpp"
#include "../Graphics/DebugRenderer.hpp"
#include "../Graphics/EntityManager.hpp"

/**
 * @brief Handles high-level input commands (toggles, shortcuts)
 *
 * Responsibilities:
 * - Process debug toggle keys (R, T, L)
 * - Handle gameplay mode switches (F for forces)
 * - Provide clean interface for command handling
 */
class InputCommandHandler {
public:
    InputCommandHandler() = default;

    // Process all input commands this frame
    void ProcessCommands(InputManager& inputManager,
        PhysicsManager& physicsManager,
        int playerID,
        bool& useForces,
        bool& showAuxDebug);

private:
    // Individual command handlers
    void HandleDebugToggles(InputManager& inputManager, bool& showAuxDebug);
    void HandleForceToggle(InputManager& inputManager,
        PhysicsManager& physicsManager,
        int playerID,
        bool& useForces);
};

