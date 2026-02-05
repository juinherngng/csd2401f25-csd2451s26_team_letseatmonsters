/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerOrderUILogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung

 DESCRIPTION:       Declares the CustomerOrderUILogic component, responsible for displaying
                    and updating customer order UI elements such as the order bubble,
                    patience bar, and payment result feedback attached to a customer.

         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "GameObjectLogic.hpp"
#include "FoodTypes.hpp"
#include <glm/glm.hpp>
#include <string>

class Scene;
class InputManager;

class CustomerOrderUILogic : public GameObjectLogic
{
public:
    using GameObjectLogic::GameObjectLogic;

    void Start(Scene& scene) override;
    void Update(float dt, Scene& scene, InputManager& input) override;
    void OnDestroy(Scene& scene) override;

    std::string GetName() const override { return "CustomerOrderUILogic"; }

private:
    // --- spawned UI object IDs (attached to this customer) ---
    int bubbleBG_ID_ = -1; // thought bubble background sprite
    int bubbleDish_ID_ = -1; // dish icon sprite on top of bubble
    int barBG_ID_ = -1; // patience background bar (red)
    int barFill_ID_ = -1; // patience fill bar (shrinks)

    // --- config (tune these values) ---
    std::string uiLayerBG_ = "50";
    std::string uiLayerTop_ = "51";

    glm::vec2 bubbleOffset_ = { -55.f, -95.f }; // above head (y negative = up)
    glm::vec2 dishOffset_ = { -54.f, -100.f }; // relative to customer (sits in bubble)
    glm::vec2 barOffset_ = { 0.f,  55.f };  // below customer

    glm::vec2 bubbleSize_ = { 120.f, 90.f };
    glm::vec2 dishIconSize_ = { 70.f, 70.f };   // dish wants bigger
    glm::vec2 coinIconSize_ = { 40.f, 40.f };   // coin wants smaller

    glm::vec2 barBGSize_ = { 120.f, 14.f };
    glm::vec2 barFillSize_ = { 112.f, 10.f };  // slightly inset from BG

    // Track what icon texture is currently on the bubble
    std::string lastIconPath_;

    // Your coin icon path
    const char* coinIconPath_ = "../assets/Coin.png";

    // --- asset paths ---
    const char* bubbleBGPath_ = "../assets/Customer_Order.png";
    const char* patienceBGPath_ = "../assets/Customer_Timer_Red.png";   // red
    const char* patienceFillPath_ = "../assets/Customer_Timer_Green.png"; // green

private:
    // helpers
    void EnsureBubble(Scene& scene, DishType dish);
    void EnsurePatienceBar(Scene& scene);
    void EnsureBubbleIcon(Scene& scene, const char* iconPath);

    void DestroyBubble(Scene& scene);
    void DestroyPatienceBar(Scene& scene);

    //void UpdateDishIconTexture(Scene& scene, DishType dish);
    void UpdateIconTexture(Scene& scene, const char* iconPath);
    void FollowCustomer(Scene& scene);

    void UpdatePatienceFill(Scene& scene, float ratio01);

    const char* DishToIconPath(DishType t) const;

    void SpawnPaymentVFX(Scene& scene, const char* path);
    void UpdatePaymentVFX(Scene& scene, float dt);
    void DestroyPaymentVFX(Scene& scene);
    glm::vec2 GetIconSizeForPath(const char* iconPath) const;

    // --- payment result VFX ---
    int   payVFX_ID_ = -1;
    float payVFXTimer_ = 0.0f;
    float payVFXDuration_ = 0.8f;          // how long the face stays
    glm::vec2 payVFXOffset_ = { -10.f, -140.f }; // above head
    glm::vec2 payVFXSize_ = { 64.f,  64.f };
    float payVFXRiseSpeed_ = 25.f;        // float upward speed (pixels/sec)

    const char* happyFacePath_ = "../assets/HappyFace.png";
    const char* sadFacePath_ = "../assets/SadFace.png";

    // Track previous BehaviourState as int (avoid including SimpleNpcLogic in header)
    int prevBehaviourState_ = -1;

};
