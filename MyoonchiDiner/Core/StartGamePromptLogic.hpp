/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			StartGamePromptLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Declares the StartGamePromptLogic component class, which manages the tutorial prompt that appears
					when the player clicks the "Play" button on the main menu. This logic handles mouse input
					to detect clicks on the button, opens a tutorial popup, and manages hover states for visual feedback.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <glm/vec2.hpp>
#include <string>
#include <utility>

#include "Core/AudioManager.hpp"
#include "Core/GameObjectLogic.hpp"

class StartGamePromptLogic final : public GameObjectLogic {
public:
	// Constructor takes explicit target JSON paths and simulation activation.
	StartGamePromptLogic(int ownerID,
		std::string tutorialJson,
		std::string skipJson,
		bool activateSimulation)
		: GameObjectLogic(ownerID)
		, tutorialJson_(std::move(tutorialJson))
		, skipJson_(std::move(skipJson))
		, activateSimulation_(activateSimulation) {
	}

	// Override Update to handle hover state and click interactions
	void Update(float dt, Scene& scene, InputManager& input) override;

	// Set AudioManager for button click sounds
	void SetAudioManager(AudioManager* mgr) {
		audioManager_ = mgr;
	}

private:
	// Helper functions for mouse world conversion and point-in-rect testing
	bool GetMouseWorld(Scene& scene, InputManager& input, glm::vec2& outWorld) const;
	bool IsPointInRect(const glm::vec2& p, const glm::vec2& min, const glm::vec2& max) const;

	// Helper functions to open/close the tutorial prompt, which may involve spawning/despawning GameObjects and setting simulation state.
	void OpenPrompt(Scene& scene);
	void ClosePrompt(Scene& scene);

	// Click targets
	std::string tutorialJson_;
	std::string skipJson_;
	bool activateSimulation_ = false;

	// Popup state
	bool promptOpen_ = false;
	int popupId_ = -1;

	// Hover state cache
	bool initialized_ = false;
	bool hovered_ = false;
	std::string normalTexturePath_;
	std::string hoverTexturePath_;

	// Popup layout constants (center and size in world units)
	glm::vec2 popupCenter_{ 0.0f, 0.0f };
	glm::vec2 popupSize_{ 1152.0f, 648.0f };

	// Audio
	AudioManager* audioManager_ = nullptr;
};