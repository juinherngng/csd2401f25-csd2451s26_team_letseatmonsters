#include "InputManager.hpp"
#include <iostream>

void InputManager::Update(GLFWwindow* window) {
	// Keys to poll (expand as needed)
	int keys[] = {
		GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN,
		GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_P, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3
	};

	for (int key : keys) {
		bool state = glfwGetKey(window, key) == GLFW_PRESS;
		mPreviousKeyStates[key] = mCurrentKeyStates[key]; // carry previous
		mCurrentKeyStates[key] = state;					  // update current

		//// Debug print
		//if (state) {
		//    std::cout << "Key " << key << " pressed with window " << window << std::endl;
		//}
		//else {
		//    std::cout << "Key " << key << " released with window " << window << std::endl;
		//}
	}

	// Mouse buttons (extendable)
	int buttons[] = { GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT, GLFW_MOUSE_BUTTON_MIDDLE };
	for (int b : buttons) {
		bool state = glfwGetMouseButton(window, b) == GLFW_PRESS;
		mPrevMouseButtons[b] = mMouseButtons[b]; // carry previous
		mMouseButtons[b] = state;                // update current
	}

	// Cursor position
	glfwGetCursorPos(window, &mMousePos.x, &mMousePos.y);

	// Optional: Sync entire key maps (safeguard)
	// Already covered in loop above, but keeps consistency
	mPreviousKeyStates = mCurrentKeyStates;
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
