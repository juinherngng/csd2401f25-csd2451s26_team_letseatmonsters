#pragma once

#include <unordered_map>
#include <GLFW/glfw3.h>

class InputManager {
public:
    void Update(GLFWwindow* window);             // Call once per frame to poll keys
    bool IsKeyPressed(int key) const;            // Returns true if key currently pressed
    bool IsKeyJustPressed(int key) const;        // Returns true if key pressed this frame (edge detection)

private:
    std::unordered_map<int, bool> mCurrentKeyStates;
    std::unordered_map<int, bool> mPreviousKeyStates;
};

