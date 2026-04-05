/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         CustomerOrderUILogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (85%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (15%)

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

	/**
	 * @brief Resets runtime UI state when the logic is first attached.
	 * @param scene Active scene that owns the customer object.
	 */
	void Start(Scene& scene) override;

	/**
	 * @brief Updates customer UI visibility, anchoring, and feedback effects for one frame.
	 * @param dt Delta time for the frame.
	 * @param scene Active scene containing the customer and UI objects.
	 * @param input Input manager forwarded by the logic system.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Cleans up all spawned UI and VFX objects owned by this customer.
	 * @param scene Active scene containing the spawned helper objects.
	 */
	void OnDestroy(Scene& scene) override;

	/**
	 * @brief Tests whether a world-space point overlaps the customer's order bubble.
	 * @param scene Active scene containing the bubble objects.
	 * @param worldPos World-space point to test.
	 * @return True if the point hits the bubble background or icon.
	 */
	bool HitTestBubble(Scene& scene, const glm::vec2& worldPos) const;

	/**
	 * @brief Returns the stable runtime logic name used by the engine.
	 * @return Name string for the logic component.
	 */
	std::string GetName() const override {
		// Keep the logic name stable for registration, debugging, and save data lookups.
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
	std::string uiLayerBG_ = "100";
	std::string uiLayerTop_ = "101";

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

	/**
	 * @brief Ensures the patience-bar background and fill widgets exist.
	 * @param scene Active scene containing the customer.
	 */
	void EnsurePatienceBar(Scene& scene);

	/**
	 * @brief Ensures the order bubble icon exists and uses the requested texture.
	 * @param scene Active scene containing the customer.
	 * @param iconPath Texture path for the icon that should be displayed.
	 */
	void EnsureBubbleIcon(Scene& scene, const char* iconPath);

	/**
	 * @brief Destroys the currently spawned order-bubble objects, if any.
	 * @param scene Active scene containing the bubble sprites.
	 */
	void DestroyBubble(Scene& scene);

	/**
	 * @brief Destroys the patience-bar widgets, if any are active.
	 * @param scene Active scene containing the patience-bar sprites.
	 */
	void DestroyPatienceBar(Scene& scene);

	/**
	 * @brief Replaces the current bubble icon texture and refreshes its rendered size.
	 * @param scene Active scene containing the icon sprite.
	 * @param iconPath Texture path for the replacement icon.
	 */
	void UpdateIconTexture(Scene& scene, const char* iconPath);

	/**
	 * @brief Repositions all spawned UI elements so they follow the customer.
	 * @param scene Active scene containing the customer and UI objects.
	 */
	void FollowCustomer(Scene& scene);

	/**
	 * @brief Rescales the patience fill bar from a normalized ratio.
	 * @param scene Active scene containing the bar sprites.
	 * @param ratio01 Normalized patience value in the range `[0, 1]`.
	 */
	void UpdatePatienceFill(Scene& scene, float ratio01);

	/**
	 * @brief Ensures the looping eating VFX is spawned while the customer is eating.
	 * @param scene Active scene containing the customer and VFX objects.
	 */
	void EnsureEatingVFX(Scene& scene);

	/**
	 * @brief Repositions and refreshes the eating VFX while it is active.
	 * @param scene Active scene containing the customer and VFX objects.
	 */
	void UpdateEatingVFX(Scene& scene);

	/**
	 * @brief Returns the authored eating-VFX offset that matches the current pose.
	 * @param scene Active scene used to query the customer's animation state.
	 * @return Offset to apply to the eating VFX.
	 */
	glm::vec2 GetEatingVfxOffset(Scene& scene) const;

	/**
	 * @brief Destroys the active eating VFX, if it exists.
	 * @param scene Active scene containing the VFX object.
	 */
	void DestroyEatingVFX(Scene& scene);

	/**
	 * @brief Updates the low-patience warning glow, pulse, and shake state.
	 * @param scene Active scene containing the patience bar.
	 * @param dt Delta time for the frame.
	 * @param ratio01 Normalized patience value in the range `[0, 1]`.
	 * @param showBar True when the patience bar should currently be visible.
	 */
	void UpdateLowPatienceWarning(Scene& scene, float dt, float ratio01, bool showBar);

	/**
	 * @brief Ensures the low-patience outline overlay exists around the bar background.
	 * @param scene Active scene containing the patience bar.
	 */
	void EnsureLowPatienceGlow(Scene& scene);

	/**
	 * @brief Keeps the low-patience glow aligned with the bar and updates its pulse alpha.
	 * @param scene Active scene containing the glow sprites.
	 * @param alpha Alpha value to apply to the outline sprites.
	 */
	void SyncLowPatienceGlow(Scene& scene, float alpha);

	/**
	 * @brief Destroys the low-patience glow overlay if it exists.
	 * @param scene Active scene containing the glow sprites.
	 */
	void DestroyLowPatienceGlow(Scene& scene);

	const char* happyFacePath_ = "../assets/UI/Reaction_Happy_Face.png";
	const char* sadFacePath_ = "../assets/UI/Reaction_Angry_Face.png";

	/**
	 * @brief Spawns a temporary payment reaction VFX above the customer.
	 * @param scene Active scene containing the customer.
	 * @param path Texture path for the VFX sprite to spawn.
	 */
	void SpawnPaymentVFX(Scene& scene, const char* path);

	/**
	 * @brief Updates payment VFX movement and lifetime while it is active.
	 * @param scene Active scene containing the spawned VFX object.
	 * @param dt Delta time for the frame.
	 */
	void UpdatePaymentVFX(Scene& scene, float dt);

	/**
	 * @brief Destroys the payment VFX object and resets its runtime state.
	 * @param scene Active scene containing the spawned VFX object.
	 */
	void DestroyPaymentVFX(Scene& scene);

	/**
	 * @brief Returns icon dimensions tailored to the requested icon texture.
	 * @param iconPath Texture path for the icon being displayed.
	 * @return Authored icon size for the requested texture.
	 */
	glm::vec2 GetIconSizeForPath(const char* iconPath) const;

	/**
	 * @brief Maps a dish type to the icon texture shown in the thought bubble.
	 * @param dish Requested dish type.
	 * @return Texture path for the corresponding dish icon.
	 */
	const char* DishToIconPath(DishType dish) const;

	/**
	 * @brief Launches the success reaction that flies toward the order panel.
	 * @param scene Active scene containing the customer and order UI.
	 */
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
