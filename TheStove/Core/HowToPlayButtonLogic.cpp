/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         HowToPlayButtonLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            <your name>

 DESCRIPTION:       Implements hover + click behaviour for the main-menu
                    "How To Play" button. Clicking shows a full-screen
                    ../assets/HowToPlay.png overlay, clicking again (or Esc)
                    hides it and restores the menu.

         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "HowToPlayButtonLogic.hpp"

#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"

namespace {
    std::string MakeHoverPath(const std::string& path) {
        if (path.empty()) return path;
        const size_t dot = path.find_last_of('.');
        if (dot == std::string::npos) return path + "_hover";
        return path.substr(0, dot) + "_hover" + path.substr(dot);
    }

    void TrySetTexture(GameObject* owner, const std::string& texPath) {
        if (!owner || texPath.empty()) return;

        std::string cacheName = "staticsprite_" + texPath;
        if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
            owner->SetTexture(tex);
        }
    }
}

void HowToPlayButtonLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
#ifdef _DEBUG
    (void)scene; (void)input;
    return; // inert in debug build (matches MenuButtonLogic / PauseButtonLogic pattern)
#endif

    const bool overlayActive = scene.IsHowToPlayOverlayActive();

    // --- CASE 1: overlay already active -> treat any click or Esc as "close overlay" ---
    if (overlayActive) {
        const bool clickClose = input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);
        const bool escClose = input.IsKeyJustPressed(GLFW_KEY_ESCAPE);

        if (clickClose || escClose) {
            if (overlayId_ >= 0) {
                scene.DespawnByID(overlayId_);
                overlayId_ = -1;
            }

            scene.SetHowToPlayOverlayActive(false);

            // Restore button texts on main menu
            scene.CreateMenuButtonTexts();

            if (clickClose) {
                input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
            }
        }

        // While overlay is active, we do NOT want hover or button-click behaviour.
        return;
    }

    // --- CASE 2: overlay not active -> regular button hover + click to show overlay ---

    // Lazy init texture paths for button hover effect
    if (!initialized_) {
        normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
        hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
        initialized_ = true;
    }

    // Mouse position in world coords
    glm::vec2 mouseWorld{};
    bool insideScene = GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld);
    if (!insideScene) {
        glm::vec3 w = input.ScreenToWorld(
            static_cast<float>(input.GetMousePosition().x),
            static_cast<float>(input.GetMousePosition().y));
        mouseWorld = glm::vec2(w.x, w.y);
    }

    GameObject* owner = GetOwner(scene);
    if (!owner) return;

    // AABB hit-test using button position + size
    const glm::vec3 pos = owner->GetPositionGLM();
    const glm::vec3 sz = owner->GetScaleGLM();
    const float halfW = sz.x * 0.5f;
    const float halfH = sz.y * 0.5f;

    const bool over =
        mouseWorld.x >= (pos.x - halfW) && mouseWorld.x <= (pos.x + halfW) &&
        mouseWorld.y >= (pos.y - halfH) && mouseWorld.y <= (pos.y + halfH);

    // Hover visual swap
    if (over && !hovered_) {
        hovered_ = true;
        TrySetTexture(owner, hoverTexturePath_);
    }
    else if (!over && hovered_) {
        hovered_ = false;
        TrySetTexture(owner, normalTexturePath_);
    }

    // Click → show overlay
    if (!over || !input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        return;
    }

    input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);

    // Spawn full-screen HowToPlay overlay on top UI layer
    const std::string uiLayer = "999999";
    const float w = static_cast<float>(GraphicsEngine::kRefW);
    const float h = static_cast<float>(GraphicsEngine::kRefH);

    GameObject* img = scene.SpawnStaticSprite(
        "../assets/HowToPlay.png",             // change path if needed
        { w * 0.5f, h * 0.5f, 0.0f },          // center
        { w, h },                              // full screen
        uiLayer);

    if (!img) {
        return; // failed to spawn (wrong path etc.)
    }

    overlayId_ = img->GetID();
    img->SetMovableByPhysics(false);

    // Mark overlay active in the scene and hide menu button texts
    scene.SetHowToPlayOverlayActive(true);
    scene.ClearMenuButtonTexts();
}
