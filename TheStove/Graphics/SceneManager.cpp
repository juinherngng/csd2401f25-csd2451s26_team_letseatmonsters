#include "SceneManager.h"
#include <iostream>

Scene::Scene(GraphicsEngine& engine) : graphicsEngine(engine) {}

void Scene::LoadScene(const std::string& sceneName) {

    // Test scene for now
    LoadTest();
}

GameObject* Scene::SpawnTriangle(const glm::vec3& position, const glm::vec3& scale, float rotation) {
    GameObject* obj = graphicsEngine.CreateGameObject("triangle", "basic");
    if (obj) {
        obj->SetPosition(position);
        obj->SetScale(scale);
        obj->SetRotation(glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        sceneObjects.push_back(obj);
    }
    return obj;
}

GameObject* Scene::SpawnSprite(const std::string& texturePath, const glm::vec3& position, const glm::vec2& size) {
    // Load sprite texture if not already loaded
    std::string textureName = "sprite_" + texturePath; // Simple naming scheme
    Texture* spriteTexture = ResourceManager::Instance().LoadTexture(textureName, texturePath);

    if (!spriteTexture) {
        std::cerr << "Failed to load sprite texture: " << texturePath << std::endl;
        return nullptr;
    }

    // Create sprite game object
    GameObject* obj = graphicsEngine.CreateGameObject("sprite", "sprite");
    if (obj) {
        obj->SetPosition(position);
        obj->SetScale(glm::vec3(size.x, size.y, 1.0f));
        obj->SetTexture(spriteTexture);
        sceneObjects.push_back(obj);
    }

    return obj;
}

void Scene::SetSceneBackground(const std::string& texturePath) {
    graphicsEngine.SetBackground(texturePath);
}

void Scene::LoadTest() {
    // Set background image
    SetSceneBackground("../assets/Background.png");

    GameObject* player = SpawnSprite("../assets/mc_front.png",
        glm::vec3(400, 400, 0), //Position
        glm::vec2(128, 128));   //Scale

    //SpawnTriangle(glm::vec3(600, 400, 0), glm::vec3(100.0f), 180.0f);
    //SpawnTriangle(glm::vec3(300, 200, 0), glm::vec3(100.0f), 180.0f);
}

