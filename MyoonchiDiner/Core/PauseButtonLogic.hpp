/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PauseButtonLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		 Declares the PauseButtonLogic component class, which manages pause menu button
					 interactions (Resume, How-To-Play, Quit) with hover states.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "GameObjectLogic.hpp"
#include <string>

 // Enum for different pause menu button actions
enum class PauseAction {
	Resume,
	HowToPlay,
	Quit
};

// Logic component for pause menu buttons, handling hover states and click interactions
class PauseButtonLogic final : public GameObjectLogic {
public:
	// Constructor takes the owner GameObject ID and the specific action this button represents
	explicit PauseButtonLogic(int ownerID, PauseAction action)
		: GameObjectLogic(ownerID), action_(action) {
	}

	// Override lifecycle methods to manage hover state and trigger actions on click
	void Update(float dt, Scene& scene, InputManager& input) override;

private:
	// Helper method to perform the assigned action when the button is clicked
	PauseAction action_;
	bool initialized_ = false;
	bool hovered_ = false;
	std::string normalTexturePath_;
	std::string hoverTexturePath_;
};

