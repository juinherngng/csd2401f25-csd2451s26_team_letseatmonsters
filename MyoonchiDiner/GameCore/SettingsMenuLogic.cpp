/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         SettingsMenuLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Implements the dedicated settings screen behaviour.
					The file derives slider/button interaction from authored
					scene widgets, applies fullscreen and audio changes live,
					keeps knob visuals synchronized with config-backed values,
					and plays the appropriate UI hover/click feedback.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <string>

#include "EngineCore/ApplicationState.hpp"
#include "EngineCore/FilePaths.hpp"
#include "EngineCore/InputManager.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/SettingsMenuLogic.hpp"
#include "MyoonchiDiner/GamePaths.hpp"

namespace {
	/**
	 * @brief Axis-aligned rectangle used for authored UI hit-testing.
	 */
	struct Rect {
		float minX = 0.0f;
		float maxX = 0.0f;
		float minY = 0.0f;
		float maxY = 0.0f;
		bool valid = false;
	};

	/**
	 * @brief Visual bounds extracted from a scene object.
	 */
	struct ObjectBounds {
		float minX = 0.0f;
		float maxX = 0.0f;
		float minY = 0.0f;
		float maxY = 0.0f;
		bool valid = false;
	};

	/**
	 * @brief Derived slider layout data built from authored bar/knob objects.
	 */
	struct SliderGeometry {
		float centerMinX = 0.0f;
		float centerMaxX = 0.0f;
		float centerY = 0.0f;
		float hitMinX = 0.0f;
		float hitMaxX = 0.0f;
		float hitHalfHeight = 0.0f;
		bool valid = false;
	};

	/**
	 * @brief Tests whether a world-space point lies within a valid rectangle.
	 * @param point World-space point to test.
	 * @param rect Rectangle to test against.
	 * @return True when the point lies inside the rectangle.
	 */
	static bool IsPointInRect(const glm::vec2& point, const Rect& rect) {
		// Invalid rectangles are treated as non-interactable.
		if (!rect.valid) {
			return false;
		}
		return point.x >= rect.minX && point.x <= rect.maxX &&
			point.y >= rect.minY && point.y <= rect.maxY;
	}

	/**
	 * @brief Computes the width of an extracted bounds record.
	 * @param bounds Bounds to measure.
	 * @return Width of the bounds.
	 */
	static float BoundsWidth(const ObjectBounds& bounds) {
		return bounds.maxX - bounds.minX;
	}

	/**
	 * @brief Computes the height of an extracted bounds record.
	 * @param bounds Bounds to measure.
	 * @return Height of the bounds.
	 */
	static float BoundsHeight(const ObjectBounds& bounds) {
		return bounds.maxY - bounds.minY;
	}

	/**
	 * @brief Computes the horizontal center of an extracted bounds record.
	 * @param bounds Bounds to measure.
	 * @return Center X of the bounds.
	 */
	static float BoundsCenterX(const ObjectBounds& bounds) {
		return (bounds.minX + bounds.maxX) * 0.5f;
	}

	/**
	 * @brief Computes the vertical center of an extracted bounds record.
	 * @param bounds Bounds to measure.
	 * @return Center Y of the bounds.
	 */
	static float BoundsCenterY(const ObjectBounds& bounds) {
		return (bounds.minY + bounds.maxY) * 0.5f;
	}

	/**
	 * @brief Derives a hover texture path from an authored normal-state texture path.
	 * @param path Normal-state texture path, typically ending in `_s`.
	 * @return Matching hover-state path, typically ending in `_h`.
	 */
	static std::string MakeHoverPath(const std::string& path) {
		// Preserve empty paths so callers can treat missing textures safely.
		if (path.empty()) {
			return path;
		}

		const size_t dot = path.find_last_of('.');
		const std::string ext = (dot != std::string::npos) ? path.substr(dot) : std::string();
		const std::string base = (dot != std::string::npos) ? path.substr(0, dot) : path;

		if (base.size() >= 2 && base.substr(base.size() - 2) == "_h") {
			return path;
		}
		if (base.size() >= 2 && base.substr(base.size() - 2) == "_s") {
			return base.substr(0, base.size() - 2) + "_h" + ext;
		}
		return base + "_h" + ext;
	}

