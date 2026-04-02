/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         SettingsMenuLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Declares the SettingsMenuLogic component that powers the
					dedicated settings screen, including fullscreen toggles,
					and volume sliders.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/ConfigManager.hpp"
#include "EngineCore/GameObjectLogic.hpp"

class SettingsMenuLogic final : public GameObjectLogic {
public:
	explicit SettingsMenuLogic(int ownerID)
		: GameObjectLogic(ownerID) {}

	void Start(Scene& scene) override;
	void Update(float dt, Scene& scene, InputManager& input) override;

	void SetAudioManager(AudioManager* mgr) {
		audioManager_ = mgr;
	}

private:
	enum class SliderTarget {
		None,
		Master,
		Bgm,
		Sfx
	};

	void ResolveWidgetIds(Scene& scene);
	void RefreshVisualState(Scene& scene);
	void PersistSettings(bool applyFullscreenImmediately);
	void StartSliderDrag(SliderTarget target, float mouseX);
	void UpdateSliderDrag(float mouseX, Scene& scene);
	void PlayHoverSound();
	void PlayClickSound();

	bool initialized_ = false;
	ConfigManager::Settings settings_{};
	std::string configPath_;

	int fullscreenVisualId_ = -1;
	int windowedVisualId_ = -1;
	int masterBarVisualId_ = -1;
	int bgmBarVisualId_ = -1;
	int sfxBarVisualId_ = -1;
	int masterKnobVisualId_ = -1;
	int bgmKnobVisualId_ = -1;
	int sfxKnobVisualId_ = -1;

	bool fullscreenHovered_ = false;
	bool windowedHovered_ = false;

	SliderTarget draggingSlider_ = SliderTarget::None;
	AudioManager* audioManager_ = nullptr;

	std::string fullscreenNormalTexturePath_;
	std::string fullscreenHoverTexturePath_;
	std::string windowedNormalTexturePath_;
	std::string windowedHoverTexturePath_;
};
