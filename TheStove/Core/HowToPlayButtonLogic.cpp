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

#include <iostream> // <-- for debug logs

#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"

#ifndef _DEBUG
namespace {
    static std::string MakeHoverPath(const std::string& path) {
        if (path.empty()) return path;

        const size_t dot = path.find_last_of('.');
        const std::string ext = (dot != std::string::npos) ? path.substr(dot) : std::string();
        const std::string base = (dot != std::string::npos) ? path.substr(0, dot) : path;

        // If already ends with "_h", keep it
        if (base.size() >= 2 && base.substr(base.size() - 2) == "_h") {
            return dot != std::string::npos ? path : (base + ext);
        }

        // If ends with "_s", replace with "_h"
        if (base.size() >= 2 && base.substr(base.size() - 2) == "_s") {
            return base.substr(0, base.size() - 2) + "_h" + ext;
        }

        // Fallback: append "_h" before extension
        return base + "_h" + ext;
    }

    static void TrySetTexture(GameObject* owner, const std::string& texPath) {
        if (!owner || texPath.empty()) return;
        // Match EntityManager naming convention for static sprite textures
        std::string cacheName = "staticsprite_" + texPath;
        if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
            owner->SetTexture(tex);
        }
    }
}
#endif // _DEBUG

void HowToPlayButtonLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
#ifdef _DEBUG
    // In Debug build this script is currently inert.
    std::cout << "[HowToPlayButtonLogic] Update called in _DEBUG build, doing nothing. ownerID="
        << GetOwnerID() << "\n";
    (void)scene;
    (void)input;
#else
    GameObject* owner = GetOwner(scene);
    if (!owner) {
        std::cout << "[HowToPlayButtonLogic] owner is NULL, ownerID=" << GetOwnerID() << "\n";
        return;
    }

    const bool overlayActive = scene.IsHowToPlayOverlayActive();

    std::cout << "[HowToPlayButtonLogic] Update ownerID=" << GetOwnerID()
        << " overlayActive=" << overlayActive
        << " overlayId_=" << overlayId_
        << " initialized_=" << (initialized_ ? "true" : "false")
        << "\n";

    // If overlay is active but THIS instance did not spawn it,
    // ignore input – let the owner instance handle closing.
    if (overlayActive && overlayId_ < 0) {
        std::cout << "  [HowToPlayButtonLogic] overlayActive && overlayId_ < 0, ignoring input.\n";
        return;
    }

    // --- CASE 1: overlay already active -> treat any click or Esc as "close overlay" ---
    if (overlayActive) {
        const bool clickClose = input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);
        const bool escClose = input.IsKeyJustPressed(GLFW_KEY_ESCAPE);

        std::cout << "  [HowToPlayButtonLogic] overlay active, clickClose="
            << clickClose << " escClose=" << escClose << "\n";

        if (clickClose || escClose) {
            if (overlayId_ >= 0) {
                std::cout << "  [HowToPlayButtonLogic] Despawning overlay id=" << overlayId_ << "\n";
                scene.DespawnByID(overlayId_);
                overlayId_ = -1;
            }

            scene.SetHowToPlayOverlayActive(false);
            std::cout << "  [HowToPlayButtonLogic] SetHowToPlayOverlayActive(false)\n";

            // Restore button texts on main menu (no-op in gameplay if no menu buttons)
            // scene.CreateMenuButtonTexts();
            std::cout << "  [HowToPlayButtonLogic] Called CreateMenuButtonTexts()\n";

            if (clickClose) {
                input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
                std::cout << "  [HowToPlayButtonLogic] Consumed mouse click to close overlay\n";
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

        std::cout << "  [HowToPlayButtonLogic] Lazy init: normalTexturePath_='"
            << normalTexturePath_ << "' hoverTexturePath_='"
            << hoverTexturePath_ << "'\n";
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

    std::cout << "  [HowToPlayButtonLogic] mouseWorld=("
        << mouseWorld.x << ", " << mouseWorld.y
        << ") insideScene=" << (insideScene ? "true" : "false") << "\n";

    // AABB hit-test using button position + size
    const glm::vec3 pos = owner->GetPositionGLM();
    const glm::vec3 sz = owner->GetScaleGLM();
    const float halfW = sz.x * 0.5f;
    const float halfH = sz.y * 0.5f;

    const bool over =
        mouseWorld.x >= (pos.x - halfW) && mouseWorld.x <= (pos.x + halfW) &&
        mouseWorld.y >= (pos.y - halfH) && mouseWorld.y <= (pos.y + halfH);

    std::cout << "  [HowToPlayButtonLogic] button pos=(" << pos.x << ", " << pos.y
        << ") size=(" << sz.x << ", " << sz.y << ") over=" << (over ? "true" : "false") << "\n";

    // Hover visual swap
    if (over && !hovered_) {
        hovered_ = true;
        std::cout << "  [HowToPlayButtonLogic] Hover ENTER, switching to hover texture '"
            << hoverTexturePath_ << "'\n";
        TrySetTexture(owner, hoverTexturePath_);
    }
    else if (!over && hovered_) {
        hovered_ = false;
        std::cout << "  [HowToPlayButtonLogic] Hover LEAVE, switching back to normal texture '"
            << normalTexturePath_ << "'\n";
        TrySetTexture(owner, normalTexturePath_);
    }

    // Click → show overlay
    if (!over || !input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        return;
    }

    std::cout << "  [HowToPlayButtonLogic] CLICK on button, spawning HowToPlay overlay\n";

    input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);

    // Spawn full-screen HowToPlay overlay on top UI layer
    const std::string uiLayer = "9999999";
    const float w = static_cast<float>(GraphicsEngine::kRefW);
    const float h = static_cast<float>(GraphicsEngine::kRefH);

    GameObject* img = scene.SpawnStaticSprite(
        "../assets/HowToPlay.png",             // change path if needed
        { w * 0.5f, h * 0.5f, 0.0f },          // center
        { w, h },                              // full screen
        uiLayer);

    if (!img) {
        std::cout << "  [HowToPlayButtonLogic] ERROR: SpawnStaticSprite returned nullptr\n";
        return; // failed to spawn (wrong path etc.)
    }

    overlayId_ = img->GetID();
    img->SetMovableByPhysics(false);

    std::cout << "  [HowToPlayButtonLogic] Overlay spawned with id=" << overlayId_ << "\n";

    // Mark overlay active in the scene and hide menu button texts
    scene.SetHowToPlayOverlayActive(true);
    //scene.ClearMenuButtonTexts();
    std::cout << "  [HowToPlayButtonLogic] SetHowToPlayOverlayActive(true) and ClearMenuButtonTexts()\n";
#endif // _DEBUG
}