	/**
	 * @brief Gets the current mouse position in scene world coordinates.
	 * @param input Shared input manager.
	 * @return Mouse position in the scene's world space.
	 */
	static glm::vec2 GetMouseWorld(InputManager& input) {
		glm::vec2 mouseWorld{};
		// Prefer the scene viewport mapping when available.
		if (!GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld)) {
			// Fall back to the generic screen-to-world transform when needed.
			const glm::vec3 world = input.ScreenToWorld(
				static_cast<float>(input.GetMousePosition().x),
				static_cast<float>(input.GetMousePosition().y));
			mouseWorld = glm::vec2(world.x, world.y);
		}
		return mouseWorld;
	}

	/**
	 * @brief Swaps an object's texture and refreshes its loaded sprite resource.
	 * @param scene Scene containing the target object.
	 * @param objectId Object whose texture should change.
	 * @param texturePath New texture path to apply.
	 */
	static void TrySetTexture(Scene& scene, int objectId, const char* texturePath) {
		// Reject invalid texture requests early.
		if (objectId < 0 || texturePath == nullptr || texturePath[0] == '\0') {
			return;
		}

		GameObject* owner = scene.GetGameObjectByID(objectId);
		if (!owner) {
			return;
		}

		scene.SetObjectTexturePath(objectId, texturePath);
		std::string cacheName = "staticsprite_" + std::string(texturePath);
		if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texturePath)) {
			owner->SetTexture(tex);
		}
	}

	/**
	 * @brief Finds the first object in the scene whose tag matches the supplied string.
	 * @param scene Scene to search.
	 * @param tag Tag string to match.
	 * @return Matching object ID, or `-1` when no object matches.
	 */
	static int FindObjectByTag(Scene& scene, const char* tag) {
		// Empty tags cannot resolve to authored settings widgets.
		if (!tag || tag[0] == '\0') {
			return -1;
		}

		// The settings scene only expects one widget per tag.
		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) {
				continue;
			}
			if (scene.GetObjectTag(obj->GetID()) == tag) {
				return obj->GetID();
			}
		}

		return -1;
	}

	/**
	 * @brief Builds a world-space bounds record from a scene object's center/scale.
	 * @param scene Scene containing the object.
	 * @param objectId Object to inspect.
	 * @return Extracted bounds, or an invalid record if the object is missing.
	 */
	static ObjectBounds GetObjectBounds(Scene& scene, int objectId) {
		GameObject* obj = scene.GetGameObjectByID(objectId);
		if (!obj) {
			return {};
		}

		// Settings UI objects use center-position plus size-style scale values.
		const glm::vec3 pos = obj->GetPositionGLM();
		const glm::vec3 scale = obj->GetScaleGLM();
		const float halfW = scale.x * 0.5f;
		const float halfH = scale.y * 0.5f;

		return {
			pos.x - halfW,
			pos.x + halfW,
			pos.y - halfH,
			pos.y + halfH,
			true
		};
	}

	/**
	 * @brief Converts an object's bounds into a rectangle for hit-testing.
	 * @param scene Scene containing the object.
	 * @param objectId Object to inspect.
	 * @return Rectangle matching the object's visual extents.
	 */
	static Rect GetObjectRect(Scene& scene, int objectId) {
		const ObjectBounds bounds = GetObjectBounds(scene, objectId);
		if (!bounds.valid) {
			return {};
		}

		return { bounds.minX, bounds.maxX, bounds.minY, bounds.maxY, true };
	}

	/**
	 * @brief Derives slider movement and hit-test geometry from authored bar/knob objects.
	 * @param scene Scene containing the slider widgets.
	 * @param barObjectId Bar object for the slider.
	 * @param knobObjectId Knob object for the slider.
	 * @return Derived slider geometry, or an invalid record if the bar is missing.
	 */
	static SliderGeometry GetSliderGeometry(Scene& scene, int barObjectId, int knobObjectId) {
		const ObjectBounds barBounds = GetObjectBounds(scene, barObjectId);
		if (!barBounds.valid) {
			return {};
		}

		const ObjectBounds knobBounds = GetObjectBounds(scene, knobObjectId);
		// Clamp knob center travel so the knob stays visually inside the authored bar.
		const float knobHalfWidth = knobBounds.valid ? (BoundsWidth(knobBounds) * 0.5f) : 0.0f;
		float centerMinX = barBounds.minX + knobHalfWidth;
		float centerMaxX = barBounds.maxX - knobHalfWidth;
		if (centerMaxX < centerMinX) {
			const float centerX = BoundsCenterX(barBounds);
			centerMinX = centerX;
			centerMaxX = centerX;
		}

		const float tallestElement = std::max(BoundsHeight(barBounds), knobBounds.valid ? BoundsHeight(knobBounds) : 0.0f);
		const float hitHalfHeight = tallestElement > 0.0f ? (tallestElement * 0.75f) : 0.0f;
		const float hitMinX = barBounds.minX;
		const float hitMaxX = barBounds.maxX;
		return {
			centerMinX,
			centerMaxX,
			BoundsCenterY(barBounds),
			hitMinX,
			hitMaxX,
			hitHalfHeight,
			true
		};
	}

	/**
	 * @brief Converts derived slider geometry into a track interaction rectangle.
	 * @param geometry Slider geometry to convert.
	 * @return Rectangle covering the authored slider strip.
	 */
	static Rect SliderGeometryToRect(const SliderGeometry& geometry) {
		if (!geometry.valid) {
			return {};
		}

		return {
			geometry.hitMinX,
			geometry.hitMaxX,
			geometry.centerY - geometry.hitHalfHeight,
			geometry.centerY + geometry.hitHalfHeight,
			true
		};
	}

	/**
	 * @brief Returns the union of two rectangles, ignoring invalid inputs.
	 * @param a First rectangle.
	 * @param b Second rectangle.
	 * @return Rectangle covering both inputs.
	 */
	static Rect UnionRects(const Rect& a, const Rect& b) {
		if (!a.valid) {
			return b;
		}

		if (!b.valid) {
			return a;
		}

		return {
			std::min(a.minX, b.minX),
			std::max(a.maxX, b.maxX),
			std::min(a.minY, b.minY),
			std::max(a.maxY, b.maxY),
			true
		};
	}

	/**
	 * @brief Maps a normalized volume value to a knob center X position.
	 * @param volume Normalized volume in the range `[0, 1]`.
	 * @param geometry Slider geometry describing the valid travel range.
	 * @return Knob center X position in world space.
	 */
	static float VolumeToKnobCenterX(float volume, const SliderGeometry& geometry) {
		if (!geometry.valid) {
			return 0.0f;
		}

		// Clamp to keep authored slider motion stable even if config values drift.
		const float clamped = std::clamp(volume, 0.0f, 1.0f);
		return geometry.centerMinX + ((geometry.centerMaxX - geometry.centerMinX) * clamped);
	}

	/**
	 * @brief Maps a knob center X position back to a normalized volume value.
	 * @param knobCenterX Knob center X in world space.
	 * @param geometry Slider geometry describing the valid travel range.
	 * @return Normalized volume value in the range `[0, 1]`.
	 */
	static float KnobCenterXToVolume(float knobCenterX, const SliderGeometry& geometry) {
		if (!geometry.valid || geometry.centerMaxX <= geometry.centerMinX) {
			return 0.0f;
		}

		const float clamped = std::clamp(knobCenterX, geometry.centerMinX, geometry.centerMaxX);
		return (clamped - geometry.centerMinX) / (geometry.centerMaxX - geometry.centerMinX);
	}

	/**
	 * @brief Moves an authored UI object so its center matches the supplied world position.
	 * @param scene Scene containing the object.
	 * @param objectId Object to move.
	 * @param desiredCenter Desired world-space center point.
	 */
	static void SetVisualObjectCenter(Scene& scene, int objectId, const glm::vec2& desiredCenter) {
		GameObject* obj = scene.GetGameObjectByID(objectId);
		if (!obj) {
			return;
		}

		// Preserve the authored Z value while updating only the 2D center.
		glm::vec3 pos = obj->GetPositionGLM();
		pos.x = desiredCenter.x;
		pos.y = desiredCenter.y;
		obj->SetPosition(pos);
	}

	/**
	 * @brief Resolves the config file path used by the settings screen.
	 * @return Absolute or project-relative config path used for load/save.
	 */
	static std::string ResolveConfigPath() {
		std::string path;
		// Prefer the engine's resolved asset path when available.
		if (ConfigManager::ResolveAssetPath(path)) {
			return path;
		}

		return FilePaths::JoinPath(FilePaths::Dirs::ASSETS, "config.txt");
	}
}

