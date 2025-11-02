/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GraphicsEngine.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Seah Wang Hua, wanghua.seah@digipen.edu

DESCRIPTION:		Initializes rendering, loads default GPU resources, manages a fullscreen background,
					and renders scene GameObjects with view/projection.

		All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "DebugRenderer.hpp"
#include "Renderer.hpp"
#include "ResourceManager.hpp"
#include "GameObject.hpp"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <vector>
#include <memory>

class GraphicsEngine {
public:
	GraphicsEngine();

	void Initialize();
	void BeginFrame();
	// Non-owning draw of a list of scene-owned objects.
	void Render(const std::vector<GameObject*>& objects);
	void Shutdown();

	// Background management
	void SetBackground(const std::string& texturePath);
	void ClearBackground();

	void BeginImGuiFrame(); // call at start of each frame
	void EndImGuiFrame(); // call at end of each frame

	void Resize(int width, int height);
	int GetWidth() const { return screenWidth; }
	int GetHeight() const { return screenHeight; }

	const glm::mat4& GetProjection() const;
	const glm::mat4& GetView() const;

	static GraphicsEngine& Instance();

private:
	Renderer renderer;
	ResourceManager& resourceManager;

	int screenWidth = 1200;
	int screenHeight = 800;

	// Game Object rendering
	std::vector<std::unique_ptr<GameObject>> gameObjects;
	// Background rendering
	std::unique_ptr<GameObject> backgroundObject;

	glm::mat4 view{ 1.0f };
	glm::mat4 projection{ 1.0f };

	void LoadDefaultResources();
};
