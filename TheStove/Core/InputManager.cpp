/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			InputManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Implements the InputManager class for handling keyboard and mouse input.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "InputManager.hpp"
#include <iostream>

void InputManager::Update(GLFWwindow* window) {
	mPreviousKeyStates = mCurrentKeyStates;
	mPrevMouseButtons = mMouseButtons;

	// Poll commonly used keys
	int keys[] = {
		GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN,
		GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_P, GLFW_KEY_R, GLFW_KEY_T,
		GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3
	};

	for (int key : keys) {
		mCurrentKeyStates[key] = (glfwGetKey(window, key) == GLFW_PRESS);
	}

	// Poll mouse buttons
	int buttons[] = { GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT, GLFW_MOUSE_BUTTON_MIDDLE };
	for (int b : buttons) {
		mMouseButtons[b] = (glfwGetMouseButton(window, b) == GLFW_PRESS);
	}

	// Get mouse cursor position
	glfwGetCursorPos(window, &mMousePos.x, &mMousePos.y);
}

// Returns whether a key is currently pressed.
bool InputManager::IsKeyPressed(int key) const {
	auto it = mCurrentKeyStates.find(key);
	return (it != mCurrentKeyStates.end()) && it->second;
}

// Returns whether a key transitioned from up to down this frame.
bool InputManager::IsKeyJustPressed(int key) const {
	bool curr = false;
	bool prev = false;

	auto currIt = mCurrentKeyStates.find(key);
	if (currIt != mCurrentKeyStates.end()) curr = currIt->second;

	auto prevIt = mPreviousKeyStates.find(key);
	if (prevIt != mPreviousKeyStates.end()) prev = prevIt->second;

	return curr && !prev;
}

// Returns whether a mouse button is currently pressed.
bool InputManager::IsMouseButtonPressed(int button) const {
	auto it = mMouseButtons.find(button);
	return (it != mMouseButtons.end()) && it->second;
}

// Returns whether a mouse button was pressed this frame (edge).
bool InputManager::IsMouseButtonJustPressed(int button) const {
	auto itC = mMouseButtons.find(button);
	auto itP = mPrevMouseButtons.find(button);

	bool curr = (itC != mMouseButtons.end()) && itC->second;
	bool prev = (itP != mPrevMouseButtons.end()) && itP->second;

	return curr && !prev;
}

// Get the current mouse cursor position in window coordinates.
glm::dvec2 InputManager::GetMousePosition() const {
	return mMousePos;
}
