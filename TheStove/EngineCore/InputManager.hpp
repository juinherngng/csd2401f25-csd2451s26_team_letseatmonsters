/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			InputManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(50%)
					Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:		Centralized keyboard/mouse input state tracker with edge detection.
					- Polls GLFW each frame and mirrors common key/mouse states.
					- Respects ImGui IO capture flags to avoid consuming UI input.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "EngineCore/System.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "imgui.h"

 // Forward declare Scene to avoid circular dependency
class InputManager : public CoreFramework::SystemInterface {
public:

	struct Snapshot {
		std::vector<int> pressedKeys;
		std::vector<int> pressedMouseButtons;
		glm::dvec2 mousePos{ 0.0, 0.0 };
	};

	/**
	 * @brief Constructs a `InputManager` instance.
	 */
	InputManager();

	/**
	 * @brief Returns this object.
	 * @return Requested value.
	 */
	static InputManager& Get();

	/**
	 * @brief Initializes this object.
	 */
	void Initialize() override;

	/**
	 * @brief Updates this object.
	 * @param dt Frame delta time in seconds.
	 */
	void Update(float dt) override;

	/**
	 * @brief Returns the stable name for this object.
	 * @return Requested value.
	 */
	std::string GetName() override;

	/**
	 * @brief Sets scene viewport wants game mouse.
	 * @param enable Boolean flag controlling whether the feature is enabled.
	 */
	void SetSceneViewportWantsGameMouse(bool enable);

	/**
	 * @brief Sets window.
	 * @param window Parameter for window.
	 */
	void SetWindow(GLFWwindow* window);

	/**
	 * @brief Returns whether key pressed.
	 * @param key Parameter for key.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsKeyPressed(int key) const;

	/**
	 * @brief Returns whether key just pressed.
	 * @param key Parameter for key.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsKeyJustPressed(int key);

	/**
	 * @brief Returns whether mouse button pressed.
	 * @param button Parameter for button.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsMouseButtonPressed(int button) const;

	/**
	 * @brief Returns whether mouse button just pressed.
	 * @param button Parameter for button.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsMouseButtonJustPressed(int button);

	/**
	 * @brief Returns whether mouse button just released.
	 * @param button Parameter for button.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsMouseButtonJustReleased(int button) const;

	/**
	 * @brief Returns mouse position.
	 * @return Requested value.
	 */
	glm::dvec2 GetMousePosition() const;

	/**
	 * @brief Performs screen to world.
	 * @param mouseX Parameter for mouse x.
	 * @param mouseY Parameter for mouse y.
	 * @return Result produced by this operation.
	 */
	glm::vec3 ScreenToWorld(float mouseX, float mouseY) const;

	/**
	 * @brief Clears state.
	 */
	void ClearState();

	/**
	 * @brief Performs consume next mouse press.
	 * @param button Parameter for button.
	 */
	void ConsumeNextMousePress(int button);

	/**
	 * @brief Clears mouse consume.
	 * @param button Parameter for button.
	 */
	void ClearMouseConsume(int button);

	/**
	 * @brief Performs consume next key press.
	 * @param key Parameter for key.
	 */
	void ConsumeNextKeyPress(int key);

	/**
	 * @brief Clears key consume.
	 * @param key Parameter for key.
	 */
	void ClearKeyConsume(int key);

	/**
	 * @brief Performs capture snapshot.
	 * @param out Output value for out.
	 */
	void CaptureSnapshot(Snapshot& out) const;

	/**
	 * @brief Applies snapshot.
	 * @param snapshot Parameter for snapshot.
	 */
	void ApplySnapshot(const Snapshot& snapshot);

	/**
	 * @brief Sets replay override.
	 * @param enable Boolean flag controlling whether the feature is enabled.
	 */
	void SetReplayOverride(bool enable);

	/**
	 * @brief Returns whether replay override.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsReplayOverride() const;

private:

	/**
	 * @brief Updates internal.
	 * @param window Parameter for window.
	 */
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

	// Sets whose next just-pressed edge will be consumed
	std::unordered_set<int> mConsumeNextMousePress;
	std::unordered_set<int> mConsumeNextKeyPress;

	bool replayOverride_ = false;
};
