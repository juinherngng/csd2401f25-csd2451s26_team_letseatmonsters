/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			MenuButtonLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Ng Juin Herng, juinherng.ng@digipen.edu

 DESCRIPTION:		 Declares the MenuButtonLogic component class, which provides interactive 
					 button behavior for menu GameObjects including hover texture swapping and 
					 deferred level loading via Scene::QueueLevelLoad when clicked.

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include "GameObjectLogic.hpp"

class AudioManager;

class MenuButtonLogic final : public GameObjectLogic {
public:
	explicit MenuButtonLogic(int ownerID, std::string targetJson, bool activateSimulation)
		: GameObjectLogic(ownerID),
		  activateSimulation_(activateSimulation) {
		// Determine which state to load based on the JSON path BEFORE moving
		if (targetJson.find("kitchen01") != std::string::npos) {
			stateToLoad_ = 1; // GS_Level2 = gameplay
		}
		else {
			stateToLoad_ = 0; // GS_Level1 = main menu
		}
		// Now store the path after we've checked it
		targetJson_ = std::move(targetJson);
	}

	void Update(float dt, Scene& scene, InputManager& input) override;

	// Set AudioManager for button click sounds
	void SetAudioManager(AudioManager* audioMgr) { audioManager_ = audioMgr; }

private:
	// Click target
	std::string targetJson_;
	bool activateSimulation_ = false;
	int stateToLoad_ = 0;

	// Hover state cache
	bool initialized_ = false;
	bool hovered_ = false;
	std::string normalTexturePath_;
	std::string hoverTexturePath_;

	// Audio
	AudioManager* audioManager_ = nullptr;
};
