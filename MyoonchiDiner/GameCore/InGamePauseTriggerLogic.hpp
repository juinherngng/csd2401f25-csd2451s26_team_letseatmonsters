/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         InGamePauseTriggerLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu

 DESCRIPTION:       Declares logic for an in game pause trigger UI element. This component
					implements click behaviour for a pause button in the game world, including
					swapping texture when hovered and triggering the pause state on activation.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "EngineCore/GameObjectLogic.hpp"

/**
 * @brief Runtime logic for the in-game pause button shown during gameplay.
 * @details
 * Tracks hover state for the authored pause trigger sprite and requests the
 * scene pause overlay when the button is activated by the player.
 */
class InGamePauseTriggerLogic final : public GameObjectLogic {
public:
	/**
	 * @brief Constructs an in-game pause trigger controller for the specified owner object.
	 * @param ownerID Runtime ID of the pause-trigger GameObject.
	 */
	explicit InGamePauseTriggerLogic(int ownerID)
		: GameObjectLogic(ownerID) {
		// Forward the owner ID to the base logic so this controller can resolve its trigger sprite.
	}

	/**
	 * @brief Updates hover feedback and pause activation for one frame.
	 * @param dt Delta time for the frame.
	 * @param scene Active scene containing the trigger object and pause overlay.
	 * @param input Input manager used for cursor position and click detection.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

private:
	// Tracks whether the trigger has cached its normal and hover textures yet.
	bool initialized_ = false;

	// True while the cursor is currently inside the trigger's hit area.
	bool hovered_ = false;

	// Texture path used when the trigger is idle.
	std::string normalTexturePath_{};

	// Texture path used while the trigger is highlighted by the cursor.
	std::string hoverTexturePath_{};
};
