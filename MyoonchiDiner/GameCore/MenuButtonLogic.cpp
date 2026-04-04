/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			MenuButtonLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:		Ng Juin Herng, juinherng.ng@digipen.edu (10%)
					Yat Chun Wee, y.chunwee@digipen.edu		(35%)
					Vu Phan Hung, phanhung.vu@digipen.edu	(15%)

 DESCRIPTION:		Implements hover and activation behavior for top-level menu buttons.
					Handles hover-state texture swaps, menu audio feedback, and direct
					level-transition requests when a button is activated.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <string>
#include <vector>

#include "EngineCore/AudioManager.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/MenuButtonLogic.hpp"
#include "GameCore/MenuKeyboardNavigation.hpp"
#include "MyoonchiDiner/GamePaths.hpp"

namespace {
	/**
	 * @brief Builds the hover-texture path that corresponds to a normal button texture.
	 * @param path Current texture path authored for the button.
	 * @return Hover-state texture path derived from the `_s`/`_h` naming convention.
	 */
	static std::string MakeHoverPath(const std::string& path) {
		// Preserve empty inputs so callers can safely skip missing texture metadata.
		if (path.empty()) {
			return path;
		}

		// Split the file into base name and extension so suffix replacement stays simple.
		const size_t dot = path.find_last_of('.');
		const std::string ext = (dot != std::string::npos) ? path.substr(dot) : std::string();
		const std::string base = (dot != std::string::npos) ? path.substr(0, dot) : path;

		// Respect already-hovered textures instead of stacking extra suffixes onto them.
		if (base.size() >= 2 && base.substr(base.size() - 2) == "_h") {
			return dot != std::string::npos ? path : (base + ext);
		}

		// Convert the standard selected suffix into a hover suffix when present.
		if (base.size() >= 2 && base.substr(base.size() - 2) == "_s") {
			return base.substr(0, base.size() - 2) + "_h" + ext;
		}

		// Fall back to appending `_h` for assets that do not use the selected suffix.
		return base + "_h" + ext;
	}

	/**
	 * @brief Applies a texture to the owner object if the asset can be loaded.
	 * @param owner Object whose sprite texture should be updated.
	 * @param texPath Texture path to load and assign.
	 */
	static void TrySetTexture(GameObject* owner, const std::string& texPath) {
		// Ignore invalid targets or empty paths so menu hover transitions stay defensive.
		if (!owner || texPath.empty()) {
			return;
		}

		// Reuse the shared texture cache so repeated hover swaps stay cheap.
		const std::string cacheName = "staticsprite_" + texPath;
		if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
			owner->SetTexture(tex);
		}
	}
}

void MenuButtonLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
	// Discover the normal and hover texture paths lazily from the scene-authored sprite metadata.
	if (!initialized_) {
		normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
		hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
		initialized_ = true;
	}

	// Editor-only scenes should not run the runtime menu interaction path.
	if (!scene.ShouldUseRuntimeParityMode()) {
		if (hovered_) {
			hovered_ = false;
			if (GameObject* owner = GetOwner(scene)) {
				TrySetTexture(owner, normalTexturePath_);
			}
		}
		return;
	}

	// Cutscenes and presentation transitions temporarily own the screen, so suppress menu input.
	if (scene.IsMenuInteractionSuppressed()) {
		if (hovered_) {
			hovered_ = false;
			if (GameObject* owner = GetOwner(scene)) {
				TrySetTexture(owner, normalTexturePath_);
			}
		}
		return;
	}

	// Modal overlays take priority over regular top-level menu buttons.
	if (scene.IsHowToPlayOverlayActive() || scene.IsMenuModalActive()) {
		if (hovered_) {
			hovered_ = false;
			if (GameObject* owner = GetOwner(scene)) {
				TrySetTexture(owner, normalTexturePath_);
			}
		}
		return;
	}

	glm::vec2 mouseWorld{};
	bool insideScene = false;

	// Use the replay cursor when playback overrides the live mouse input stream.
	if (input.IsReplayOverride()) {
		const glm::dvec2 replayMousePos = input.GetMousePosition();
		insideScene = scene.GetGraphicsEngine().GetMouseWorldInScene(mouseWorld, &replayMousePos);
	}
	else {
		insideScene = scene.GetGraphicsEngine().GetMouseWorldInScene(mouseWorld, nullptr);
	}

	// Abort cleanly if the button object was removed this frame.
	GameObject* owner = GetOwner(scene);
	if (!owner) {
		return;
	}

	// Build the button bounds from the authored sprite transform.
	const glm::vec3 pos = owner->GetPositionGLM();
	const glm::vec3 sz = owner->GetScaleGLM();
	const float halfW = sz.x * 0.5f;
	const float halfH = sz.y * 0.5f;

	// Mouse hover is valid only when the pointer is inside the rendered scene viewport.
	const bool mouseOver = insideScene &&
		mouseWorld.x >= (pos.x - halfW) && mouseWorld.x <= (pos.x + halfW) &&
		mouseWorld.y >= (pos.y - halfH) && mouseWorld.y <= (pos.y + halfH);

	// Merge pointer hover with keyboard navigation so both inputs drive the same highlight state.
	const std::vector<int> buttonIds = MenuKeyboardNavigation::CollectCurrentSceneTopLevelButtons(scene);
	const int focusedButtonId = MenuKeyboardNavigation::UpdateFocus(
		scene,
		input,
		MenuKeyboardNavigation::GetCurrentSceneTopLevelScopeKey(scene),
		buttonIds,
		mouseOver ? GetOwnerID() : -1);
	const bool keyboardFocused = (focusedButtonId == GetOwnerID());
	const bool over = mouseOver || keyboardFocused;

	// Promote the button into its hover state and optionally play hover feedback once on entry.
	if (over && !hovered_) {
		hovered_ = true;
		TrySetTexture(owner, hoverTexturePath_);

		const std::string ownerTag = scene.GetObjectTag(GetOwnerID());
		if (ownerTag != "btn_next_level" && ownerTag != "btn_retry_level") {
			scene.TriggerUiButtonHoverFeedback(GetOwnerID());
		}

		if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
			audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager_->GetVfxVolume(), false);
		}
	}
	// Restore the default texture once the pointer or focus leaves the button.
	else if (!over && hovered_) {
		hovered_ = false;
		TrySetTexture(owner, normalTexturePath_);
	}

	// Treat keyboard submit as the same activation path as a direct left-click.
	const bool keyboardSubmit = keyboardFocused && MenuKeyboardNavigation::ConsumeSubmitPress(input);
	if ((mouseOver && input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) || keyboardSubmit) {
		// Play the shared confirmation sounds before handing control to the scene transition.
		if (audioManager_) {
			if (audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager_->GetVfxVolume(), false);
			}
			if (audioManager_->HasSound(MyoonchiPaths::Audio::SFX_START_BUTTON)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_START_BUTTON, audioManager_->GetVfxVolume(), false);
			}
			audioManager_->FadeChannel(MyoonchiPaths::Audio::BGM_MAIN_MENU, 0.0f, 0.35f);
		}

		// Consume the click so deeper UI layers do not also react to the same press.
		if (mouseOver) {
			input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
		}

		// Hand off to the direct level transition path for non-cutscene menu buttons.
		scene.StartLevelTransition(targetJson_, activateSimulation_, 0.35f, 0.35f);
	}
}
