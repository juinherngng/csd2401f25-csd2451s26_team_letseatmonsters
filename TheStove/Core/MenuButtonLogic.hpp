/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			MenuButtonLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu

 DESCRIPTION:		 Declares the MenuButtonLogic component class, which provides interactive 
					 button behavior for menu GameObjects including hover texture swapping and 
					 deferred level loading via Scene::QueueLevelLoad when clicked.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include "GameObjectLogic.hpp"

class MenuButtonLogic final : public GameObjectLogic {
public:
	explicit MenuButtonLogic(int ownerID, std::string targetJson, bool activateSimulation)
		: GameObjectLogic(ownerID),
		  targetJson_(std::move(targetJson)),
		  activateSimulation_(activateSimulation) {
	}

	void Update(float dt, Scene& scene, InputManager& input) override;

private:
	// Click target
	std::string targetJson_;
	bool activateSimulation_ = false;

	// Hover state cache
	bool initialized_ = false;
	bool hovered_ = false;
	std::string normalTexturePath_;
	std::string hoverTexturePath_;
};
