/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			InputManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (30%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(60%)
					Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:		Implements the InputManager class for handling keyboard and mouse input.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <iostream>

#include "InputManager.hpp"

#if defined(_DEBUG)
#include <imgui.h>
#endif

// Lifetime / Access
InputManager* InputManager::sActive = nullptr;

InputManager::InputManager() {
	sActive = this;
}

InputManager& InputManager::Get() {
	static InputManager fallback;
	return sActive?*sActive:fallback;
}

// SystemInterface implementation
void InputManager::Initialize() {
	// Nothing to initialize - window will be set externally
}

void InputManager::Update(float dt) {
	(void)dt; // Suppress unused parameter warning	

	if (mWindow) {
		UpdateInternal(mWindow);
	}
}

std::string InputManager::GetName() {
	return "InputManager";
}

// Frame Update / Focus Hints
void InputManager::SetSceneViewportWantsGameMouse(bool enable) {
	mSceneViewportWantsGameMouse = enable;
}

void InputManager::SetWindow(GLFWwindow* window) {
	mWindow = window;
}

void InputManager::UpdateInternal(GLFWwindow* window) {
	mPreviousKeyStates = mCurrentKeyStates;
	mPrevMouseButtons = mMouseButtons;

#if defined(_DEBUG)
	ImGuiIO& io = ImGui::GetIO();
	bool wantCaptureKeyboard = io.WantCaptureKeyboard;
#else
	bool wantCaptureKeyboard = false;
#endif

	// Poll commonly used keys
	int keys[] = {
		GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN,
		GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D,
		// physics dt, collider, points/lines, force, level editor
		GLFW_KEY_P, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_F, GLFW_KEY_L,
		GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3,
		GLFW_KEY_ESCAPE,
		GLFW_KEY_F1,  // FPS display toggle in Release
		GLFW_KEY_SPACE
	};

	// If ImGui wants the keyboard, clear key states so gameplay won't react
	if (!wantCaptureKeyboard) {
		for (int key:keys) {
			mCurrentKeyStates[key] = (glfwGetKey(window, key) == GLFW_PRESS);
		}
	}
	else {
		for (int key:keys) {
			mCurrentKeyStates[key] = false;
		}
	}

	// Mouse buttons to track
	int buttons[] = { GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT, GLFW_MOUSE_BUTTON_MIDDLE };

	// Always track mouse button states, let individual systems check WantCaptureMouse themselves
	for (int b:buttons) {
		mMouseButtons[b] = (glfwGetMouseButton(window, b) == GLFW_PRESS);
	}

	// Get mouse cursor position
	glfwGetCursorPos(window, &mMousePos.x, &mMousePos.y);
}

void InputManager::ClearState() {
	mCurrentKeyStates.clear();
	mPreviousKeyStates.clear();
	mMouseButtons.clear();
	mPrevMouseButtons.clear();
	mMousePos = glm::dvec2(0.0, 0.0);
}

// Keyboard Queries
bool InputManager::IsKeyPressed(int key) const {
	auto it = mCurrentKeyStates.find(key);
	return (it != mCurrentKeyStates.end()) && it->second;
}

bool InputManager::IsKeyJustPressed(int key) const {
	bool curr = false;
	bool prev = false;

	auto currIt = mCurrentKeyStates.find(key);
	if (currIt != mCurrentKeyStates.end()) curr = currIt->second;

	auto prevIt = mPreviousKeyStates.find(key);
	if (prevIt != mPreviousKeyStates.end()) prev = prevIt->second;

	return curr && !prev;
}

// Mouse Queries
bool InputManager::IsMouseButtonPressed(int button) const {
	auto it = mMouseButtons.find(button);
	return (it != mMouseButtons.end()) && it->second;
}

bool InputManager::IsMouseButtonJustPressed(int button) const {
	auto itC = mMouseButtons.find(button);
	auto itP = mPrevMouseButtons.find(button);

	bool curr = (itC != mMouseButtons.end()) && itC->second;
	bool prev = (itP != mPrevMouseButtons.end()) && itP->second;

	bool justPressed = curr && !prev;

	// If flagged for consumption, hide this edge once
	if (justPressed) {
		if (mConsumeNextMousePress.find(button) != mConsumeNextMousePress.end()) {
			// Remove flag so only one press is consumed
			const_cast<std::unordered_set<int>&>(mConsumeNextMousePress).erase(button);
			return false;
		}
	}

	return justPressed;
}

bool InputManager::IsMouseButtonJustReleased(int button) const {
	auto itC = mMouseButtons.find(button);
	auto itP = mPrevMouseButtons.find(button);

	bool curr = (itC != mMouseButtons.end()) && itC->second;
	bool prev = (itP != mPrevMouseButtons.end()) && itP->second;

	return !curr && prev;
}

glm::dvec2 InputManager::GetMousePosition() const {
	return mMousePos;
}

// Coordinate Conversion
glm::vec3 InputManager::ScreenToWorld(float mouseX, float mouseY) const {
	const int w = GraphicsEngine::Instance().GetWidth();
	const int h = GraphicsEngine::Instance().GetHeight();

	// Normalize to -1..1 in NDC (OpenGL origin bottom-left)
	float x = (2.0f * mouseX) / static_cast<float>(w) - 1.0f;
	float y = 1.0f - (2.0f * mouseY) / static_cast<float>(h);
	glm::vec4 clipCoords(x, y, -1.0f, 1.0f);

	const glm::mat4 vpInv = glm::inverse(GraphicsEngine::Instance().GetProjection() * GraphicsEngine::Instance().GetView());

	glm::vec4 world = vpInv * clipCoords;

	if (world.w != 0.0f) {
		world /= world.w;
	}

	return glm::vec3(world.x, world.y, world.z);
}

void InputManager::ConsumeNextMousePress(int button) {
	mConsumeNextMousePress.insert(button);
}

void InputManager::ClearMouseConsume(int button) {
	mConsumeNextMousePress.erase(button);
}
