/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         InGamePauseTriggerLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu

 DESCRIPTION:       Declares logic for an in‑game pause trigger UI element. This component 
					implements click behaviour for a pause button in the game world, including 
					swapping texture when hovered and triggering the pause state on activation.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "EngineCore/GameObjectLogic.hpp"

class InGamePauseTriggerLogic final : public GameObjectLogic {
public:
	explicit InGamePauseTriggerLogic(int ownerID)
		: GameObjectLogic(ownerID) {}

	void Update(float dt, Scene& scene, InputManager& input) override;

private:
	/// Tracks whether one‑time initialization has been completed.
	bool initialized_ = false;

	/// True when the cursor is currently hovering over the trigger.
	bool hovered_ = false;

	/// File path for the default (non‑hovered) texture of the trigger. 
	std::string normalTexturePath_{};

	/// File path for the texture used while the trigger is hovered. 
	std::string hoverTexturePath_{};
};
