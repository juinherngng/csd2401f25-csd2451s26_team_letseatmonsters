/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			StartGamePromptLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (85%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (15%)

 DESCRIPTION:		Declares the main-menu Play button prompt flow.
					The logic controls the Play button hover state, manages the tutorial
					prompt, and routes the player's choice into either the tutorial or
					the intro-video path before gameplay begins.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <glm/vec2.hpp>
#include <string>
#include <utility>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/GameObjectLogic.hpp"

/**
 * @class StartGamePromptLogic
 * @brief Handles the Play button and its tutorial-choice modal popup.
 */
class StartGamePromptLogic final : public GameObjectLogic {
public:
	/**
	 * @brief Constructs the Play-button prompt controller.
	 * @param ownerID Scene object id for the Play button sprite.
	 * @param tutorialJson Level JSON path used when the player chooses the tutorial.
	 * @param skipJson Level JSON path used after the intro-video path completes.
	 * @param activateSimulation Whether the destination level should start with simulation enabled.
	 */
	StartGamePromptLogic(int ownerID,
		std::string tutorialJson,
		std::string skipJson,
		bool activateSimulation)
		: GameObjectLogic(ownerID)
		, tutorialJson_(std::move(tutorialJson))
		, skipJson_(std::move(skipJson))
		, activateSimulation_(activateSimulation) {
		// Persist both possible destinations so the popup can route either choice immediately.
	}

	/**
	 * @brief Updates the Play button or tutorial prompt depending on the current modal state.
	 * @param dt Unused frame delta time in seconds.
	 * @param scene Active scene providing UI spawning and transition helpers.
	 * @param input Frame input snapshot used for hover, click, and keyboard submit behavior.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Injects the audio manager used for UI sound playback.
	 * @param mgr Non-owning pointer to the shared audio manager.
	 */
	void SetAudioManager(AudioManager* mgr) {
		// Cache the audio bridge so button hover and click feedback can be played on demand.
		audioManager_ = mgr;
	}

private:
	/**
	 * @brief Converts the current cursor position into scene-space coordinates.
	 * @param scene Active scene, unused because the graphics singleton resolves the world position.
	 * @param input Input manager used as the fallback world-space conversion path.
	 * @param outWorld Output cursor position in reference-scene coordinates.
	 * @return `true` when a usable world position was produced.
	 */
	bool GetMouseWorld(Scene& scene, InputManager& input, glm::vec2& outWorld) const;

	/**
	 * @brief Tests whether a point lies inside a world-space axis-aligned rectangle.
	 * @param p Point to test.
	 * @param min Minimum corner of the rectangle.
	 * @param max Maximum corner of the rectangle.
	 * @return `true` when the point lies within the rectangle bounds.
	 */
	bool IsPointInRect(const glm::vec2& p, const glm::vec2& min, const glm::vec2& max) const;

	/**
	 * @brief Spawns the tutorial-choice popup and its Yes/No buttons.
	 * @param scene Active scene used for UI object spawning.
	 */
	void OpenPrompt(Scene& scene);

	/**
	 * @brief Closes the tutorial-choice popup and clears its runtime state.
	 * @param scene Active scene used for UI object despawning.
	 */
	void ClosePrompt(Scene& scene);

	// Level JSON path queued when the player chooses the tutorial.
	std::string tutorialJson_;
	// Level JSON path queued after the intro-video path completes.
	std::string skipJson_;
	// Whether the chosen destination should start with gameplay simulation enabled.
	bool activateSimulation_ = false;

	// Tracks whether the tutorial-choice prompt is currently visible.
	bool promptOpen_ = false;
	// Runtime id of the popup background sprite.
	int popupId_ = -1;
	// Runtime id of the Yes button sprite.
	int yesButtonId_ = -1;
	// Runtime id of the No button sprite.
	int noButtonId_ = -1;
	// Tracks whether the Yes button is currently highlighted.
	bool yesHovered_ = false;
	// Tracks whether the No button is currently highlighted.
	bool noHovered_ = false;

	// Tracks whether the Play-button texture paths have been initialized.
	bool initialized_ = false;
	// Tracks whether the Play button is currently highlighted.
	bool hovered_ = false;
	// Cached idle texture path for the Play button.
	std::string normalTexturePath_;
	// Cached hover texture path for the Play button.
	std::string hoverTexturePath_;

	// Cached popup center in reference-scene coordinates.
	glm::vec2 popupCenter_{ 0.0f, 0.0f };
	// Authored popup size used for spawn and button-position calculations.
	glm::vec2 popupSize_{ 1152.0f, 648.0f };

	// Optional audio bridge used for hover, click, and transition sounds.
	AudioManager* audioManager_ = nullptr;
};
