/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			ApplicationFlowCoordinator.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements the application-owned coordinator responsible for high-level
					game-state fade transitions between Scene and GameStateManager.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "ApplicationFlowCoordinator.hpp"

#include "../Graphics/GraphicsEngine.hpp"

#include "InputManager.hpp"
#include "Logger.hpp"

 /**
  * @brief Queues a state transition and starts a fade if one is not already active.
  * @param newState Requested destination game state.
  * @param graphicsEngine Graphics system used to drive the fade transition.
  */
void ApplicationFlowCoordinator::QueueStateChange(Framework::GameState newState, GraphicsEngine& graphicsEngine) {
	// Always keep the latest requested destination so the app only commits the most recent request.
	pendingStateAfterFade_ = newState;

	if (!graphicsEngine.IsTransitionActive()) {
		graphicsEngine.StartSceneTransition(0.35f, 0.35f);
	}

	TS_LOG_DEBUG("[ApplicationFlowCoordinator] Queued state change " << Framework::ToString(newState) << " to run at blackout");
}

/**
 * @brief Advances the pending transition and commits the state switch at blackout.
 * @param gsm Game-state manager that performs the actual state switch.
 * @param graphicsEngine Graphics system used to query and continue the fade.
 * @param inputMgr Optional input manager used to clear carry-over input after switching states.
 * @param frameDt Frame delta time used for the state-switch callback.
 */
void ApplicationFlowCoordinator::Update(Framework::GameStateManager& gsm, GraphicsEngine& graphicsEngine, InputManager* inputMgr, float frameDt) {
	if (!pendingStateAfterFade_.has_value() || !graphicsEngine.IsAtBlackout()) {
		return;
	}

	// Commit the state switch only when the screen is fully black to avoid visible popping.
	TS_LOG_INFO("[ApplicationFlowCoordinator] Blackout reached; switching to state " << Framework::ToString(*pendingStateAfterFade_));
	gsm.UpdateGameState(*pendingStateAfterFade_, frameDt);
	graphicsEngine.ContinueTransitionFadeIn();
	pendingStateAfterFade_.reset();

	// Clear carry-over input so menu clicks do not leak into the next state.
	if (inputMgr != nullptr) {
		inputMgr->ClearState();
	}
}

/**
 * @brief Clears any queued transition state without performing a switch.
 */
void ApplicationFlowCoordinator::Reset() {
	pendingStateAfterFade_.reset();
}
