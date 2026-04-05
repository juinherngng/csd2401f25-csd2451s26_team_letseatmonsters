/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PauseButtonLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (85%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (15%)

 DESCRIPTION:		 Declares the PauseButtonLogic component class, which manages pause menu button
					 interactions (Resume, How-To-Play, Quit) with hover states.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "EngineCore/GameObjectLogic.hpp"

 // Enum for different pause menu button actions
enum class PauseAction {
	Resume,
	HowToPlay,
	Quit
};

class PauseButtonLogic final : public GameObjectLogic {
public:
	/**
	 * @brief Constructs pause-menu button logic for a specific button action.
	 * @param ownerID Runtime ID of the button GameObject that owns this logic.
	 * @param action Pause action triggered by this button.
	 */
	explicit PauseButtonLogic(int ownerID, PauseAction action)
		: GameObjectLogic(ownerID), action_(action) {
		// Store the authored action so Update() can dispatch the correct pause-menu behavior.
	}

	/**
	 * @brief Updates hover feedback, keyboard focus, and click handling for one frame.
	 * @param dt Delta time for the frame.
	 * @param scene Active scene containing the pause overlay.
	 * @param input Input manager used for mouse and keyboard interaction.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

private:
	// Authored action this pause button performs when activated.
	PauseAction action_;
	// Tracks whether the button has cached its normal and hover texture paths.
	bool initialized_ = false;
	// True while the button is currently hovered by mouse or keyboard focus.
	bool hovered_ = false;
	// Texture path used when the button is idle.
	std::string normalTexturePath_;
	// Texture path used while the button is highlighted.
	std::string hoverTexturePath_;
};
