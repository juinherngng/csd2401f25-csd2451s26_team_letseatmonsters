#include "InputManager.h"
#include <iostream>

void InputManager::Update(GLFWwindow* window) {

	// Put keys here; expand as needed
	int keys[] = { GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D };

	for (int key : keys) {
		bool state = glfwGetKey(window, key) == GLFW_PRESS;
		mPreviousKeyStates[key] = mCurrentKeyStates[key];
		mCurrentKeyStates[key] = state;

		//// Debug print
		//if (state) {
		//    std::cout << "Key " << key << " pressed with window " << window << std::endl;
		//}
		//else {
		//    std::cout << "Key " << key << " released with window " << window << std::endl;
		//}
	}

	// --- NEW: mouse button & cursor position ---
	int buttons[] = { GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT, GLFW_MOUSE_BUTTON_MIDDLE };
	for (int b : buttons) {
		bool state = glfwGetMouseButton(window, b) == GLFW_PRESS;
		mPrevMouseButtons[b] = mMouseButtons[b];
		mMouseButtons[b] = state;
	}

	glfwGetCursorPos(window, &mMousePos.x, &mMousePos.y);

	// Optional: carry current key states to previous each frame,
	// if you want IsKeyJustPressed to work (you already store maps).
	mPreviousKeyStates = mCurrentKeyStates;
}

bool InputManager::IsKeyPressed(int key) const {
	auto it = mCurrentKeyStates.find(key);
	return (it != mCurrentKeyStates.end()) && it->second;
}

bool InputManager::IsKeyJustPressed(int key) const {
	bool curr = false, prev = false;

	auto currIt = mCurrentKeyStates.find(key);
	if (currIt != mCurrentKeyStates.end()) curr = currIt->second;

	auto prevIt = mPreviousKeyStates.find(key);
	if (prevIt != mPreviousKeyStates.end()) prev = prevIt->second;

	return curr && !prev;
}

bool InputManager::IsMouseButtonPressed(int button) const {
	auto it = mMouseButtons.find(button);
	return (it != mMouseButtons.end()) && it->second;
}

bool InputManager::IsMouseButtonJustPressed(int button) const {
	auto itC = mMouseButtons.find(button);
	auto itP = mPrevMouseButtons.find(button);
	bool curr = (itC != mMouseButtons.end()) && itC->second;
	bool prev = (itP != mPrevMouseButtons.end()) && itP->second;
	return curr && !prev;
}

glm::dvec2 InputManager::GetMousePosition() const {
	return mMousePos;
}
