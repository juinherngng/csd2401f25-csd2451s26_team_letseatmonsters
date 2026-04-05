/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			InputControls.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Glenn Yeo Yi Heng, g.yeo@digipen.edu (70%)
 CO-AUTHOR:         Yat Chun Wee, y.chunwee@digipen.edu	 (30%)

 DESCRIPTION:		Input Control function definitions.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "EngineCore/InputControls.hpp"

// Define and initialize static cached input state.
bool InputControls::leftMouse = false;
bool InputControls::w = false;
bool InputControls::a = false;
bool InputControls::s = false;
bool InputControls::d = false;
double InputControls::mouseX = 0.0;
double InputControls::mouseY = 0.0;

/**
 * @brief Polls the current WASD keyboard state from the specified window.
 * @param window GLFW window to query.
 */
void InputControls::PollKeyboard(GLFWwindow* window) {
	// Snapshot the current movement-key state so other systems can query simple booleans.
	w = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
	a = (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS);
	s = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
	d = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS);
}

/**
 * @brief Updates cached mouse-button state from the GLFW callback.
 * @param window GLFW window that received the callback.
 * @param button Mouse button that changed state.
 * @param action GLFW action such as press or release.
 */
void InputControls::MouseButtonCallback(GLFWwindow* window, int button, int action) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			leftMouse = true;
			// Record the click position once so game code can reuse the same press location.
			glfwGetCursorPos(window, &mouseX, &mouseY);
		}
		else if (action == GLFW_RELEASE) {
			leftMouse = false;
		}
	}
}

/**
 * @brief Returns whether the left mouse button is currently pressed.
 * @return True if the left mouse button is pressed.
 */
bool InputControls::LeftMousePressed() {
	return leftMouse;
}

/**
 * @brief Returns whether the W key is currently pressed.
 * @return True if W is pressed.
 */
bool InputControls::WKeyPressed() {
	return w;
}

/**
 * @brief Returns whether the A key is currently pressed.
 * @return True if A is pressed.
 */
bool InputControls::AKeyPressed() {
	return a;
}

/**
 * @brief Returns whether the S key is currently pressed.
 * @return True if S is pressed.
 */
bool InputControls::SKeyPressed() {
	return s;
}

/**
 * @brief Returns whether the D key is currently pressed.
 * @return True if D is pressed.
 */
bool InputControls::DKeyPressed() {
	return d;
}

/**
 * @brief Returns the cached mouse X position.
 * @return Mouse X position in window coordinates.
 */
double InputControls::MouseXPos() {
	return mouseX;
}

/**
 * @brief Returns the cached mouse Y position.
 * @return Mouse Y position in window coordinates.
 */
double InputControls::MouseYPos() {
	return mouseY;
}
