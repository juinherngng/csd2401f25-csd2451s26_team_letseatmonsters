#pragma once

#include <unordered_map>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class InputManager {
public:
	void Update(GLFWwindow* window);             // Call once per frame to poll keys
	bool IsKeyPressed(int key) const;            // Returns true if key currently pressed
	bool IsKeyJustPressed(int key) const;        // Returns true if key pressed this frame (edge detection)

	bool IsMouseButtonPressed(int button) const;	 // Check if a mouse button is currently pressed
	bool IsMouseButtonJustPressed(int button) const; // Check if a mouse button was pressed this frame 

	// Get current mouse cursor position
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
