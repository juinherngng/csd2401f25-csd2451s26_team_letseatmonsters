/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			MenuButtonLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Ng Juin Herng, juinherng.ng@digipen.edu

 DESCRIPTION:		 Implements MenuButtonLogic::Update for release builds only, handling mouse click registering logic
					 against button AABB, hover texture transitions using ResourceManager, and triggering a cutscene
					 followed by JSON level load after left-click.

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "MenuButtonLogic.hpp"
#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"
#include "AudioManager.hpp"

#include <string>
#include <vector>

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

void MenuButtonLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
#ifdef _DEBUG
	// Debug build: keep inert (no hover, no click).
	(void)scene;
	(void)input;
#else
	// When HowToPlay overlay is active, ignore all menu buttons
	if (scene.IsHowToPlayOverlayActive()) {
		return;
	}

	// Lazy init texture paths
	if (!initialized_) {
		normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
		hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
		initialized_ = true;
	}

	// Compute mouse position in world coords
	glm::vec2 mouseWorld{};
	bool insideScene = GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld);
	if (!insideScene) {
		glm::vec3 w = input.ScreenToWorld(
			static_cast<float>(input.GetMousePosition().x),
			static_cast<float>(input.GetMousePosition().y));
		mouseWorld = glm::vec2(w.x, w.y);
	}

	GameObject* owner = GetOwner(scene);
	if (!owner) {
		return;
	}

	// AABB hit-test using position + size (width/height)
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
		// Try swap to hover texture (if it exists). If not found, keep normal silently.
		TrySetTexture(owner, hoverTexturePath_);
	}
	else if (!over && hovered_) {
		hovered_ = false;
		// Restore normal texture
		TrySetTexture(owner, normalTexturePath_);
	}

	// Click to trigger cutscene followed by level load
	if (over && input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		// Play UI click sound
		if (audioManager_) {
			audioManager_->PlayUIClickSound();
		}

		input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT); // Prevent carry-over

		// Cutscene images
		const std::vector<std::string> cutsceneImages = {
			"../assets/Cutscene_starting_1.png",
			"../assets/Cutscene_starting_2.png",
			"../assets/Cutscene_starting_3.png",
			"../assets/Cutscene_starting_4.png",
			"../assets/Cutscene_starting_5.png",
			"../assets/Cutscene_starting_6.png",
		};

		// Show each for 1.5s with a 0.5s fade (Scene will load targetJson_ when finished)
		scene.StartCutsceneTransitioned(cutsceneImages, targetJson_, activateSimulation_, 0.35f, 0.35f);
	}
#endif // _DEBUG
}