/**
 * @brief Initializes config-backed state and resolves authored widget references.
 * @param scene Active scene containing the settings screen.
 */
void SettingsMenuLogic::Start(Scene& scene) {
	// Initialization is one-shot for the lifetime of this logic component.
	if (initialized_) {
		return;
	}

	// Reuse the scene's audio manager if one was not injected earlier.
	if (!audioManager_) {
		audioManager_ = scene.GetAudioManager();
	}

	// Load persisted settings first so runtime visuals can snap to real values.
	settings_ = ConfigManager::LoadFromAssetsOrDefaults();
	ConfigManager::Validate(settings_);
	if (g_AppState) {
		// Fullscreen must reflect the live app window, not only the stored file.
		settings_.fullscreen = IsApplicationFullscreen();
	}

	// Cache scene-authored widgets and their normal/hover texture paths.
	configPath_ = ResolveConfigPath();
	ResolveWidgetIds(scene);
	fullscreenNormalTexturePath_ = fullscreenVisualId_ >= 0 ? scene.GetObjectTexturePath(fullscreenVisualId_) : std::string();
	windowedNormalTexturePath_ = windowedVisualId_ >= 0 ? scene.GetObjectTexturePath(windowedVisualId_) : std::string();
	fullscreenHoverTexturePath_ = MakeHoverPath(fullscreenNormalTexturePath_);
	windowedHoverTexturePath_ = MakeHoverPath(windowedNormalTexturePath_);

	// In the editor we want authored knob positions from the level JSON to stay editable.
	// Only snap visuals to live config.txt values when the scene is actually running in menu runtime mode.
	if (scene.ShouldUseRuntimeParityMode()) {
		RefreshVisualState(scene);
	}

	initialized_ = true;
}

