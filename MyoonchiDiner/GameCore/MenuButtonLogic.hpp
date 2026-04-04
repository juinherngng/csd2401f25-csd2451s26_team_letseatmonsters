/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			MenuButtonLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (95%)
 CO-AUTHORS:		Ng Juin Herng, juinherng.ng@digipen.edu (5%)

 DESCRIPTION:		Declares the menu button logic used by top-level menu entries.
					This component owns the hover-state texture swap, optional UI audio
					feedback, and deferred level-transition requests for menu buttons.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <utility>

#include "EngineCore/GameObjectLogic.hpp"

class AudioManager;

/**
 * @class MenuButtonLogic
 * @brief Implements hover and activation behavior for a menu button sprite.
 */
class MenuButtonLogic final : public GameObjectLogic {
public:
	/**
	 * @brief Constructs a menu button controller for a specific owner object.
	 * @param ownerID Scene object id that owns this logic component.
	 * @param targetJson Level JSON path to load when the button is activated.
	 * @param activateSimulation Whether the target level should start with simulation enabled.
	 */
	explicit MenuButtonLogic(int ownerID, std::string targetJson, bool activateSimulation)
		: GameObjectLogic(ownerID),
		targetJson_(std::move(targetJson)),
		activateSimulation_(activateSimulation) {
		// Persist the authored destination so activation can transition immediately.
	}

	/**
	 * @brief Updates hover, keyboard focus, and click activation for the owner button.
	 * @param dt Unused frame delta time in seconds.
	 * @param scene Active scene providing object state and transition helpers.
	 * @param input Frame input snapshot used for pointer and keyboard activation.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Injects the audio manager used for hover and click sound effects.
	 * @param audioMgr Non-owning pointer to the shared audio manager.
	 */
	void SetAudioManager(AudioManager* audioMgr) {
		// Cache the shared audio bridge so the button can play UI feedback sounds on demand.
		audioManager_ = audioMgr;
	}

private:
	// Level JSON path queued when the button is activated.
	std::string targetJson_;
	// Whether the destination scene should resume gameplay simulation.
	bool activateSimulation_ = false;

	// Tracks whether the button texture paths have been initialized from scene metadata.
	bool initialized_ = false;
	// Tracks whether the button is currently highlighted.
	bool hovered_ = false;
	// Cached default sprite path used when the button is idle.
	std::string normalTexturePath_;
	// Cached highlighted sprite path used while the button is hot.
	std::string hoverTexturePath_;

	// Optional audio bridge for hover and click sound playback.
	AudioManager* audioManager_ = nullptr;
};
