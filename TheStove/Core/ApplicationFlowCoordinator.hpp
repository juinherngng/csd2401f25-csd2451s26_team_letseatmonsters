/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			ApplicationFlowCoordinator.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Declares the application-owned coordinator responsible for high-level
					game-state fade transitions between Scene and GameStateManager.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "GameStateManager.hpp"

#include <optional>

class GraphicsEngine;
class InputManager;

/**
 * @brief Owns app-level state-transition orchestration between scene requests and state changes.
 */
class ApplicationFlowCoordinator {
public:
	/**
	 * @brief Returns whether a fade-driven state transition is currently pending.
	 * @return `true` when the coordinator is waiting to switch states at blackout.
	 */
	bool HasPendingTransition() const {
		return pendingStateAfterFade_.has_value();
	}

	/**
	 * @brief Queues a state transition and starts a fade if one is not already active.
	 * @param newState Requested destination game state.
	 * @param graphicsEngine Graphics system used to drive the fade transition.
	 */
	void QueueStateChange(Framework::GameState newState, GraphicsEngine& graphicsEngine);

	/**
	 * @brief Advances the pending transition and commits the state switch at blackout.
	 * @param gsm Game-state manager that performs the actual state switch.
	 * @param graphicsEngine Graphics system used to query and continue the fade.
	 * @param inputMgr Optional input manager used to clear carry-over input after switching states.
	 * @param frameDt Frame delta time used for the state-switch callback.
	 */
	void Update(Framework::GameStateManager& gsm, GraphicsEngine& graphicsEngine, InputManager* inputMgr, float frameDt);

	/**
	 * @brief Clears any queued transition state without performing a switch.
	 */
	void Reset();

private:
	std::optional<Framework::GameState> pendingStateAfterFade_;
};
