/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			MenuButtonLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (95%)
 CO-AUTHORS:		Ng Juin Herng, juinherng.ng@digipen.edu (5%)

 DESCRIPTION:		 Declares the MenuButtonLogic component class, which provides interactive
					 button behavior for menu GameObjects including hover texture swapping and
					 deferred level loading via Scene::QueueLevelLoad when clicked.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "GameObjectLogic.hpp"

#include <string>

class AudioManager;

// Logic component for menu buttons that load levels or toggle simulation when clicked.
class MenuButtonLogic final : public GameObjectLogic {
public:
	// Constructor takes explicit target JSON path and simulation activation.
	explicit MenuButtonLogic(int ownerID, std::string targetJson, bool activateSimulation)
		: GameObjectLogic(ownerID),
		targetJson_(std::move(targetJson)),
		activateSimulation_(activateSimulation) {
	}

	// Override Update to handle hover state and click interactions
	void Update(float dt, Scene& scene, InputManager& input) override;

	// Set AudioManager for button click sounds
	void SetAudioManager(AudioManager* audioMgr) {
		audioManager_ = audioMgr;
	}

private:
	// Click target
	std::string targetJson_;
	bool activateSimulation_ = false;

	// Hover state cache
	bool initialized_ = false;
	bool hovered_ = false;
	std::string normalTexturePath_;
	std::string hoverTexturePath_;

	// Audio
	AudioManager* audioManager_ = nullptr;
};
