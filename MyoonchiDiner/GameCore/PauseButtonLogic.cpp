/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PauseButtonLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

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
	static std::string MakeHoverPath(const std::string& path) {
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

	static void TrySetTexture(GameObject* owner, const std::string& texPath) {
		if (!owner || texPath.empty()) return;
		std::string cacheName = "staticsprite_" + texPath;
		if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
			owner->SetTexture(tex);
		}
	}
}

void PauseButtonLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
	// Lazy init from entity metadata
	if (!initialized_) {
		normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
		hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
		initialized_ = true;
	}

	if (!scene.ShouldUseRuntimeParityMode()) {
		if (hovered_) {
			hovered_ = false;
			if (GameObject* owner = GetOwner(scene)) {
				TrySetTexture(owner, normalTexturePath_);
			}
		}
		return;
	}

	if (scene.IsHowToPlayOverlayActive() || scene.IsMenuModalActive()) {
		if (hovered_) {
			hovered_ = false;
			if (GameObject* owner = GetOwner(scene)) {
				TrySetTexture(owner, normalTexturePath_);
			}
		}
		return;
	}

	// Hover hit-test
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

	const glm::vec3 pos = owner->GetPositionGLM();
	const glm::vec3 sz = owner->GetScaleGLM();
	const float halfW = sz.x * 0.5f;
	const float halfH = sz.y * 0.5f;

	const bool mouseOver =
		mouseWorld.x >= (pos.x - halfW) && mouseWorld.x <= (pos.x + halfW) &&
		mouseWorld.y >= (pos.y - halfH) && mouseWorld.y <= (pos.y + halfH);

	const std::vector<int> buttonIds = MenuKeyboardNavigation::CollectPauseOverlayButtons(scene);
	const int focusedButtonId = MenuKeyboardNavigation::UpdateFocus(
		scene,
		input,
		MenuKeyboardNavigation::GetPauseOverlayScopeKey(scene),
		buttonIds,
		mouseOver ? GetOwnerID() : -1);
	const bool keyboardFocused = (focusedButtonId == GetOwnerID());
	const bool over = mouseOver || keyboardFocused;

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
	else if (!over && hovered_) {
		hovered_ = false;
		TrySetTexture(owner, normalTexturePath_);
	}

	// Click
	const bool keyboardSubmit = keyboardFocused && MenuKeyboardNavigation::ConsumeSubmitPress(input);
	if ((!mouseOver || !input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) && !keyboardSubmit) {
		return;
	}

	// Consume click so it won't leak into gameplay after resume
	if (mouseOver) {
		input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
	}

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

		scene.HidePauseOverlay();
		scene.RequestResumeFromPauseOverlay();
		// Fully clear transient input after clicking Resume so gameplay
		// does not receive stale mouse/key edges from the pause UI frame.
		input.ClearState();
		break;

	case PauseAction::HowToPlay:
		if (AudioManager* audioManager = scene.GetAudioManager()) {
			if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
				audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager->GetVfxVolume(), false);
			}
		}
		// (Optional) You can hook this to your HowToPlay overlay too
		// e.g. scene.ShowHowToPlayFromPause();
		break;

	case PauseAction::Quit:
	{
		if (AudioManager* audioManager = scene.GetAudioManager()) {
			if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
				audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager->GetVfxVolume(), false);
			}
		}
		// Close the window safely
		if (GLFWwindow* win = glfwGetCurrentContext()) {
			glfwSetWindowShouldClose(win, GLFW_TRUE);
		}
	} break;
	}
}
