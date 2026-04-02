/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         HowToPlayButtonLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Handles hover and click behaviour for the How To Play
					button in the main menu. On click, it spawns a fullscreen
					HowToPlay overlay image; clicking again (or pressing Esc)
					closes the overlay. Also manages hover texture swapping and
					ignores input while the overlay is active.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineCore/FilePaths.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/HowToPlayButtonLogic.hpp"
#include "MyoonchiDiner/GamePaths.hpp"

#ifndef _DEBUG
namespace {
	static std::string MakeHoverPath(const std::string& path) {
		if (path.empty()) return path;

		const size_t dot = path.find_last_of('.');
		const std::string ext = (dot != std::string::npos) ? path.substr(dot) : std::string();
		const std::string base = (dot != std::string::npos) ? path.substr(0, dot) : path;

		// If already ends with "_h", keep it
		if (base.size() >= 2 && base.substr(base.size() - 2) == "_h") {
			return dot != std::string::npos ? path : (base + ext);
		}

		// If ends with "_s", replace with "_h"
		if (base.size() >= 2 && base.substr(base.size() - 2) == "_s") {
			return base.substr(0, base.size() - 2) + "_h" + ext;
		}

		// Fallback: append "_h" before extension
		return base + "_h" + ext;
	}

	static void TrySetTexture(GameObject* owner, const std::string& texPath) {
		if (!owner || texPath.empty()) return;
		// Match EntityManager naming convention for static sprite textures
		std::string cacheName = "staticsprite_" + texPath;
		if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
			owner->SetTexture(tex);
		}
	}
}
#endif // _DEBUG

void HowToPlayButtonLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
#ifdef _DEBUG
	// In Debug build this script is currently inert.
	(void)scene;
	(void)input;
#else
	GameObject* owner = GetOwner(scene);
	if (!owner) {
		return;
	}

	const bool overlayActive = scene.IsHowToPlayOverlayActive();

	// If overlay is active but THIS instance did not spawn it,
	// ignore input, let the owner instance handle closing.
	if (overlayActive && overlayId_ < 0) {
		return;
	}

	// --- CASE 1: overlay already active -> treat any click or Esc as "close overlay" ---
	if (overlayActive) {
		const bool clickClose = input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);
		const bool escClose = input.IsKeyJustPressed(GLFW_KEY_ESCAPE);

		if (clickClose || escClose) {
			if (audioManager_) {
				if (audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_BACK)) {
					audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_BACK, audioManager_->GetVfxVolume(), false);
				}
			}

			if (overlayId_ >= 0) {
				scene.DespawnByID(overlayId_);
				overlayId_ = -1;
			}

			scene.SetHowToPlayOverlayActive(false);
			// Restore button texts on main menu (no-op in gameplay if no menu buttons)
			// scene.CreateMenuButtonTexts();

			if (clickClose) {
				input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
			}
			if (escClose) {
				input.ConsumeNextKeyPress(GLFW_KEY_ESCAPE);
			}
		}

		// While overlay is active, we do NOT want hover or button-click behaviour.
		return;
	}

	// --- CASE 2: overlay not active -> regular button hover + click to show overlay ---

	// Lazy init texture paths for button hover effect
	if (!initialized_) {
		normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
		hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
		initialized_ = true;

	}

	// Mouse position in world coords
	glm::vec2 mouseWorld{};
	bool insideScene = GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld);
	if (!insideScene) {
		glm::vec3 w = input.ScreenToWorld(
			static_cast<float>(input.GetMousePosition().x),
			static_cast<float>(input.GetMousePosition().y));
		mouseWorld = glm::vec2(w.x, w.y);
	}

	// AABB hit-test using button position + size
	const glm::vec3 pos = owner->GetPositionGLM();
	const glm::vec3 sz = owner->GetScaleGLM();
	const float halfW = sz.x * 0.5f;
	const float halfH = sz.y * 0.5f;

	const bool over =
		mouseWorld.x >= (pos.x - halfW) && mouseWorld.x <= (pos.x + halfW) &&
		mouseWorld.y >= (pos.y - halfH) && mouseWorld.y <= (pos.y + halfH);

	// Hover visual swap
	if (over && !hovered_) {
		hovered_ = true;
		TrySetTexture(owner, hoverTexturePath_);
		if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
			audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager_->GetVfxVolume(), false);
		}
	}
	else if (!over && hovered_) {
		hovered_ = false;
		TrySetTexture(owner, normalTexturePath_);
	}

	// Click to show overlay
	if (!over || !input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		return;
	}

	if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
		audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager_->GetVfxVolume(), false);
	}

	input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);

	// Spawn full-screen HowToPlay overlay on top UI layer
	const std::string uiLayer = "9999999";
	const float w = static_cast<float>(GraphicsEngine::kRefW);
	const float h = static_cast<float>(GraphicsEngine::kRefH);

	GameObject* img = scene.SpawnStaticSprite(
		FilePaths::Textures::HOW_TO_PLAY,      // change path if needed
		{ w * 0.5f, h * 0.5f, 0.0f },          // center
		{ w, h },                              // full screen
		uiLayer);

	if (!img) {
		return; // failed to spawn (wrong path etc.)
	}

	overlayId_ = img->GetID();
	img->SetMovableByPhysics(false);

	// Mark overlay active in the scene and hide menu button texts
	scene.SetHowToPlayOverlayActive(true);
	//scene.ClearMenuButtonTexts();
#endif // _DEBUG
}
