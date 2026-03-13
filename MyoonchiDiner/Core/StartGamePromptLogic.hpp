#pragma once

#include <string>
#include <utility>
#include <glm/vec2.hpp>

#include "Core/GameObjectLogic.hpp"
#include "Core/AudioManager.hpp"

class StartGamePromptLogic final : public GameObjectLogic {
public:
	StartGamePromptLogic(int ownerID,
		std::string tutorialJson,
		std::string skipJson,
		bool activateSimulation)
		: GameObjectLogic(ownerID)
		, tutorialJson_(std::move(tutorialJson))
		, skipJson_(std::move(skipJson))
		, activateSimulation_(activateSimulation) {
	}

	void Update(float dt, Scene& scene, InputManager& input) override;

	void SetAudioManager(AudioManager* mgr) { audioManager_ = mgr; }

private:
	bool GetMouseWorld(Scene& scene, InputManager& input, glm::vec2& outWorld) const;
	bool IsPointInRect(const glm::vec2& p, const glm::vec2& min, const glm::vec2& max) const;

	void OpenPrompt(Scene& scene);
	void ClosePrompt(Scene& scene);

	std::string tutorialJson_;
	std::string skipJson_;
	bool activateSimulation_ = false;

	bool promptOpen_ = false;
	int popupId_ = -1;

	// hover texture state
	bool initialized_ = false;
	bool hovered_ = false;
	std::string normalTexturePath_;
	std::string hoverTexturePath_;

	glm::vec2 popupCenter_{ 0.0f, 0.0f };
	glm::vec2 popupSize_{ 1152.0f, 648.0f }; 

	AudioManager* audioManager_ = nullptr;
};