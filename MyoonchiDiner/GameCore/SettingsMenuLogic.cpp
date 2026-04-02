/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         SettingsMenuLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Implements the dedicated settings screen behaviour,
					including live config updates for fullscreen and audio,
					authored slider hit-testing.

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
	struct Rect {
		float minX = 0.0f;
		float maxX = 0.0f;
		float minY = 0.0f;
		float maxY = 0.0f;
		bool valid = false;
	};

	struct ObjectBounds {
		float minX = 0.0f;
		float maxX = 0.0f;
		float minY = 0.0f;
		float maxY = 0.0f;
		bool valid = false;
	};

	struct SliderGeometry {
		float centerMinX = 0.0f;
		float centerMaxX = 0.0f;
		float centerY = 0.0f;
		float hitMinX = 0.0f;
		float hitMaxX = 0.0f;
		float hitHalfHeight = 0.0f;
		bool valid = false;
	};

	static bool IsPointInRect(const glm::vec2& point, const Rect& rect) {
		if (!rect.valid) {
			return false;
		}
		return point.x >= rect.minX && point.x <= rect.maxX &&
			point.y >= rect.minY && point.y <= rect.maxY;
	}

	static float BoundsWidth(const ObjectBounds& bounds) {
		return bounds.maxX - bounds.minX;
	}

	static float BoundsHeight(const ObjectBounds& bounds) {
		return bounds.maxY - bounds.minY;
	}

	static float BoundsCenterX(const ObjectBounds& bounds) {
		return (bounds.minX + bounds.maxX) * 0.5f;
	}

	static float BoundsCenterY(const ObjectBounds& bounds) {
		return (bounds.minY + bounds.maxY) * 0.5f;
	}

	static std::string MakeHoverPath(const std::string& path) {
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

	static glm::vec2 GetMouseWorld(InputManager& input) {
		glm::vec2 mouseWorld{};
		if (!GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld)) {
			const glm::vec3 world = input.ScreenToWorld(
				static_cast<float>(input.GetMousePosition().x),
				static_cast<float>(input.GetMousePosition().y));
			mouseWorld = glm::vec2(world.x, world.y);
		}
		return mouseWorld;
	}

	static void TrySetTexture(Scene& scene, int objectId, const char* texturePath) {
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

	static int FindObjectByTag(Scene& scene, const char* tag) {
		if (!tag || tag[0] == '\0') {
			return -1;
		}

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

	static ObjectBounds GetObjectBounds(Scene& scene, int objectId) {
		GameObject* obj = scene.GetGameObjectByID(objectId);
		if (!obj) {
			return {};
		}

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

	static Rect GetObjectRect(Scene& scene, int objectId) {
		const ObjectBounds bounds = GetObjectBounds(scene, objectId);
		if (!bounds.valid) {
			return {};
		}

		return { bounds.minX, bounds.maxX, bounds.minY, bounds.maxY, true };
	}

	static SliderGeometry GetSliderGeometry(Scene& scene, int barObjectId, int knobObjectId) {
		const ObjectBounds barBounds = GetObjectBounds(scene, barObjectId);
		if (!barBounds.valid) {
			return {};
		}

		const ObjectBounds knobBounds = GetObjectBounds(scene, knobObjectId);
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

	static float VolumeToKnobCenterX(float volume, const SliderGeometry& geometry) {
		if (!geometry.valid) {
			return 0.0f;
		}
		const float clamped = std::clamp(volume, 0.0f, 1.0f);
		return geometry.centerMinX + ((geometry.centerMaxX - geometry.centerMinX) * clamped);
	}

	static float KnobCenterXToVolume(float knobCenterX, const SliderGeometry& geometry) {
		if (!geometry.valid || geometry.centerMaxX <= geometry.centerMinX) {
			return 0.0f;
		}

		const float clamped = std::clamp(knobCenterX, geometry.centerMinX, geometry.centerMaxX);
		return (clamped - geometry.centerMinX) / (geometry.centerMaxX - geometry.centerMinX);
	}

	static void SetVisualObjectCenter(Scene& scene, int objectId, const glm::vec2& desiredCenter) {
		GameObject* obj = scene.GetGameObjectByID(objectId);
		if (!obj) {
			return;
		}

		glm::vec3 pos = obj->GetPositionGLM();
		pos.x = desiredCenter.x;
		pos.y = desiredCenter.y;
		obj->SetPosition(pos);
	}

	static std::string ResolveConfigPath() {
		std::string path;
		if (ConfigManager::ResolveAssetPath(path)) {
			return path;
		}
		return FilePaths::JoinPath(FilePaths::Dirs::ASSETS, "config.txt");
	}
}

void SettingsMenuLogic::Start(Scene& scene) {
	if (initialized_) {
		return;
	}

	if (!audioManager_) {
		audioManager_ = scene.GetAudioManager();
	}

	settings_ = ConfigManager::LoadFromAssetsOrDefaults();
	ConfigManager::Validate(settings_);
	if (g_AppState) {
		settings_.fullscreen = IsApplicationFullscreen();
	}

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

void SettingsMenuLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
	if (!initialized_) {
		Start(scene);
	}

	if (!scene.ShouldUseRuntimeParityMode()) {
		return;
	}

	const bool actualFullscreen = IsApplicationFullscreen();
	if (g_AppState && settings_.fullscreen != actualFullscreen) {
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
		fullscreenHovered_ = overFullscreen;
		if (fullscreenHovered_) {
			PlayHoverSound();
		}
		RefreshVisualState(scene);
	}

	if (overWindowed != windowedHovered_) {
		windowedHovered_ = overWindowed;
		if (windowedHovered_) {
			PlayHoverSound();
		}
		RefreshVisualState(scene);
	}

	if (input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		if (overFullscreen) {
			PlayClickSound();
			settings_.fullscreen = true;
			PersistSettings(true);
			RefreshVisualState(scene);
			input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
			return;
		}

		if (overWindowed) {
			PlayClickSound();
			settings_.fullscreen = false;
			PersistSettings(true);
			RefreshVisualState(scene);
			input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
			return;
		}

		if (IsPointInRect(mouseWorld, masterSliderInteractRect)) {
			PlayClickSound();
			StartSliderDrag(SliderTarget::Master, mouseWorld.x);
			UpdateSliderDrag(mouseWorld.x, scene);
			return;
		}

		if (IsPointInRect(mouseWorld, bgmSliderInteractRect)) {
			PlayClickSound();
			StartSliderDrag(SliderTarget::Bgm, mouseWorld.x);
			UpdateSliderDrag(mouseWorld.x, scene);
			return;
		}

		if (IsPointInRect(mouseWorld, sfxSliderInteractRect)) {
			PlayClickSound();
			StartSliderDrag(SliderTarget::Sfx, mouseWorld.x);
			UpdateSliderDrag(mouseWorld.x, scene);
			return;
		}
	}

	if (draggingSlider_ != SliderTarget::None && input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		UpdateSliderDrag(mouseWorld.x, scene);
	}

	if (draggingSlider_ != SliderTarget::None && input.IsMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
		draggingSlider_ = SliderTarget::None;
	}
}

void SettingsMenuLogic::ResolveWidgetIds(Scene& scene) {
	fullscreenVisualId_ = FindObjectByTag(scene, "settings_fullscreen_visual");
	windowedVisualId_ = FindObjectByTag(scene, "settings_windowed_visual");
	masterBarVisualId_ = FindObjectByTag(scene, "settings_master_bar_visual");
	bgmBarVisualId_ = FindObjectByTag(scene, "settings_bgm_bar_visual");
	sfxBarVisualId_ = FindObjectByTag(scene, "settings_sfx_bar_visual");
	masterKnobVisualId_ = FindObjectByTag(scene, "settings_master_knob_visual");
	bgmKnobVisualId_ = FindObjectByTag(scene, "settings_bgm_knob_visual");
	sfxKnobVisualId_ = FindObjectByTag(scene, "settings_sfx_knob_visual");
}

void SettingsMenuLogic::RefreshVisualState(Scene& scene) {
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
		SetVisualObjectCenter(
			scene,
			masterKnobVisualId_,
			{ VolumeToKnobCenterX(settings_.masterVolume, masterSlider), masterSlider.centerY });
	}

	if (bgmSlider.valid) {
		SetVisualObjectCenter(
			scene,
			bgmKnobVisualId_,
			{ VolumeToKnobCenterX(settings_.bgmVolume, bgmSlider), bgmSlider.centerY });
	}

	if (sfxSlider.valid) {
		SetVisualObjectCenter(
			scene,
			sfxKnobVisualId_,
			{ VolumeToKnobCenterX(settings_.vfxVolume, sfxSlider), sfxSlider.centerY });
	}
}

void SettingsMenuLogic::PersistSettings(bool applyFullscreenImmediately) {
	ConfigManager::Validate(settings_);

	if (applyFullscreenImmediately) {
		SetApplicationFullscreen(settings_.fullscreen);
		settings_.fullscreen = IsApplicationFullscreen();
	}

	if (audioManager_) {
		audioManager_->ApplySettings(settings_);
	}

	if (!configPath_.empty()) {
		ConfigManager::Save(configPath_, settings_);
	}
}

void SettingsMenuLogic::StartSliderDrag(SliderTarget target, float mouseX) {
	draggingSlider_ = target;
	(void)mouseX;
}

void SettingsMenuLogic::UpdateSliderDrag(float mouseX, Scene& scene) {
	SliderGeometry sliderGeometry{};

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

	PersistSettings(false);
	RefreshVisualState(scene);
}

void SettingsMenuLogic::PlayHoverSound() {
	if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
		audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager_->GetVfxVolume(), false);
	}
}

void SettingsMenuLogic::PlayClickSound() {
	if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
		audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager_->GetVfxVolume(), false);
	}
}
