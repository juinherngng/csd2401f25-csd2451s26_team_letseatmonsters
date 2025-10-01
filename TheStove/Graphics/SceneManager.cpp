#include "SceneManager.h"
#include <iostream>
#include <glm/ext/matrix_clip_space.hpp>

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

GameObject* Scene::SpawnStaticSprite(const std::string& texturePath, const glm::vec3& position, const glm::vec2& size) {
    // Load sprite texture if not already loaded
    std::string textureName = "sprite_" + texturePath; // Simple naming scheme
    Texture* spriteTexture = ResourceManager::Instance().LoadTexture(textureName, texturePath);

    if (!spriteTexture) {
        std::cerr << "Failed to load sprite texture: " << texturePath << std::endl;
        return nullptr;
    }

    // Create sprite game object with unique ID
    GameObject* obj = graphicsEngine.CreateGameObject("sprite", "staticsprite");
    if (obj) {
        obj->SetID(nextID++);
        obj->SetPosition(position);
        obj->SetScale(glm::vec3(size.x, size.y, 1.0f));
        obj->SetTexture(spriteTexture);
        sceneObjects.push_back(std::unique_ptr<GameObject>(obj));
    }

    return obj;
}

GameObject* Scene::SpawnAnimatedSprite(const std::string& texturePath, const glm::vec3& position, const glm::vec2& size, const std::vector<glm::vec4>& frames, float frameDuration, bool loop)
{
    std::string textureName = "sprite_" + texturePath;
    Texture* spriteTexture = ResourceManager::Instance().LoadTexture(textureName, texturePath);

    if (!spriteTexture) {
        std::cerr << "Failed to load animated sprite texture: " << texturePath << std::endl;
        return nullptr;
    }

    GameObject* obj = graphicsEngine.CreateGameObject("sprite", "animatedsprite");
    if (obj) {
        obj->SetID(nextID++);
        obj->SetPosition(position);
        obj->SetScale(glm::vec3(size.x, size.y, 1.0f));
        obj->SetTexture(spriteTexture);
        sceneObjects.push_back(std::unique_ptr<GameObject>(obj));

        Animator2D animator;
        animator.SetFrames(frames, frameDuration, loop);
        animator.Play();
        animators[obj->GetID()] = animator;  // Add animator for this animated sprite
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

// Utility function to generate UV frames for a sprite sheet
std::vector<glm::vec4> GenerateFrames(int startFrame, int frameCount, int totalCols, float frameWidth, float frameHeight) {
    std::vector<glm::vec4> frames;
    for (int i = 0; i < frameCount; ++i) {
        int col = startFrame + i;
        float offsetX = col * frameWidth;
        float offsetY = 1.0f - frameHeight; // single row, so just - frameHeight for Y offset
        frames.emplace_back(offsetX, offsetY, frameWidth, frameHeight);
    }
    return frames;
}


void Scene::LoadTest() {
    // Set background image
    SetSceneBackground("../assets/Background.png");

    GameObject* player = SpawnStaticSprite("../assets/mc_front.png",
        glm::vec3(400, 400, 0), //Position
        glm::vec2(128, 128));   //Scale

    if (player) {
        spriteID = player->GetID();
        std::cout << "Spawned player sprite with ID: " << spriteID << std::endl;

        spriteScales[spriteID] = glm::vec3(128, 128, 1.0f); // initial scale to match sprite size
        spriteRotations[spriteID] = 0.0f;
        spritePositions[spriteID] = glm::vec3(400, 400, 0); // initial position

    }
    else {
        spriteID = -1; //invalid
        std::cerr << "Failed to spawn sprite" << std::endl;
    }

    // Compute UV frames for a 4x4 grid sprite sheet
    std::vector<glm::vec4> frames;
    const int cols = 24;
    const int rows = 1;
    float frameWidth = 1.0f / (float)cols;   
    float frameHeight = 1.0f / (float)rows;  

    std::vector<glm::vec4> idleFrames = GenerateFrames(0, 4, cols, frameWidth, frameHeight);
    std::vector<glm::vec4> walkFrames = GenerateFrames(4, 6, cols, frameWidth, frameHeight);
    std::vector<glm::vec4> attackFrames = GenerateFrames(6, 7, cols, frameWidth, frameHeight);


    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            float offsetX = col * frameWidth;
            float offsetY = 1.0f - (row + 1) * frameHeight; // Flip Y
            frames.emplace_back(offsetX, offsetY, frameWidth, frameHeight);
        }
    }

    GameObject* dinoRed = SpawnAnimatedSprite("../assets/dino_red.png",
        glm::vec3(700, 650, 0),
        glm::vec2(128, 128),
        frames, 0.25f, true);

    if (dinoRed) {
        dinoID = dinoRed->GetID(); 
        std::cout << "Spawned dinoRed sprite with ID: " << dinoID << std::endl;

        // Store all animations
        Animator2D idleAnimator;
        idleAnimator.SetFrames(idleFrames, 0.25f, true);
        idleAnimator.Play();

        Animator2D walkAnimator;
        walkAnimator.SetFrames(walkFrames, 0.15f, true);
        walkAnimator.Play();

        Animator2D attackAnimator;
        attackAnimator.SetFrames(attackFrames, 0.15f, true);
        attackAnimator.Play();

        objectAnimations[dinoID]["IDLE"] = idleAnimator;
        objectAnimations[dinoID]["WALK"] = walkAnimator;
        objectAnimations[dinoID]["ATTACK"] = attackAnimator;

        currentAnimation[dinoID] = "IDLE";
    }

    GameObject* dinoBblue = SpawnAnimatedSprite("../assets/dino_blue.png",
        glm::vec3(700, 500, 0),
        glm::vec2(128, 128),
        frames, 0.25f, true);

    GameObject* dinoGreen = SpawnAnimatedSprite("../assets/dino_green.png",
        glm::vec3(700, 350, 0),
        glm::vec2(128, 128),
        frames, 0.25f, true);

    GameObject* dinoYellow = SpawnAnimatedSprite("../assets/dino_yellow.png",
        glm::vec3(700, 200, 0),
        glm::vec2(128, 128),
        frames, 0.25f, true);

    //SpawnTriangle(glm::vec3(600, 400, 0), glm::vec3(100.0f), 180.0f);
    //SpawnTriangle(glm::vec3(300, 200, 0), glm::vec3(100.0f), 180.0f);
}

