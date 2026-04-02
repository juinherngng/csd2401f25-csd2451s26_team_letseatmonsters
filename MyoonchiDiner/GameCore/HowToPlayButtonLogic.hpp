/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         HowToPlayButtonLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (80%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu	  (20%)

 DESCRIPTION:       Declares the HowToPlayButtonLogic class, including data
					required for hover detection, overlay spawning, texture
					swapping, and tracking which instance owns the overlay.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/GameObjectLogic.hpp"

 // Logic for the "How To Play" button in the pause menu, which spawns an overlay with instructions when clicked.
class HowToPlayButtonLogic final : public GameObjectLogic {
public:
	/**
	 * @brief Constructs a `HowToPlayButtonLogic` instance.
	 * @param ownerID Parameter for owner id.
	 * @return Result produced by this operation.
	 */
	explicit HowToPlayButtonLogic(int ownerID)
		: GameObjectLogic(ownerID) {
	}

	/**
	 * @brief Updates this object.
	 * @param dt Frame delta time in seconds.
	 * @param scene Scene being processed.
	 * @param input Input manager for the current frame.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	void SetAudioManager(AudioManager* mgr) {
		audioManager_ = mgr;
	}

private:
	// Data members for hover state, texture paths, and overlay tracking
	bool initialized_ = false;
	bool hovered_ = false;
	bool backButtonHovered_ = false;

	// Texture paths for normal and hover states (set in Start)
	std::string normalTexturePath_;
	std::string hoverTexturePath_;

	int overlayId_ = -1; // ID of spawned HowToPlay overlay, -1 if none
	int backButtonId_ = -1;

	AudioManager* audioManager_ = nullptr;
};
