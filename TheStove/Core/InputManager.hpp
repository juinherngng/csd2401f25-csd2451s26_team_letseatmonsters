/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			InputManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Centralized keyboard/mouse input state tracker with edge detection.
					- Polls GLFW each frame and mirrors common key/mouse states.
					- Respects ImGui IO capture flags to avoid consuming UI input.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <unordered_map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "imgui.h"
#include "System.hpp"
#include "../Graphics/GraphicsEngine.hpp"

 /**
  * @class InputManager
  * @brief Manages input polling for keyboard and mouse using GLFW.
  *
  * Provides per-frame input update, state tracking, and query functions
  * for detecting presses, releases, and edge transitions.
  */
class InputManager : public CoreFramework::SystemInterface {
public:
	// Lifetime / Access
	InputManager();
	static InputManager& Get();

	// SystemInterface implementation
	void Initialize() override;
	void Update(float dt) override;
	std::string GetName() override;

	// Frame Update / Focus Hints
	void SetSceneViewportWantsGameMouse(bool enable);
	void SetWindow(GLFWwindow* window);

	// Keyboard Queries
	bool IsKeyPressed(int key) const;
	bool IsKeyJustPressed(int key) const;

	// Mouse Queries
	bool IsMouseButtonPressed(int button) const;
	bool IsMouseButtonJustPressed(int button) const;
	bool IsMouseButtonJustReleased(int button) const;
	glm::dvec2 GetMousePosition() const;

	// Coordinate Conversion
	glm::vec3 ScreenToWorld(float mouseX, float mouseY) const;

private:
	// Internal update method that takes window
	void UpdateInternal(GLFWwindow* window);

	// Data Members
	static InputManager* sActive;
	bool mSceneViewportWantsGameMouse = false;
	GLFWwindow* mWindow = nullptr;

	// Current/previous keyboard states (by GLFW key code)
	std::unordered_map<int, bool> mCurrentKeyStates;
	std::unordered_map<int, bool> mPreviousKeyStates;

	// Current/previous mouse button states (by GLFW button code)
	std::unordered_map<int, bool> mMouseButtons;
	std::unordered_map<int, bool> mPrevMouseButtons;

	// Mouse position in window coordinates (pixels)
	glm::dvec2 mMousePos{ 0.0, 0.0 };
};
