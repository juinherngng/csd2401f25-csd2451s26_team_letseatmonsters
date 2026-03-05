/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PauseButtonLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Implements PauseButtonLogic::Update for release builds only, providing AABB mouse click registering logic,
					hover texture swapping, and button actions logic: resume simulation/hide overlay, load settings JSON,
					or close GLFW window on Quit.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */


#include "Graphics/GraphicsEngine.hpp"
#include "Graphics/ResourceManager.hpp"
#include "Graphics/SceneManager.hpp"

#include "PauseButtonLogic.hpp"

#include <string>

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
#endif 

void PauseButtonLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
#ifdef _DEBUG
	// Debug build: inert
	(void)scene;
	(void)input;
#else
	// Lazy init from entity metadata
	if (!initialized_) {
		normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
		hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
		initialized_ = true;
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

	// Click
	if (!over || !input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		return;
	}

	// Consume click so it won't leak into gameplay after resume
	input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);

	switch (action_) {
	case PauseAction::Resume:
		scene.SetSimulationActive(true);
		scene.HidePauseOverlay();
		// Also clear input so no stray edges remain
		input.ClearState();
		break;

	case PauseAction::HowToPlay:
		// (Optional) You can hook this to your HowToPlay overlay too
		// e.g. scene.ShowHowToPlayFromPause();
		break;

	case PauseAction::Quit:
	{
		// Close the window safely
		if (GLFWwindow* win = glfwGetCurrentContext()) {
			glfwSetWindowShouldClose(win, GLFW_TRUE);
		}
	} break;
	}
#endif // _DEBUG
}
