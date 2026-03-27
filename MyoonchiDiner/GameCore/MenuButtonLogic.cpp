/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			MenuButtonLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (95%)
 CO-AUTHORS:		Ng Juin Herng, juinherng.ng@digipen.edu (5%)

 DESCRIPTION:		Implements MenuButtonLogic::Update for release builds only, handling mouse click registering logic
					against button AABB, hover texture transitions using ResourceManager, and triggering a cutscene
					followed by JSON level load after left-click.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <string>

#include "EngineCore/AudioManager.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/MenuButtonLogic.hpp"
#include "GamePaths.hpp"

#ifndef _DEBUG
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
#endif // _DEBUG

void MenuButtonLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
#ifdef _DEBUG
	(void)scene; (void)input;
#else
	if (scene.IsHowToPlayOverlayActive()) {
		return;
	}

	if (!initialized_) {
		normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
		hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
		initialized_ = true;
	}

	glm::vec2 mouseWorld{};
	bool insideScene = false;

	if (input.IsReplayOverride()) {
		const glm::dvec2 replayMousePos = input.GetMousePosition();
		insideScene = scene.GetGraphicsEngine().GetMouseWorldInScene(mouseWorld, &replayMousePos);
	}
	else {
		insideScene = scene.GetGraphicsEngine().GetMouseWorldInScene(mouseWorld, nullptr);
	}

	if (!insideScene) {
		return;
	}

	GameObject* owner = GetOwner(scene);
	if (!owner) {
		return;
	}

	const glm::vec3 pos = owner->GetPositionGLM();
	const glm::vec3 sz = owner->GetScaleGLM();
	const float halfW = sz.x * 0.5f;
	const float halfH = sz.y * 0.5f;

	const bool over =
		mouseWorld.x >= (pos.x - halfW) && mouseWorld.x <= (pos.x + halfW) &&
		mouseWorld.y >= (pos.y - halfH) && mouseWorld.y <= (pos.y + halfH);

	if (over && !hovered_) {
		hovered_ = true;
		TrySetTexture(owner, hoverTexturePath_);
	}
	else if (!over && hovered_) {
		hovered_ = false;
		TrySetTexture(owner, normalTexturePath_);
	}

	if (over && input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		if (audioManager_) {
			audioManager_->PlayUIClickSound();
			if (audioManager_->HasSound(MyoonchiPaths::Audio::SFX_START_BUTTON)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_START_BUTTON, audioManager_->GetVfxVolume(), false);
			}
			audioManager_->FadeChannel(MyoonchiPaths::Audio::BGM_MAIN_MENU, 0.0f, 0.35f);
		}

		input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);

		// Direct transition only. No intro cutscene here.
		scene.StartLevelTransition(targetJson_, activateSimulation_, 0.35f, 0.35f);
	}
#endif // _DEBUG
}
