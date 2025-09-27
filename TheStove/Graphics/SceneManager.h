#pragma once

#include "GraphicsEngine.h"
#include "../Core/InputManager.h"
#include <string>
#include <vector>

class Scene {
public:
    Scene(GraphicsEngine& engine);

    void LoadScene(const std::string& sceneName);
    void Update(float deltaTime, GLFWwindow* window);

    // Scene-specific object creation
    GameObject* SpawnTriangle(const glm::vec3& position, const glm::vec3& scale, float rotation = 0.0f);
    GameObject* SpawnSprite(const std::string& texturePath, const glm::vec3& position,
                            const glm::vec2& size = glm::vec2(100.0f, 100.0f));

    // Background management
    void SetSceneBackground(const std::string& texturePath);

	// Object Lookup
    GameObject* GetGameObjectByID(int targetID);

private:
    GraphicsEngine& graphicsEngine;
    InputManager inputManager;

    std::vector<std::unique_ptr<GameObject>> sceneObjects;
	int nextID = 1; // ID counter for GameObjects
    int spriteID = -1; // default invalid ID

    std::unordered_map<int, glm::vec3> spriteScales;
    std::unordered_map<int, glm::vec3> spritePositions;
    std::unordered_map<int, float> spriteRotations;

    void LoadTest();
};
