/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			InputControls.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Glenn Yeo Yi Heng, g.yeo@digipen.edu (100%)

 DESCRIPTION:		Input Control function definitions.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "EngineCore/InputControls.hpp"

// Define and initalized at start for static members
bool InputControls::leftMouse = false;
bool InputControls::w = false;
bool InputControls::a = false;
bool InputControls::s = false;
bool InputControls::d = false;
double InputControls::mouseX = 0.0;
double InputControls::mouseY = 0.0;

/*
How to use

must add Input::PollKeyboard(window) first to check keys

if (InputControls::WKeyPressed())
{
	// Player move up
}

if (InputControls::LeftMousePressed())
{
	// get mouse positions for ur local x and y
	x = InputControls::MouseX();
	y = InputControls::MouseY();

	// use the x and y for the movement and stuffz


}

*/

// Check for keyboard presses in window
void InputControls::PollKeyboard(GLFWwindow* window) {
	w = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
	a = (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS);
	s = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
	d = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS);
}

// Check for mouse Click in window
void InputControls::MouseButtonCallback(GLFWwindow* window, int button, int action) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			leftMouse = true;
			// Records mouse position at click
			glfwGetCursorPos(window, &mouseX, &mouseY);
		}
		else if (action == GLFW_RELEASE) {
			leftMouse = false;
		}
	}
}

// ---- Bool checks for WASD ----

bool InputControls::LeftMousePressed() {
	return leftMouse;
}

bool InputControls::WKeyPressed() {
	return w;
}

bool InputControls::AKeyPressed() {
	return a;
}

bool InputControls::SKeyPressed() {
	return s;
}

bool InputControls::DKeyPressed() {
	return d;
}

double InputControls::MouseXPos() {
	return mouseX;
}

double InputControls::MouseYPos() {
	return mouseY;
}
