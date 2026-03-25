/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			InputManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (30%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(60%)
					Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:		Implements the InputManager class for handling keyboard and mouse input.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "InputManager.hpp"

#include <iostream>

#if defined(_DEBUG)
#include <imgui.h>
#endif

 // Helper to get button state from a map, defaulting to false if not found
namespace {

	/**
	 * @brief Returns button state.
	 * @param states Parameter for states.
	 * @param code Parameter for code.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool GetButtonState(const std::unordered_map<int, bool>& states, int code) {
		auto it = states.find(code);
		return (it != states.end()) && it->second;
	}

	// List of keys and mouse buttons we want to track.
	constexpr int kTrackedKeys[] = {
		GLFW_KEY_LEFT,
		GLFW_KEY_RIGHT,
		GLFW_KEY_UP,
		GLFW_KEY_DOWN,
		GLFW_KEY_W,
		GLFW_KEY_A,
		GLFW_KEY_S,
		GLFW_KEY_D,
		GLFW_KEY_P,		 // Pause menu in Release, debug toggle in Debug
		GLFW_KEY_G,		 // Grid toggle in Release, debug toggle in Debug
		GLFW_KEY_H,		 // Hitbox toggle in Release, debug toggle in Debug
		GLFW_KEY_F,		 // FPS toggle in Release, debug toggle in Debug
		GLFW_KEY_L,		 // Level reload in Release, debug toggle in Debug
		GLFW_KEY_1,	     // Debug shortcuts for quick testing of various conditions (e.g., spawn item, trigger event, etc.)
		GLFW_KEY_2,		 // Debug shortcuts for quick testing of various conditions (e.g., spawn item, trigger event, etc.)
		GLFW_KEY_3,		 // Debug shortcuts for quick testing of various conditions (e.g., spawn item, trigger event, etc.)
		GLFW_KEY_SPACE,  // Used for various gameplay actions, so we track it even in Debug
		GLFW_KEY_ESCAPE, // Menu toggle in Release
		GLFW_KEY_F1,	 // FPS display toggle in Release
		GLFW_KEY_F5,	 // Scene reload in Release, debug toggle in Debug
		GLFW_KEY_F6,	 // Level editor toggle
		GLFW_KEY_F10	 // Gameplay debug shortcut (instant win testing during gameplay)
	};

	// We track mouse buttons separately since they have different semantics and are often used in combination with ImGui's WantCaptureMouse.
	constexpr int kTrackedMouseButtons[] = {
		GLFW_MOUSE_BUTTON_LEFT,
		GLFW_MOUSE_BUTTON_RIGHT,
		GLFW_MOUSE_BUTTON_MIDDLE
	};
}

// Static instance pointer initialization
InputManager* InputManager::sActive = nullptr;

/**
 * @brief Performs input manager.
 * @return Result produced by this operation.
 */
InputManager::InputManager() {
	sActive = this;
}

/**
 * @brief Returns this object.
 * @return Requested value.
 */
InputManager& InputManager::Get() {
	static InputManager fallback;
	return sActive ? *sActive : fallback;
}

/**
 * @brief Initializes this object.
 * @return Result produced by this operation.
 */
void InputManager::Initialize() {
	// Nothing to initialize - window will be set externally
}

/**
 * @brief Updates this object.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void InputManager::Update(float dt) {
	(void)dt; // Suppress unused parameter warning	

	if (replayOverride_) {
		return;
	}

	if (mWindow) {
		UpdateInternal(mWindow);
	}
}

/**
 * @brief Returns the stable name for this object.
 * @return Requested value.
 */
std::string InputManager::GetName() {
	return "InputManager";
}

/**
 * @brief Sets scene viewport wants game mouse.
 * @param enable Boolean flag controlling whether the feature is enabled.
 * @return Result produced by this operation.
 */
void InputManager::SetSceneViewportWantsGameMouse(bool enable) {
	mSceneViewportWantsGameMouse = enable;
}

/**
 * @brief Sets window.
 * @param window Parameter for window.
 * @return Result produced by this operation.
 */
void InputManager::SetWindow(GLFWwindow* window) {
	mWindow = window;
}

/**
 * @brief Updates internal.
 * @param window Parameter for window.
 * @return Result produced by this operation.
 */
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
		for (int key : kTrackedKeys) {
			mCurrentKeyStates[key] = (glfwGetKey(window, key) == GLFW_PRESS);
		}
	}
	else {
		for (int key : kTrackedKeys) {
			mCurrentKeyStates[key] = false;
		}
	}

	// Always track mouse button states, let individual systems check WantCaptureMouse themselves
	for (int b : kTrackedMouseButtons) {
		mMouseButtons[b] = (glfwGetMouseButton(window, b) == GLFW_PRESS);
	}

	// Get mouse cursor position
	glfwGetCursorPos(window, &mMousePos.x, &mMousePos.y);
}

/**
 * @brief Clears state.
 * @return Result produced by this operation.
 */
void InputManager::ClearState() {
	mCurrentKeyStates.clear();
	mPreviousKeyStates.clear();
	mMouseButtons.clear();
	mPrevMouseButtons.clear();
	mConsumeNextMousePress.clear();
	mConsumeNextKeyPress.clear();
	mMousePos = glm::dvec2(0.0, 0.0);
}

/**
 * @brief Returns whether key pressed.
 * @param key Parameter for key.
 * @return True when the operation succeeds or the condition is met.
 */
