#pragma once

#include "Renderer.h"
#include "ResourceManager.h"
#include "GameObject.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

class GraphicsEngine {
public:
    GraphicsEngine();

    void Initialize();
    void BeginFrame();
    void Render();
    void Shutdown();

    // GameObject management
    GameObject* CreateGameObject(const std::string& meshName, const std::string& shaderName);
    void RemoveGameObject(GameObject* obj);

    // Background management
    void SetBackground(const std::string& texturePath);
    void ClearBackground();

private:
    Renderer renderer;
    ResourceManager& resourceManager;

	// Game Object rendering
    std::vector<std::unique_ptr<GameObject>> gameObjects;
    // Background rendering
    std::unique_ptr<GameObject> backgroundObject;

    glm::mat4 projection;
    glm::mat4 view;

    void LoadDefaultResources();
};
