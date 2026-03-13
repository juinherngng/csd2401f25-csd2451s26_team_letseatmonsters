/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerOrderUILogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Declares the CustomerOrderUILogic component, responsible for displaying
					and updating customer order UI elements such as the order bubble,
					patience bar, and payment result feedback attached to a customer.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "FoodTypes.hpp"
#include "GameObjectLogic.hpp"

#include <glm/glm.hpp>
#include <string>

class Scene;
class InputManager;

class CustomerOrderUILogic : public GameObjectLogic {
public:
	using GameObjectLogic::GameObjectLogic;

	void Start(Scene& scene) override;
	void Update(float dt, Scene& scene, InputManager& input) override;
	void OnDestroy(Scene& scene) override;
	bool HitTestBubble(Scene& scene, const glm::vec2& worldPos) const;

	std::string GetName() const override {
		return "CustomerOrderUILogic";
	}

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
	glm::vec2 coinIconSize_ = { 60.f, 60.f };   // coin wants smaller

	glm::vec2 barBGSize_ = { 120.f, 14.f };
	glm::vec2 barFillSize_ = { 112.f, 10.f };  // slightly inset from BG

	// Track what icon texture is currently on the bubble
	std::string lastIconPath_;

	// Your coin icon path
	const char* coinIconPath_ = "../assets/Reaction_Money.png";

	// --- asset paths ---
	const char* bubbleBGPath_ = "../assets/Customer_Order.png";
	const char* patienceBGPath_ = "../assets/Customer_Timer_Red.png";	  // red
	const char* patienceFillPath_ = "../assets/Customer_Timer_Green.png"; // green

	// helpers
	// Ensures patience bar background/fill widgets are created
	void EnsurePatienceBar(Scene& scene);

	// Ensures the icon sprite exists and is set to the provided texture path
	void EnsureBubbleIcon(Scene& scene, const char* iconPath);

	// Destroys all bubble-related UI objects if they exist
	void DestroyBubble(Scene& scene);

	// Destroys patience bar UI objects if they exist
	void DestroyPatienceBar(Scene& scene);

	// void UpdateDishIconTexture(Scene& scene, DishType dish);
	// Replaces bubble icon texture and updates its rendered size
	void UpdateIconTexture(Scene& scene, const char* iconPath);
	// Keeps spawned UI anchored to the customer's current position
	void FollowCustomer(Scene& scene);

	// Rescales patience fill width from a normalized [0,1] ratio
	void UpdatePatienceFill(Scene& scene, float ratio01);

	const char* happyFacePath_ = "../assets/Reaction_Happy_Face.png";
	const char* sadFacePath_ = "../assets/Reaction_Angry_Face.png";

	// Spawns temporary payment reaction VFX (happy/sad face)
	void SpawnPaymentVFX(Scene& scene, const char* path);

	// Updates payment VFX position/lifetime while active
	void UpdatePaymentVFX(Scene& scene, float dt);

	// Destroys payment VFX object and resets related state
	void DestroyPaymentVFX(Scene& scene);

	// Returns icon dimensions tailored to the requested icon path
	glm::vec2 GetIconSizeForPath(const char* iconPath) const;

	// Maps dish enum to the icon texture shown in the thought bubble
	const char* DishToIconPath(DishType dish) const;

	// --- payment result VFX ---
	int   payVFX_ID_ = -1;
	float payVFXTimer_ = 0.0f;
	float payVFXDuration_ = 0.8f;				 // how long the face stays
	glm::vec2 payVFXOffset_ = { -10.f, -140.f }; // above head
	glm::vec2 payVFXSize_ = { 64.f,  64.f };
	float payVFXRiseSpeed_ = 25.f;				 // float upward speed (pixels/sec)

	// Track previous BehaviourState as int (avoid including SimpleNpcLogic in header)
	int prevBehaviourState_ = -1;

};
