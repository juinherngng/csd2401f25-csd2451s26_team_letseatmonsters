/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			InputControls.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Glenn Yeo Yi Heng, g.yeo@digipen.edu (100%)

 DESCRIPTION:		Input Control function declarations.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <GLFW/glfw3.h>

class InputControls {
public:
	/**
	 * @brief Polls the current WASD keyboard state from the given window.
	 * @param window GLFW window to query.
	 */
	static void PollKeyboard(GLFWwindow* window);

	/**
	 * @brief Handles mouse-button callback updates for cached input state.
	 * @param window GLFW window that received the callback.
	 * @param button Mouse button that changed state.
	 * @param action GLFW action value such as press or release.
	 */
	static void MouseButtonCallback(GLFWwindow* window, int button, int action);

	/**
	 * @brief Returns whether the left mouse button is currently pressed.
	 * @return True if the left mouse button is pressed.
	 */
	static bool LeftMousePressed();

	/**
	 * @brief Returns whether the W key is currently pressed.
	 * @return True if W is pressed.
	 */
	static bool WKeyPressed();

	/**
	 * @brief Returns whether the A key is currently pressed.
	 * @return True if A is pressed.
	 */
	static bool AKeyPressed();

	/**
	 * @brief Returns whether the S key is currently pressed.
	 * @return True if S is pressed.
	 */
	static bool SKeyPressed();

	/**
	 * @brief Returns whether the D key is currently pressed.
	 * @return True if D is pressed.
	 */
	static bool DKeyPressed();

	/**
	 * @brief Returns the cached mouse X position recorded on click.
	 * @return Mouse X position in window coordinates.
	 */
	static double MouseXPos();

	/**
	 * @brief Returns the cached mouse Y position recorded on click.
	 * @return Mouse Y position in window coordinates.
	 */
	static double MouseYPos();

private:
	static bool leftMouse;
	static bool w, a, s, d;
	static double mouseX, mouseY;
};
