// PlayerLogic.cpp
#include "PlayerLogic.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Core/InputManager.hpp"
#include "../Core/InputControls.hpp"
#include "../Core/DebugUI.hpp"        // for DebuggerApp
#include <iostream>

void PlayerLogic::Update(float dt, Scene& scene, InputManager& input) {
    GameObject* player = GetOwner(scene);
    if (!player) return;

    //glm::vec3 pos = player->GetPositionGLM();

    //// WASD movement
    //float speed = 200.0f;
    //if (input.IsKeyPressed(GLFW_KEY_A)) pos.x -= speed * dt;
    //if (input.IsKeyPressed(GLFW_KEY_D)) pos.x += speed * dt;
    //if (input.IsKeyPressed(GLFW_KEY_W)) pos.y -= speed * dt;
    //if (input.IsKeyPressed(GLFW_KEY_S)) pos.y += speed * dt;

    //// Simple left-click teleport for demo
    //if (input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
    //    glm::vec2 worldPos;
    //    if (GraphicsEngine::Instance().GetMouseWorldInScene(worldPos)) {
    //        pos.x = worldPos.x;
    //        pos.y = worldPos.y;
    //    }
    //}

    //player->SetPosition(pos);

    if (input.IsKeyJustPressed(GLFW_KEY_SPACE)) {
        std::cout << "press space" ;
    }
}
