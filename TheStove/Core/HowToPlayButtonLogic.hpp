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

class HowToPlayButtonLogic final : public GameObjectLogic {
public:
	explicit HowToPlayButtonLogic(int ownerID)
		: GameObjectLogic(ownerID) {
	}

	void Update(float dt, Scene& scene, InputManager& input) override;

private:
	bool initialized_ = false;
	bool hovered_ = false;

	std::string normalTexturePath_;
	std::string hoverTexturePath_;

	int overlayId_ = -1; // ID of spawned HowToPlay overlay, -1 if none
};