/**
 * @brief Processes settings hover, click, and slider drag interaction.
 * @param dt Frame delta time in seconds.
 * @param scene Active scene containing the settings UI.
 * @param input Shared input manager used for mouse interaction.
 */
void SettingsMenuLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
	// Lazily initialize when the logic begins updating.
	if (!initialized_) {
		Start(scene);
	}

	// Editor authoring mode should not drive the runtime menu interaction layer.
	if (!scene.ShouldUseRuntimeParityMode()) {
		return;
	}

	const bool actualFullscreen = IsApplicationFullscreen();
	if (g_AppState && settings_.fullscreen != actualFullscreen) {
		// Keep the stored state in sync with external fullscreen changes.
		settings_.fullscreen = actualFullscreen;
		ConfigManager::Validate(settings_);
		if (!configPath_.empty()) {
			ConfigManager::Save(configPath_, settings_);
		}
		RefreshVisualState(scene);
	}

	const glm::vec2 mouseWorld = GetMouseWorld(input);
	const Rect fullscreenButtonRect = GetObjectRect(scene, fullscreenVisualId_);
	const Rect windowedButtonRect = GetObjectRect(scene, windowedVisualId_);
	const SliderGeometry masterSlider = GetSliderGeometry(scene, masterBarVisualId_, masterKnobVisualId_);
	const SliderGeometry bgmSlider = GetSliderGeometry(scene, bgmBarVisualId_, bgmKnobVisualId_);
	const SliderGeometry sfxSlider = GetSliderGeometry(scene, sfxBarVisualId_, sfxKnobVisualId_);
	const Rect masterSliderInteractRect = UnionRects(SliderGeometryToRect(masterSlider), GetObjectRect(scene, masterKnobVisualId_));
	const Rect bgmSliderInteractRect = UnionRects(SliderGeometryToRect(bgmSlider), GetObjectRect(scene, bgmKnobVisualId_));
	const Rect sfxSliderInteractRect = UnionRects(SliderGeometryToRect(sfxSlider), GetObjectRect(scene, sfxKnobVisualId_));
	const bool overFullscreen = IsPointInRect(mouseWorld, fullscreenButtonRect);
	const bool overWindowed = IsPointInRect(mouseWorld, windowedButtonRect);

	if (overFullscreen != fullscreenHovered_) {
		// Refresh hover art and sound only when the state actually changes.
		fullscreenHovered_ = overFullscreen;
		if (fullscreenHovered_) {
			PlayHoverSound();
		}
		RefreshVisualState(scene);
	}

	if (overWindowed != windowedHovered_) {
		// Windowed uses the same hover feedback flow as fullscreen.
		windowedHovered_ = overWindowed;
		if (windowedHovered_) {
			PlayHoverSound();
		}

		RefreshVisualState(scene);
	}

	if (input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		if (overFullscreen) {
			// Fullscreen toggles save immediately because the app window changes right away.
			PlayClickSound();
			settings_.fullscreen = true;
			PersistSettings(true);
			RefreshVisualState(scene);
			input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
			return;
		}

		if (overWindowed) {
			// Windowed mode follows the same immediate-apply path.
			PlayClickSound();
			settings_.fullscreen = false;
			PersistSettings(true);
			RefreshVisualState(scene);
			input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
			return;
		}

		if (IsPointInRect(mouseWorld, masterSliderInteractRect)) {
			// Dragging starts from either the bar strip or the current knob body.
			PlayClickSound();
			StartSliderDrag(SliderTarget::Master, mouseWorld.x);
			UpdateSliderDrag(mouseWorld.x, scene);
			return;
		}

		if (IsPointInRect(mouseWorld, bgmSliderInteractRect)) {
			// BGM slider uses the same authored geometry-derived interaction region.
			PlayClickSound();
			StartSliderDrag(SliderTarget::Bgm, mouseWorld.x);
			UpdateSliderDrag(mouseWorld.x, scene);
			return;
		}

		if (IsPointInRect(mouseWorld, sfxSliderInteractRect)) {
			// SFX slider also supports click-to-jump plus immediate drag continuation.
			PlayClickSound();
			StartSliderDrag(SliderTarget::Sfx, mouseWorld.x);
			UpdateSliderDrag(mouseWorld.x, scene);
			return;
		}
	}

	if (draggingSlider_ != SliderTarget::None && input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		// Continue updating the active slider while the mouse button is held.
		UpdateSliderDrag(mouseWorld.x, scene);
	}

	if (draggingSlider_ != SliderTarget::None && input.IsMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
		// Releasing the mouse ends slider dragging cleanly.
		draggingSlider_ = SliderTarget::None;
	}
}

