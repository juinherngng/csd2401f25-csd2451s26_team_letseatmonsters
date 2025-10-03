/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			InputManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Declares the InputManager class responsible for handling keyboard
					and mouse input using GLFW.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <unordered_map>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


 /**
  * @class InputManager
  * @brief Manages input polling for keyboard and mouse using GLFW.
  *
  * Provides per-frame input update, state tracking, and query functions
  * for detecting presses, releases, and edge transitions.
  */
class InputManager {
public:
	/**
	 * @brief Update the internal state of keys and mouse buttons.
	 * @param window Pointer to the active GLFW window for input polling.
	 */
	void Update(GLFWwindow* window);

	/**
	 * @brief Check if a key is currently held down.
	 * @param key GLFW key code.
	 * @return True if pressed, false otherwise.
	 */
	bool IsKeyPressed(int key) const;

	/**
	 * @brief Check if a key transitioned from released to pressed this frame.
	 * @param key GLFW key code.
	 * @return True if just pressed, false otherwise.
	 */
	bool IsKeyJustPressed(int key) const;

	/**
	 * @brief Check if a mouse button is currently held down.
	 * @param button GLFW mouse button code.
	 * @return True if pressed, false otherwise.
	 */
	bool IsMouseButtonPressed(int button) const;

	/**
	 * @brief Check if a mouse button was pressed this frame.
	 * @param button GLFW mouse button code.
	 * @return True if just pressed, false otherwise.
	 */
	bool IsMouseButtonJustPressed(int button) const;

	/**
	 * @brief Get the current mouse cursor position in window coordinates.
	 * @return glm::dvec2 representing (x,y) position.
	 */
	glm::dvec2 GetMousePosition() const;

private:
	// Keyboard state tracking
	std::unordered_map<int, bool> mCurrentKeyStates;
	std::unordered_map<int, bool> mPreviousKeyStates;

	// Mouse button tracking
	std::unordered_map<int, bool> mMouseButtons;
	std::unordered_map<int, bool> mPrevMouseButtons;

	// Current mouse position in window coordinates
	glm::dvec2 mMousePos{ 0.0, 0.0 };
};