void Scene::SetAnimation(int objID, const std::string& newAnim) {
    if (objectAnimations.count(objID) &&
        objectAnimations[objID].count(newAnim) &&
        currentAnimation[objID] != newAnim)
    {
        currentAnimation[objID] = newAnim;
        objectAnimations[objID][newAnim].Play();  // restart animation
    }
}


void Scene::Update(float deltaTime, GLFWwindow* window) {

    inputManager.Update(window);

    for (auto& [id, animMap] : objectAnimations) {
        std::string& animName = currentAnimation[id];
        Animator2D& animator = animMap[animName];
        animator.Update(deltaTime);

        GameObject* obj = GetGameObjectByID(id);
        if (!obj) continue;
        Shader* shader = obj->GetShader();
        if (!shader) continue;

        glm::vec4 uvFrame = animator.GetCurrentFrameUV();
        shader->Use();
        shader->SetUVOffset(glm::vec2(uvFrame.x, uvFrame.y));
        shader->SetUVScale(glm::vec2(uvFrame.z, uvFrame.w));
    }




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
        SetAnimation(dinoID, "WALK");
        sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_back.png"));

        position.y -= moveSpeed;  // Move up
    }
    if (inputManager.IsKeyPressed(GLFW_KEY_S)) {
        SetAnimation(dinoID, "IDLE");
        sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_front.png"));
        position.y += moveSpeed;  // Move down
    }
    if (inputManager.IsKeyPressed(GLFW_KEY_A)) {
        SetAnimation(dinoID, "ATTACK");
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