/**
 * @brief Resolves all authored settings widget IDs from their scene tags.
 * @param scene Scene containing the settings objects.
 */
void SettingsMenuLogic::ResolveWidgetIds(Scene& scene) {
	// Each tag maps to a single authored widget in settings.json.
	fullscreenVisualId_ = FindObjectByTag(scene, "settings_fullscreen_visual");
	windowedVisualId_ = FindObjectByTag(scene, "settings_windowed_visual");
	masterBarVisualId_ = FindObjectByTag(scene, "settings_master_bar_visual");
	bgmBarVisualId_ = FindObjectByTag(scene, "settings_bgm_bar_visual");
	sfxBarVisualId_ = FindObjectByTag(scene, "settings_sfx_bar_visual");
	masterKnobVisualId_ = FindObjectByTag(scene, "settings_master_knob_visual");
	bgmKnobVisualId_ = FindObjectByTag(scene, "settings_bgm_knob_visual");
	sfxKnobVisualId_ = FindObjectByTag(scene, "settings_sfx_knob_visual");
}

/**
 * @brief Applies the current settings state back onto button textures and knob positions.
 * @param scene Scene containing the live settings UI.
 */
void SettingsMenuLogic::RefreshVisualState(Scene& scene) {
	// Update button art first so hover changes appear immediately.
	TrySetTexture(scene,
		fullscreenVisualId_,
		fullscreenHovered_ ? fullscreenHoverTexturePath_.c_str() : fullscreenNormalTexturePath_.c_str());

	TrySetTexture(scene,
		windowedVisualId_,
		windowedHovered_ ? windowedHoverTexturePath_.c_str() : windowedNormalTexturePath_.c_str());

	const SliderGeometry masterSlider = GetSliderGeometry(scene, masterBarVisualId_, masterKnobVisualId_);
	const SliderGeometry bgmSlider = GetSliderGeometry(scene, bgmBarVisualId_, bgmKnobVisualId_);
	const SliderGeometry sfxSlider = GetSliderGeometry(scene, sfxBarVisualId_, sfxKnobVisualId_);

	if (masterSlider.valid) {
		// Snap the knob to the config-backed master volume value.
		SetVisualObjectCenter(
			scene,
			masterKnobVisualId_,
			{ VolumeToKnobCenterX(settings_.masterVolume, masterSlider), masterSlider.centerY });
	}

	if (bgmSlider.valid) {
		// Snap the knob to the config-backed BGM volume value.
		SetVisualObjectCenter(
			scene,
			bgmKnobVisualId_,
			{ VolumeToKnobCenterX(settings_.bgmVolume, bgmSlider), bgmSlider.centerY });
	}

	if (sfxSlider.valid) {
		// Snap the knob to the config-backed SFX volume value.
		SetVisualObjectCenter(
			scene,
			sfxKnobVisualId_,
			{ VolumeToKnobCenterX(settings_.vfxVolume, sfxSlider), sfxSlider.centerY });
	}
}