bool InputManager::IsKeyPressed(int key) const {
	return GetButtonState(mCurrentKeyStates, key);
}

/**
 * @brief Returns whether key just pressed.
 * @param key Parameter for key.
 * @return True when the operation succeeds or the condition is met.
 */
bool InputManager::IsKeyJustPressed(int key) {
	const bool curr = GetButtonState(mCurrentKeyStates, key);
	const bool prev = GetButtonState(mPreviousKeyStates, key);
	const bool justPressed = curr && !prev;

	if (justPressed && mConsumeNextKeyPress.erase(key) > 0) {
		return false;
	}

	return justPressed;
}

/**
 * @brief Returns whether mouse button pressed.
 * @param button Parameter for button.
 * @return True when the operation succeeds or the condition is met.
 */
bool InputManager::IsMouseButtonPressed(int button) const {
	return GetButtonState(mMouseButtons, button);
}

/**
 * @brief Returns whether mouse button just pressed.
 * @param button Parameter for button.
 * @return True when the operation succeeds or the condition is met.
 */
bool InputManager::IsMouseButtonJustPressed(int button) {
	const bool curr = GetButtonState(mMouseButtons, button);
	const bool prev = GetButtonState(mPrevMouseButtons, button);
	const bool justPressed = curr && !prev;

	// If flagged for consumption, hide this edge once
	if (justPressed && mConsumeNextMousePress.erase(button) > 0) {
		return false;
	}

	return justPressed;
}

/**
 * @brief Returns whether mouse button just released.
 * @param button Parameter for button.
 * @return True when the operation succeeds or the condition is met.
 */
bool InputManager::IsMouseButtonJustReleased(int button) const {
	const bool curr = GetButtonState(mMouseButtons, button);
	const bool prev = GetButtonState(mPrevMouseButtons, button);

	return !curr && prev;
}

/**
 * @brief Returns mouse position.
 * @return Requested value.
 */
glm::dvec2 InputManager::GetMousePosition() const {
	return mMousePos;
}

/**
 * @brief Performs screen to world.
 * @param mouseX Parameter for mouse x.
 * @param mouseY Parameter for mouse y.
 * @return Result produced by this operation.
 */
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

/**
 * @brief Performs consume next mouse press.
 * @param button Parameter for button.
 * @return Result produced by this operation.
 */
void InputManager::ConsumeNextMousePress(int button) {
	const bool curr = GetButtonState(mMouseButtons, button);
	const bool prev = GetButtonState(mPrevMouseButtons, button);

	// If the edge happened this frame, consume it immediately so later
	// systems in the same frame do not see the click.
	if (curr && !prev) {
		mPrevMouseButtons[button] = true;
		return;
	}

	mConsumeNextMousePress.insert(button);
}

/**
 * @brief Clears mouse consume.
 * @param button Parameter for button.
 * @return Result produced by this operation.
 */
void InputManager::ClearMouseConsume(int button) {
	mConsumeNextMousePress.erase(button);
}

/**
 * @brief Performs consume next key press.
 * @param key Parameter for key.
 * @return Result produced by this operation.
 */
void InputManager::ConsumeNextKeyPress(int key) {
	const bool curr = GetButtonState(mCurrentKeyStates, key);
	const bool prev = GetButtonState(mPreviousKeyStates, key);

	// If the edge happened this frame, consume it immediately so later
	// systems in the same frame do not see the key press.
	if (curr && !prev) {
		mPreviousKeyStates[key] = true;
		return;
	}

	mConsumeNextKeyPress.insert(key);
}

/**
 * @brief Clears key consume.
 * @param key Parameter for key.
 * @return Result produced by this operation.
 */
void InputManager::ClearKeyConsume(int key) {
	mConsumeNextKeyPress.erase(key);
}

/**
 * @brief Performs capture snapshot.
 * @param out Output value for out.
 * @return Result produced by this operation.
 */
void InputManager::CaptureSnapshot(Snapshot& out) const {
	out.pressedKeys.clear();
	out.pressedMouseButtons.clear();

	for (const auto& [key, pressed] : mCurrentKeyStates) {
		if (pressed) {
			out.pressedKeys.push_back(key);
		}
	}

	for (const auto& [button, pressed] : mMouseButtons) {
		if (pressed) {
			out.pressedMouseButtons.push_back(button);
		}
	}

	out.mousePos = mMousePos;
}

/**
 * @brief Applies snapshot.
 * @param snapshot Parameter for snapshot.
 * @return Result produced by this operation.
 */
void InputManager::ApplySnapshot(const Snapshot& snapshot) {
	mPreviousKeyStates = mCurrentKeyStates;
	mPrevMouseButtons = mMouseButtons;

	mCurrentKeyStates.clear();
	mMouseButtons.clear();

	for (int key : snapshot.pressedKeys) {
		mCurrentKeyStates[key] = true;
	}

	for (int button : snapshot.pressedMouseButtons) {
		mMouseButtons[button] = true;
	}

	mMousePos = snapshot.mousePos;
}

/**
 * @brief Sets replay override.
 * @param enable Boolean flag controlling whether the feature is enabled.
 * @return Result produced by this operation.
 */
void InputManager::SetReplayOverride(bool enable) {
	replayOverride_ = enable;
}

/**
 * @brief Returns whether replay override.
 * @return True when the operation succeeds or the condition is met.
 */
bool InputManager::IsReplayOverride() const {
	return replayOverride_;
}

