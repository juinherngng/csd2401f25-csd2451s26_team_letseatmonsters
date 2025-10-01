#include "InputManager.h"
#include <iostream>

void InputManager::Update(GLFWwindow* window) {

    // Put keys here; expand as needed
    int keys[] = { GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D };

    for (int key : keys) {
        bool state = glfwGetKey(window, key) == GLFW_PRESS;
        mCurrentKeyStates[key] = state;

        //// Debug print
        //if (state) {
        //    std::cout << "Key " << key << " pressed with window " << window << std::endl;
        //}
        //else {
        //    std::cout << "Key " << key << " released with window " << window << std::endl;
        //}
    }
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


