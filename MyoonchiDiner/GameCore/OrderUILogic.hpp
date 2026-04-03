/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         OrderUILogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Declares the OrderUILogic component, responsible for collecting
					active customer orders and displaying them as animated order
					tickets with dish, ingredient, and station icons.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "EngineCore/GameObjectLogic.hpp"
#include "EngineCore/Math.hpp"
#include "GameCore/FoodTypes.hpp"

class Scene;
class InputManager;

class OrderUILogic : public GameObjectLogic {
public:
	// lets customer UI find where its order card currently is
	static bool TryGetPanelCenterForCustomer(int customerId, glm::vec2& outPos);

	// lets customer UI ask the order UI to play the stamp on arrival
	static void RequestCompletionStampForCustomer(int customerId);

	// UI state for each order slot
	struct OrderSlot {
		int customerId = -1;
		int tableId = -1;

		int panelId = -1;
		int dishIconId = -1;

		std::vector<int> ingredientIconIds;
		std::vector<int> stationIconIds;

		bool panelSpawned = false;
		DishType lastDish = DishType::PoopDish;
		bool hasLastDish = false;

		// completion animation
		bool completing = false;
		float completionTimer = 0.0f;
		glm::vec2 completionStartPanelPos = { 0.0f, 0.0f };

		// stamp overlay
		int stampId = -1;
		bool stampActive = false;
		float stampTimer = 0.0f;
	};

	using GameObjectLogic::GameObjectLogic;

	void Start(Scene& scene) override;
	void Update(float dt, Scene& scene, InputManager& input) override;
	void OnDestroy(Scene& scene) override;

	std::string GetName() const override {
		return "OrderUILogic";
	}

private:
	static constexpr int kMaxOrders = 4;
	static constexpr int kRecipeCols = 2;

	struct WaitingOrder {
		int tableId = -1;
		int customerId = -1;
		DishType dish = DishType::PoopDish;
		float patienceRemaining = 0.0f;
		float patienceRatio = 0.0f;
	};

	std::vector<OrderSlot> slots_;

	const char* panelTex_ = "../assets/UI/Order_UI.png";
	const char* darkPanelTex_ = "../assets/UI/Order_UI_Dark.png";
	const char* invisTex_ = "../assets/UI/invis.png";

	std::string panelLayer_ = "3";
	std::string dishLayer_ = "4";
	std::string ingredientLayer_ = "4";
	std::string stationLayer_ = "4";
	std::string stampLayer_ = "5";

	glm::vec2 panelTargetPos_ = { 460.f, 64.f };
	glm::vec2 panelSize_ = { 168.f, 124.f };
	float slideDuration_ = 0.45f;

	glm::vec2 dishOffset_ = { 0.f, -38.f };
	glm::vec2 dishSize_ = { 60.f, 60.f };

	std::vector<glm::vec2> ingredientOffsets_ = { { -20.f, 4.f }, { 20.f, 4.f } };
	glm::vec2 ingredientSize_ = { 35.f, 35.f };

	std::vector<glm::vec2> stationOffsets_ = { { -20.f, 40.f }, { 20.f, 40.f } };
	glm::vec2 stationSize_ = { 35.f, 35.f };

	float ticketGapY_ = 120.f;

	// bigger pop + slower shrink/fade
	float completionDuration_ = 0.82f;
	float completionPopDuration_ = 0.46f;
	float completionPopScale_ = 1.50f;
	float completionRiseDistance_ = 38.0f;

	// left-compaction movement
	float livePanelMoveSpeed_ = 14.0f;

	// green-face stamp on ticket
	const char* completionStampTex_ = "../assets/UI/Reaction_Happy_Face.png";
	glm::vec2 completionStampOffset_ = { 0.f, -8.f };
	glm::vec2 completionStampSize_ = { 86.f, 86.f };
	float completionStampDuration_ = 0.34f;
	float completionStampImpactScale_ = 1.30f;

	void BeginCompleteAnimation(Scene& scene, int slotIndex, OrderSlot& slot);
	bool UpdateCompleteAnimation(Scene& scene, OrderSlot& slot, float dt);

	void ApplySlotVisualState(Scene& scene, OrderSlot& slot,
		const glm::vec2& panelPos, float scaleMul, float alpha);

	void UpdateLivePanelLayout(Scene& scene, int slotIndex, OrderSlot& slot, float dt);
	void CompactSlotsLeft();

	void StartCompletionStamp(Scene& scene, OrderSlot& slot);
	void UpdateCompletionStamp(Scene& scene, OrderSlot& slot,
		float dt, const glm::vec2& panelPos, float parentAlpha);

	void CollectWaitingOrders(Scene& scene, std::vector<WaitingOrder>& outOrders);

	glm::vec2 SlotTargetPos(int slotIndex) const;
	void ClearSlot(Scene& scene, OrderSlot& slot);

	void EnsurePanel(Scene& scene, int slotIndex, OrderSlot& slot);
	void EnsureDishIcon(Scene& scene, OrderSlot& slot);
	void EnsureRecipeIcons(Scene& scene, OrderSlot& slot);
	void FollowPanel(Scene& scene, OrderSlot& slot);
	glm::vec2 GetDishIconOffset(DishType dish) const;
	glm::vec2 GetDishIconSize(DishType dish) const;
	void UpdateDishIconVisualState(Scene& scene, OrderSlot& slot,
		const glm::vec2& panelPos, float scaleMul, float alpha);
	void SetPanelTexture(Scene& scene, int panelId, const char* texPath);
	const char* GetPanelTextureForTable(Scene& scene, int tableId) const;

	const char* DishToIconPath(DishType t) const;
	void SetIconTexture(Scene& scene, int iconId, const char* texPath);

	void UpdateRecipeIcons(Scene& scene, OrderSlot& slot, DishType dish);
	void GetRecipeIconPaths(DishType dish,
		std::vector<const char*>& outIngredientTex,
		std::vector<const char*>& outStationTex) const;

	void EnsureSubSprite(Scene& scene, int panelId, int& spriteId,
		const glm::vec2& offset, const glm::vec2& size,
		const std::string& layer);

	int FindOrderIndexByCustomer(const std::vector<WaitingOrder>& orders, int customerId) const;

	static std::unordered_map<int, glm::vec2> sCustomerPanelCenters_;
	static std::unordered_set<int> sPendingStampCustomers_;
};
