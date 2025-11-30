/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PauseButtonLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu

 DESCRIPTION:		 Declares the PauseButtonLogic component class, which manages pause menu button
					 interactions (Resume, How-To-Play, Quit) with hover states.

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include "GameObjectLogic.hpp"

enum class PauseAction {
	Resume,
	HowToPlay,
	Quit
};

class PauseButtonLogic final : public GameObjectLogic {
public:
	explicit PauseButtonLogic(int ownerID, PauseAction action)
		: GameObjectLogic(ownerID), action_(action) {
	}

	void Update(float dt, Scene& scene, InputManager& input) override;

private:
	PauseAction action_;
	bool initialized_ = false;
	bool hovered_ = false;
	std::string normalTexturePath_;
	std::string hoverTexturePath_;
};
