/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         SettingsMenuLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Declares the SettingsMenuLogic component used by the
					dedicated settings screen. The logic resolves authored UI
					widgets from the level JSON, synchronizes them with
					config-backed fullscreen and audio settings, and handles
					button/slider interaction while the scene is running as
					runtime menu UI.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <glm/vec3.hpp>
#include <string>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/ConfigManager.hpp"
#include "EngineCore/GameObjectLogic.hpp"

 /**
  * @brief Drives the dedicated settings-menu screen.
  * @details
  * Owns the authored settings UI widgets, keeps them synchronized with
  * config-backed fullscreen/audio values, and handles button/slider input
  * while the scene is running as menu UI.
  */
class SettingsMenuLogic final : public GameObjectLogic {
public:
	/**
	 * @brief Constructs the settings logic for the owning game object.
	 * @param ownerID Scene object ID that owns this logic component.
	 */
	explicit SettingsMenuLogic(int ownerID)
		: GameObjectLogic(ownerID) {
		// Defer widget lookup until Start() so scene-authored IDs are available.
	}

	/**
	 * @brief Initializes the settings screen from config and scene-authored widgets.
	 * @param scene Active scene that owns the settings UI.
	 */
	void Start(Scene& scene) override;

	/**
	 * @brief Updates settings input, hover state, and slider interaction each frame.
	 * @param dt Frame delta time in seconds.
	 * @param scene Active scene containing the settings widgets.
	 * @param input Shared input manager used for mouse interaction.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Injects the audio manager used for UI feedback and live audio updates.
	 * @param mgr Audio manager backing the current scene.
	 */
	void SetAudioManager(AudioManager* mgr) {
		// Cache the scene audio manager so slider/button interaction can update audio immediately.
		audioManager_ = mgr;
	}

private:
	/**
	 * @brief Identifies which slider, if any, is currently being dragged.
	 */
	enum class SliderTarget {
		// `None` means no slider is highlighted, dragged, or keyboard-adjusted.
		None,
		Master,
		Bgm,
		Sfx
	};

	/**
	 * @brief Resolves authored settings widget IDs from scene tags.
	 * @param scene Scene containing the settings objects.
	 */
	void ResolveWidgetIds(Scene& scene);

	/**
	 * @brief Caches the authored slider scales so focus/active scaling can restore cleanly.
	 * @param scene Scene containing the settings widgets.
	 */
	void CacheSliderBaseScales(Scene& scene);

	/**
	 * @brief Refreshes button textures and knob positions from the current settings state.
	 * @param scene Scene containing the live settings widgets.
	 */
	void RefreshVisualState(Scene& scene);

	/**
	 * @brief Saves the current settings and optionally applies fullscreen immediately.
	 * @param applyFullscreenImmediately True to push the fullscreen mode to the app instantly.
	 */
	void PersistSettings(bool applyFullscreenImmediately);

	/**
	 * @brief Starts dragging one of the settings sliders.
	 * @param target Slider being grabbed.
	 * @param mouseX Current mouse world X position at drag start.
	 */
	void StartSliderDrag(SliderTarget target, float mouseX);

	/**
	 * @brief Updates the active slider based on the current mouse X position.
	 * @param mouseX Current mouse world X position.
	 * @param scene Scene containing the slider widgets.
	 */
	void UpdateSliderDrag(float mouseX, Scene& scene);

	/**
	 * @brief Maps a focused settings object back to its corresponding slider target.
	 * @param objectId Focused object ID from menu navigation.
	 * @return Matching slider target, or `None` when the focus is on a button.
	 */
	SliderTarget SliderTargetFromFocusId(int objectId) const;

	/**
	 * @brief Returns the authored bar object ID for a given slider target.
	 * @param target Slider to resolve.
	 * @return Bar object ID, or `-1` when unavailable.
	 */
	int GetSliderBarVisualId(SliderTarget target) const;

	/**
	 * @brief Returns the authored knob object ID for a given slider target.
	 * @param target Slider to resolve.
	 * @return Knob object ID, or `-1` when unavailable.
	 */
	int GetSliderKnobVisualId(SliderTarget target) const;

	/**
	 * @brief Returns the config-backed value for the requested slider.
	 * @param target Slider to read.
	 * @return Current normalized value in the range `[0, 1]`.
	 */
	float GetSliderValue(SliderTarget target) const;

	/**
	 * @brief Stores a normalized value back into the requested slider field.
	 * @param target Slider to update.
	 * @param value Normalized value in the range `[0, 1]`.
	 */
	void SetSliderValue(SliderTarget target, float value);

	/**
	 * @brief Applies a single keyboard step to the active slider and persists the change.
	 * @param scene Scene containing the settings widgets.
	 * @param target Slider to modify.
	 * @param direction Negative for left, positive for right.
	 */
	void ApplyKeyboardSliderStep(Scene& scene, SliderTarget target, int direction);

	/**
	 * @brief Drives held-key repeat while a slider is in keyboard-adjust mode.
	 * @param dt Frame delta time in seconds.
	 * @param scene Scene containing the settings widgets.
	 * @param input Shared input manager.
	 */
	void UpdateKeyboardSliderAdjustment(float dt, Scene& scene, InputManager& input);

	/**
	 * @brief Plays the standard UI hover sound if available.
	 */
	void PlayHoverSound();

	/**
	 * @brief Plays the standard UI click sound if available.
	 */
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
	bool suppressHoverFeedbackUntilMouseMove_ = false;
	float lastMouseWorldX_ = 0.0f;
	float lastMouseWorldY_ = 0.0f;
	float sliderAdjustRepeatTimer_ = 0.0f;
	int sliderAdjustHeldDirection_ = 0;

	SliderTarget draggingSlider_ = SliderTarget::None;
	SliderTarget keyboardAdjustingSlider_ = SliderTarget::None;
	SliderTarget highlightedSlider_ = SliderTarget::None;
	AudioManager* audioManager_ = nullptr;

	std::string fullscreenNormalTexturePath_;
	std::string fullscreenHoverTexturePath_;
	std::string windowedNormalTexturePath_;
	std::string windowedHoverTexturePath_;

	glm::vec3 masterBarBaseScale_{ 0.0f, 0.0f, 1.0f };
	glm::vec3 bgmBarBaseScale_{ 0.0f, 0.0f, 1.0f };
	glm::vec3 sfxBarBaseScale_{ 0.0f, 0.0f, 1.0f };
	glm::vec3 masterKnobBaseScale_{ 0.0f, 0.0f, 1.0f };
	glm::vec3 bgmKnobBaseScale_{ 0.0f, 0.0f, 1.0f };
	glm::vec3 sfxKnobBaseScale_{ 0.0f, 0.0f, 1.0f };
};
