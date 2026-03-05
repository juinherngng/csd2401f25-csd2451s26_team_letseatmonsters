/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         HowToPlayButtonLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Declares the HowToPlayButtonLogic class, including data
					required for hover detection, overlay spawning, texture
					swapping, and tracking which instance owns the overlay.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "GameObjectLogic.hpp"

#include <string>

 // Logic for the "How To Play" button in the pause menu, which spawns an overlay with instructions when clicked.
class HowToPlayButtonLogic final : public GameObjectLogic {
public:
	// Constructor takes ownerID and initializes base GameObjectLogic
	explicit HowToPlayButtonLogic(int ownerID)
		: GameObjectLogic(ownerID) {
	}

	// Override lifecycle methods
	void Update(float dt, Scene& scene, InputManager& input) override;

private:
	// Data members for hover state, texture paths, and overlay tracking
	bool initialized_ = false;
	bool hovered_ = false;

	// Texture paths for normal and hover states (set in Start)
	std::string normalTexturePath_;
	std::string hoverTexturePath_;

	int overlayId_ = -1; // ID of spawned HowToPlay overlay, -1 if none
};
