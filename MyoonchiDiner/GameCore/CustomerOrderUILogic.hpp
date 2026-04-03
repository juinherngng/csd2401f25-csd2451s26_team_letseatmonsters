/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerOrderUILogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Declares the CustomerOrderUILogic component, responsible for displaying
					and updating customer order UI elements such as the order bubble,
					patience bar, and payment result feedback attached to a customer.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <glm/glm.hpp>
#include <string>

#include "EngineCore/GameObjectLogic.hpp"
#include "GameCore/FoodTypes.hpp"

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
	struct WarningGlowSet {
		std::array<int, 9> ids{ -1, -1, -1, -1, -1, -1, -1, -1, -1 };
		int sourceID = -1;
	};

	// --- spawned UI object IDs (attached to this customer) ---
	int bubbleBG_ID_ = -1; // thought bubble background sprite
	int bubbleDish_ID_ = -1; // dish icon sprite on top of bubble
	int barBG_ID_ = -1; // patience background bar (red)
	int barFill_ID_ = -1; // patience fill bar (shrinks)
	WarningGlowSet lowPatienceGlow_{};
	float lowPatienceGlowTimer_ = 0.0f;
	float lowPatienceGlowAlpha_ = 0.0f;
	float lowPatienceBarScaleMul_ = 1.0f;
	float lowPatienceBarAlpha_ = 1.0f;
	glm::vec2 lowPatienceBarOffset_ = { 0.0f, 0.0f };

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
	const char* coinIconPath_ = "../assets/UI/Reaction_Money.png";

	// --- asset paths ---
	const char* bubbleBGPath_ = "../assets/UI/Customer_Order.png";
	const char* patienceBGPath_ = "../assets/UI/Customer_Timer_Red.png";	  // red
	const char* patienceFillPath_ = "../assets/UI/Customer_Timer_Green.png"; // green

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

	// Ensures the looping eating VFX exists while the customer is eating
	void EnsureEatingVFX(Scene& scene);

	// Keeps the eating VFX anchored to the customer each frame
	void UpdateEatingVFX(Scene& scene);

	// Picks the eating VFX offset that matches the customer's current eating pose
	glm::vec2 GetEatingVfxOffset(Scene& scene) const;

	// Destroys the eating VFX if it exists
	void DestroyEatingVFX(Scene& scene);

	// Drives the low-patience warning glow while the bar is visible
	void UpdateLowPatienceWarning(Scene& scene, float dt, float ratio01, bool showBar);

	// Ensures the low-patience outline overlay exists around the bar background
	void EnsureLowPatienceGlow(Scene& scene);

	// Keeps the low-patience glow aligned with the bar and updates pulse alpha
	void SyncLowPatienceGlow(Scene& scene, float alpha);

	// Destroys the low-patience glow overlay if it exists
	void DestroyLowPatienceGlow(Scene& scene);

	const char* happyFacePath_ = "../assets/UI/Reaction_Happy_Face.png";
	const char* sadFacePath_ = "../assets/UI/Reaction_Angry_Face.png";

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

	void TriggerOrderCompleteSuccess(Scene& scene);

	// --- payment result VFX ---
	int   payVFX_ID_ = -1;
	float payVFXTimer_ = 0.0f;
	float payVFXDuration_ = 0.8f;				 // how long the face stays
	glm::vec2 payVFXOffset_ = { -10.f, -140.f }; // above head
	glm::vec2 payVFXSize_ = { 64.f,  64.f };
	float payVFXRiseSpeed_ = 25.f;				 // float upward speed (pixels/sec)

	// Track previous BehaviourState as int (avoid including SimpleNpcLogic in header)
	int prevBehaviourState_ = -1;

	enum class PayVfxMode {
		FloatUp,
		FlyToOrder
	};

	PayVfxMode payVFXMode_ = PayVfxMode::FloatUp;
	glm::vec2 payVFXStartPos_ = { 0.f, 0.f };
	glm::vec2 payVFXTargetPos_ = { 0.f, 0.f };
	float payVFXFlyDuration_ = 0.24f;
	bool payVFXQueueStamp_ = false;

	// --- eating VFX ---
	int eatingVFX_ID_ = -1;
	glm::vec2 eatingVFXOffsetRight_ = { 18.f, 8.f };
	glm::vec2 eatingVFXOffsetLeft_ = { -18.f, 8.f };
	glm::vec2 eatingVFXFallbackOffset_ = { 18.f, 8.f };
	glm::vec2 eatingVFXSize_ = { 96.f, 96.f };

};
