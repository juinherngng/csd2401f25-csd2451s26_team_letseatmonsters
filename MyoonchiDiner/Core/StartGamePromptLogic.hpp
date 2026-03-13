/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			StartGamePromptLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Declares `StartGamePromptLogic`, the click/hover controller for the main menu
					`btn_play` flow.


	Responsibilities:
	  1) Handles hover-state sprite swapping for the main Play button.
	  2) Opens/closes the tutorial decision popup.
	  3) Spawns and controls popup Yes/No button visuals and hover states.
	  4) Routes click actions to either:
		 - tutorial level transition (Yes), or
		 - intro cutscene sequence then level transition (No).

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
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

	// Per-frame input + UI state update.
	void Update(float dt, Scene& scene, InputManager& input) override;

	// Set AudioManager for button click sounds
	void SetAudioManager(AudioManager* mgr) {
		audioManager_ = mgr;
	}

private:
	// Converts current cursor position into scene/world coordinates.
	bool GetMouseWorld(Scene& scene, InputManager& input, glm::vec2& outWorld) const;

	// Axis-aligned rectangle hit test in world-space.
	bool IsPointInRect(const glm::vec2& p, const glm::vec2& min, const glm::vec2& max) const;

	// Spawns popup + decision buttons and initializes popup state.
	void OpenPrompt(Scene& scene);

	// Despawns popup + decision buttons and resets popup state.
	void ClosePrompt(Scene& scene);

	// Level targets selected by the popup.
	std::string tutorialJson_;
	std::string skipJson_;
	bool activateSimulation_ = false;

	// Popup runtime state.
	bool promptOpen_ = false;
	int popupId_ = -1;
	int yesButtonId_ = -1;
	int noButtonId_ = -1;
	bool yesHovered_ = false;
	bool noHovered_ = false;

	// Hover texture state for the main Play button.
	bool initialized_ = false;
	bool hovered_ = false;
	std::string normalTexturePath_;
	std::string hoverTexturePath_;

	// Popup transform authoring.
	glm::vec2 popupCenter_{ 0.0f, 0.0f };
	glm::vec2 popupSize_{ 1152.0f, 648.0f };

	// Audio
	AudioManager* audioManager_ = nullptr;
};