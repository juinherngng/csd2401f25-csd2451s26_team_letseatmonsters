/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PauseButtonLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:		Vu Phan Hung, phanhung.vu@digipen.edu   (25%)
					Yat Chun Wee, y.chunwee@digipen.edu		(20%)
					Ng Juin Herng, juinherng.ng@digipen.edu (15%)

 DESCRIPTION:		Implements PauseButtonLogic::Update for release builds only, providing AABB mouse click registering logic,
					hover texture swapping, and button actions logic: resume simulation/hide overlay, load settings JSON,
					or close GLFW window on Quit.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <string>
#include <vector>

#include "EngineCore/AudioManager.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/MenuKeyboardNavigation.hpp"
#include "GameCore/PauseButtonLogic.hpp"
#include "GameCore/PlayerLogic.hpp"
#include "MyoonchiDiner/GamePaths.hpp"

namespace {
	static int gPauseResumeButtonIdForCurrentOverlay = -1;

	/**
	 * @brief Builds the hover-state texture path corresponding to a normal button texture.
	 * @param path Authored normal-state texture path.
	 * @return Texture path for the matching hover-state texture.
	 */
	static std::string MakeHoverPath(const std::string& path) {
		// Preserve empty paths so callers can skip texture swaps safely.
		if (path.empty()) return path;

		const size_t dot = path.find_last_of('.');
		const std::string ext = (dot != std::string::npos) ? path.substr(dot) : std::string();
		const std::string base = (dot != std::string::npos) ? path.substr(0, dot) : path;

		if (base.size() >= 2 && base.substr(base.size() - 2) == "_h") {
			return dot != std::string::npos ? path : (base + ext);
		}
		if (base.size() >= 2 && base.substr(base.size() - 2) == "_s") {
			return base.substr(0, base.size() - 2) + "_h" + ext;
		}
		return base + "_h" + ext;
	}

	/**
	 * @brief Applies a texture to a button GameObject when the texture path is valid.
	 * @param owner Button GameObject that should receive the texture.
	 * @param texPath Texture path to load and apply.
	 */
	static void TrySetTexture(GameObject* owner, const std::string& texPath) {
		// Skip loading when the object or path is missing to keep hover transitions resilient.
		if (!owner || texPath.empty()) return;
		std::string cacheName = "staticsprite_" + texPath;
		if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
			owner->SetTexture(tex);
		}
	}
}

/**
 * @brief Updates hover feedback, keyboard focus, and click handling for one frame.
 * @param dt Unused delta time for the frame.
 * @param scene Active scene containing the pause overlay.
 * @param input Input manager used for mouse and keyboard interaction.
 */
void PauseButtonLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
	// Cache the button's normal and hover textures the first time the overlay updates it.
	if (!initialized_) {
		normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
		hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
		initialized_ = true;
	}

	// Pause buttons only run their interactive behavior in runtime parity mode.
	if (!scene.ShouldUseRuntimeParityMode()) {
		if (hovered_) {
			hovered_ = false;
			if (GameObject* owner = GetOwner(scene)) {
				TrySetTexture(owner, normalTexturePath_);
			}
		}
		return;
	}

	// Tear down any hover state as soon as the pause overlay disappears.
	if (!scene.IsPauseOverlayActive()) {
		if (action_ == PauseAction::Resume) {
			gPauseResumeButtonIdForCurrentOverlay = -1;
		}

		if (hovered_) {
			hovered_ = false;
			if (GameObject* owner = GetOwner(scene)) {
				TrySetTexture(owner, normalTexturePath_);
			}
		}
		return;
	}

	// Reset pause-overlay keyboard focus whenever a fresh Resume button instance becomes active.
	if (action_ == PauseAction::Resume && gPauseResumeButtonIdForCurrentOverlay != GetOwnerID()) {
		MenuKeyboardNavigation::ClearFocus(MenuKeyboardNavigation::GetPauseOverlayScopeKey(scene));
		gPauseResumeButtonIdForCurrentOverlay = GetOwnerID();
	}

	// Suspend pause-button interaction while a higher-priority overlay or modal is on top.
	if (scene.IsHowToPlayOverlayActive() || scene.IsMenuModalActive()) {
		if (hovered_) {
			hovered_ = false;
			if (GameObject* owner = GetOwner(scene)) {
				TrySetTexture(owner, normalTexturePath_);
			}
		}
		return;
	}

	// Compute the current mouse position in scene space for hover hit-testing.
	glm::vec2 mouseWorld{};
	bool insideScene = GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld);
	if (!insideScene) {
		glm::vec3 w = input.ScreenToWorld(
			static_cast<float>(input.GetMousePosition().x),
			static_cast<float>(input.GetMousePosition().y));
		mouseWorld = glm::vec2(w.x, w.y);
	}

	GameObject* owner = GetOwner(scene);
	if (!owner) return;

	// Build a simple AABB from the button's position and scale.
	const glm::vec3 pos = owner->GetPositionGLM();
	const glm::vec3 sz = owner->GetScaleGLM();
	const float halfW = sz.x * 0.5f;
	const float halfH = sz.y * 0.5f;

	const bool mouseOver =
		mouseWorld.x >= (pos.x - halfW) && mouseWorld.x <= (pos.x + halfW) &&
		mouseWorld.y >= (pos.y - halfH) && mouseWorld.y <= (pos.y + halfH);

	// Merge mouse hover and keyboard focus so either interaction mode can highlight the button.
	const std::vector<int> buttonIds = MenuKeyboardNavigation::CollectPauseOverlayButtons(scene);
	const int focusedButtonId = MenuKeyboardNavigation::UpdateFocus(
		scene,
		input,
		MenuKeyboardNavigation::GetPauseOverlayScopeKey(scene),
		buttonIds,
		mouseOver ? GetOwnerID() : -1);
	const bool keyboardFocused = (focusedButtonId == GetOwnerID());
	const bool over = mouseOver || keyboardFocused;

	// Enter hover state once and trigger the usual highlight feedback.
	if (over && !hovered_) {
		hovered_ = true;
		TrySetTexture(owner, hoverTexturePath_);
		scene.TriggerUiButtonHoverFeedback(GetOwnerID());
		if (AudioManager* audioManager = scene.GetAudioManager()) {
			if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
				audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager->GetVfxVolume(), false);
			}
		}
	}
	// Restore the idle texture when the button is no longer hovered or focused.
	else if (!over && hovered_) {
		hovered_ = false;
		TrySetTexture(owner, normalTexturePath_);
	}

	// Accept either a mouse click or keyboard submit when this button has focus.
	const bool keyboardSubmit = keyboardFocused && MenuKeyboardNavigation::ConsumeSubmitPress(input);
	if ((!mouseOver || !input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) && !keyboardSubmit) {
		return;
	}

	// Consume the mouse click so it cannot leak into gameplay once the overlay closes.
	if (mouseOver) {
		input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
	}

	// Dispatch the authored pause-menu action for this button.
	switch (action_) {
	case PauseAction::Resume:
		if (AudioManager* audioManager = scene.GetAudioManager()) {
			if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_START_RESUME)) {
				audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_START_RESUME, audioManager->GetVfxVolume(), false);
			}
		}
		if (PlayerLogic* playerLogic = scene.GetLogicManager().GetLogicForObject<PlayerLogic>(scene.GetPlayerID())) {
			playerLogic->EnterPauseState(scene);
		}

		// Hide the overlay first, then request gameplay resume from the pause flow.
		scene.HidePauseOverlay();
		scene.RequestResumeFromPauseOverlay();
		break;

	case PauseAction::HowToPlay:
		if (AudioManager* audioManager = scene.GetAudioManager()) {
			if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
				audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager->GetVfxVolume(), false);
			}
		}
		// This action slot is reserved for the pause-menu how-to-play overlay flow.
		break;

	case PauseAction::Quit:
	{
		if (AudioManager* audioManager = scene.GetAudioManager()) {
			if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
				audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager->GetVfxVolume(), false);
			}
		}
		// Close the current GLFW window when the pause-menu quit button is confirmed.
		if (GLFWwindow* win = glfwGetCurrentContext()) {
			glfwSetWindowShouldClose(win, GLFW_TRUE);
		}
	} break;
	}
}