/**
 * @brief Validates, applies, and saves the current settings state.
 * @param applyFullscreenImmediately True to change the app window mode right away.
 */
void SettingsMenuLogic::PersistSettings(bool applyFullscreenImmediately) {
	// Validate first so every downstream consumer sees clamped values.
	ConfigManager::Validate(settings_);

	if (applyFullscreenImmediately) {
		// Fullscreen mode can diverge slightly after the OS/windowing layer responds.
		SetApplicationFullscreen(settings_.fullscreen);
		settings_.fullscreen = IsApplicationFullscreen();
	}

	if (audioManager_) {
		// Live-apply audio sliders so the user hears the change immediately.
		audioManager_->ApplySettings(settings_);
	}

	if (!configPath_.empty()) {
		// Persist to config.txt so later sessions reuse the updated values.
		ConfigManager::Save(configPath_, settings_);
	}
}

/**
 * @brief Marks a specific settings slider as the active drag target.
 * @param target Slider being dragged.
 * @param mouseX Current mouse world X position at drag start.
 */
void SettingsMenuLogic::StartSliderDrag(SliderTarget target, float mouseX) {
	// Store only the active slider; current code does not need drag offsets.
	draggingSlider_ = target;
	(void)mouseX;
}

/**
 * @brief Recomputes the dragged slider's value from the current mouse position.
 * @param mouseX Current mouse world X position.
 * @param scene Scene containing the settings widgets.
 */
void SettingsMenuLogic::UpdateSliderDrag(float mouseX, Scene& scene) {
	SliderGeometry sliderGeometry{};

	// Pick the authored geometry for whichever slider is currently active.
	switch (draggingSlider_) {
	case SliderTarget::Master:
		sliderGeometry = GetSliderGeometry(scene, masterBarVisualId_, masterKnobVisualId_);
		break;
	case SliderTarget::Bgm:
		sliderGeometry = GetSliderGeometry(scene, bgmBarVisualId_, bgmKnobVisualId_);
		break;
	case SliderTarget::Sfx:
		sliderGeometry = GetSliderGeometry(scene, sfxBarVisualId_, sfxKnobVisualId_);
		break;
	case SliderTarget::None:
	default:
		return;
	}

	if (!sliderGeometry.valid) {
		return;
	}

	// Convert world-space mouse X into a normalized volume value.
	const float volume = KnobCenterXToVolume(mouseX, sliderGeometry);
	bool changed = false;

	switch (draggingSlider_) {
	case SliderTarget::Master:
		changed = settings_.masterVolume != volume;
		settings_.masterVolume = volume;
		break;
	case SliderTarget::Bgm:
		changed = settings_.bgmVolume != volume;
		settings_.bgmVolume = volume;
		break;
	case SliderTarget::Sfx:
		changed = settings_.vfxVolume != volume;
		settings_.vfxVolume = volume;
		break;
	case SliderTarget::None:
	default:
		return;
	}

	if (!changed) {
		return;
	}

	// Persist the changed value and re-render the knob in its snapped position.
	PersistSettings(false);
	RefreshVisualState(scene);
}

/**
 * @brief Plays the standard hover SFX for the settings menu.
 */
void SettingsMenuLogic::PlayHoverSound() {
	if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
		// Route hover feedback through the shared UI SFX volume channel.
		audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager_->GetVfxVolume(), false);
	}
}

/**
 * @brief Plays the standard click SFX for the settings menu.
 */
void SettingsMenuLogic::PlayClickSound() {
	if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
		// Use the same UI click sound as the rest of the menu flow.
		audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager_->GetVfxVolume(), false);
	}
}
