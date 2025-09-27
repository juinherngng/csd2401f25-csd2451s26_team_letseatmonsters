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
        sceneObjects.push_back(std::unique_ptr<GameObject>(obj));
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

    // Create sprite game object with unique ID
    GameObject* obj = graphicsEngine.CreateGameObject("sprite", "sprite");
    if (obj) {
        obj->SetID(nextID++);
        obj->SetPosition(position);
        obj->SetScale(glm::vec3(size.x, size.y, 1.0f));
        obj->SetTexture(spriteTexture);
        sceneObjects.push_back(std::unique_ptr<GameObject>(obj));
    }

    return obj;
}

void Scene::SetSceneBackground(const std::string& texturePath) {
    graphicsEngine.SetBackground(texturePath);
}

GameObject* Scene::GetGameObjectByID(int targetID) {
    for (const auto& obj : sceneObjects) {
        if (obj->GetID() == targetID)  
            return obj.get();
    }
    return nullptr; // Not found
}

void Scene::LoadTest() {
    // Set background image
    SetSceneBackground("../assets/Background.png");

    GameObject* player = SpawnSprite("../assets/mc_front.png",
        glm::vec3(400, 400, 0), //Position
        glm::vec2(128, 128));   //Scale

    if (player) {
        spriteID = player->GetID();
        std::cout << "Spawned sprite with ID: " << spriteID << std::endl;

        spriteScales[spriteID] = glm::vec3(128, 128, 1.0f); // initial scale to match sprite size
        spriteRotations[spriteID] = 0.0f;
		spritePositions[spriteID] = glm::vec3(400, 400, 0); // initial position

    }
    else {
        spriteID = -1; //invalid
        std::cerr << "Failed to spawn sprite" << std::endl;
    }

    //SpawnTriangle(glm::vec3(600, 400, 0), glm::vec3(100.0f), 180.0f);
    //SpawnTriangle(glm::vec3(300, 200, 0), glm::vec3(100.0f), 180.0f);
}

void Scene::Update(float deltaTime, GLFWwindow* window) {

    inputManager.Update(window);

	const float rotationSpeed = 1.0f * deltaTime; // degrees per second
    float moveSpeed = 200.0f * deltaTime;

    if (spriteID < 0) return;

    GameObject* sprite = GetGameObjectByID(spriteID);

    if (!sprite) {
        std::cerr << "Sprite with ID " << spriteID << " not found" << std::endl;
        return;
    }

    glm::vec3& position = spritePositions[spriteID];
    glm::vec3& scale = spriteScales[spriteID];
    float& rotation = spriteRotations[spriteID];

    if (inputManager.IsKeyPressed(GLFW_KEY_UP)) {
        std::cout << "Up key pressed: scale = " << scale.x << "," << scale.y << "," << scale.z << std::endl;
        scale *= 1.01f;

        //Clamp max scale
        scale = glm::min(scale, glm::vec3(500.0f));
    }
    if (inputManager.IsKeyPressed(GLFW_KEY_DOWN)) {
        std::cout << "Down key pressed: scale = " << scale.x << "," << scale.y << "," << scale.z << std::endl;
        scale *= 0.99f;

        //Clamp min scale
        scale = glm::max(scale, glm::vec3(50.0f));
    }
    if (inputManager.IsKeyPressed(GLFW_KEY_RIGHT)) {
        rotation += rotationSpeed;
        if (rotation > 360.0f) rotation -= 360.0f;

        std::cout << "Right key pressed: rotation = " << rotation << std::endl;
    }

    if (inputManager.IsKeyPressed(GLFW_KEY_LEFT)) {
        rotation -= rotationSpeed;
        if (rotation < 0.0f) rotation += 360.0f;

        std::cout << "Left key pressed: rotation = " << rotation << std::endl;
    }

    if (inputManager.IsKeyPressed(GLFW_KEY_W)) {

        sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_back.png"));

        position.y -= moveSpeed;  // Move up
    }
    if (inputManager.IsKeyPressed(GLFW_KEY_S)) {

        sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_front.png"));
        position.y += moveSpeed;  // Move down
    }
    if (inputManager.IsKeyPressed(GLFW_KEY_A)) {

        sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideleft", "../assets/mc_sideleft.png"));
        position.x -= moveSpeed;  // Move left
    }
    if (inputManager.IsKeyPressed(GLFW_KEY_D)) {

        sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideright", "../assets/mc_sideright.png"));
        position.x += moveSpeed;  // Move right
    }

    // Clamping position to stay within screen bounds
    position.x = glm::clamp(position.x, 0.0f, 1200.0f);
    position.y = glm::clamp(position.y, 0.0f, 800.0f);

    sprite->SetScale(scale);
    sprite->SetRotation(rotation, glm::vec3(0, 0, 1));
	sprite->SetPosition(position);
}



