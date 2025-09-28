#pragma once

#include "GraphicsEngine.h"
#include <string>
#include <vector>

class Scene {
public:
    Scene(GraphicsEngine& engine);

    void LoadScene(const std::string& sceneName);
    void Update(float deltaTime);

    // Scene-specific object creation
    GameObject* SpawnTriangle(const glm::vec3& position, const glm::vec3& scale, float rotation = 0.0f);
    GameObject* SpawnSprite(const std::string& texturePath, const glm::vec3& position,
                            const glm::vec2& size = glm::vec2(100.0f, 100.0f));

    // Background management
    void SetSceneBackground(const std::string& texturePath);

private:
    GraphicsEngine& graphicsEngine;
    std::vector<GameObject*> sceneObjects;

    void LoadTest();
};
