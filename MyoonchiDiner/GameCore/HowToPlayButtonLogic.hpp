/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         HowToPlayButtonLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu	(45%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(20%)
					Seah Wang Hua, wanghua.seah@digipen.edu (5%)
					Ng Juin Herng, juinherng.ng@digipen.edu (20%)

 DESCRIPTION:       Declares the menu logic for the How To Play overlay flow.
					The logic owns the base button hover state, the fullscreen
					instructional overlay, and the overlay-local next-button navigation
					across tutorial pages.

		All content Â© 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/GameObjectLogic.hpp"

 /**
  * @class HowToPlayButtonLogic
  * @brief Controls the How To Play button and the instructional overlay it spawns.
  */
class HowToPlayButtonLogic final : public GameObjectLogic {
public:
	/**
	 * @brief Constructs a How To Play button controller.
	 * @param ownerID Scene object id that owns this logic component.
	 */
	explicit HowToPlayButtonLogic(int ownerID)
		: GameObjectLogic(ownerID) {
		// Store the owner id in the base logic class so updates can resolve the button object later.
	}

	/**
	 * @brief Updates the base button or overlay-next-button interaction state.
	 * @param dt Unused frame delta time in seconds.
	 * @param scene Active scene providing overlay spawning and button state.
	 * @param input Frame input snapshot used for hover, click, and keyboard submit handling.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Injects the audio manager used for UI hover and click feedback.
	 * @param mgr Non-owning pointer to the shared audio manager.
	 */
	void SetAudioManager(AudioManager* mgr) {
		// Cache the shared audio bridge so the overlay can play its UI sound cues.
		audioManager_ = mgr;
	}

private:
	// Tracks whether the base button texture paths have been initialized.
	bool initialized_ = false;
	// Tracks whether the base How To Play button is currently highlighted.
	bool hovered_ = false;
	// Tracks whether the overlay next button is currently highlighted.
	bool nextButtonHovered_ = false;

	// Cached idle texture path for the base button.
	std::string normalTexturePath_;
	// Cached hover texture path for the base button.
	std::string hoverTexturePath_;

	// Runtime id of the spawned overlay sprite, or `-1` when inactive.
	int overlayId_ = -1;
	// Runtime id of the overlay-local next button, or `-1` when inactive.
	int nextButtonId_ = -1;
	// Zero-based page index into the authored tutorial overlay textures.
	int currentPage_ = 0;

	// Optional audio bridge used for menu hover and click sounds.
	AudioManager* audioManager_ = nullptr;
};
