/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         OrderUILogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Declares the OrderUILogic component, responsible for collecting
					active customer orders and displaying them as animated order
					tickets with dish, ingredient, and station icons.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once
#include "Core/FoodTypes.hpp"
#include "Core/GameObjectLogic.hpp"
#include "Core/Math.hpp"

#include <glm/glm.hpp>
#include <string>
#include <vector>

class Scene;
class InputManager;

// Component attached to a persistent GameObject that manages the Order UI display.
class OrderUILogic : public GameObjectLogic {
public:
	// Inherit constructor
	using GameObjectLogic::GameObjectLogic;

	// Lifecycle
	void Start(Scene& scene) override;
	void Update(float dt, Scene& scene, InputManager& input) override;
	void OnDestroy(Scene& scene) override;

	// Debug
	std::string GetName() const override {
		return "OrderUILogic";
	}

private:
	static constexpr int kMaxOrders = 4;
	static constexpr int kRecipeCols = 2; // 2 ingredients + 2 stations

	struct WaitingOrder {
		int tableId = -1;
		int customerId = -1;
		DishType dish = DishType::PoopDish;
	};

	// UI state for each order slot
	struct OrderSlot {
		int customerId = -1;

		int panelId = -1;

		// Dish icon (top)
		int dishIconId = -1;

		// Recipe icons
		std::vector<int> ingredientIconIds; // size = kRecipeCols
		std::vector<int> stationIconIds;    // size = kRecipeCols

		bool panelSpawned = false;
		DishType lastDish = DishType::PoopDish;
		bool hasLastDish = false;
	};

	std::vector<OrderSlot> slots_;

	// --------------------
	// UI Config
	// --------------------
	const char* panelTex_ = "../assets/Order_UI.png";
	const char* invisTex_ = "../assets/invis.png";

	// Render layers (TWEAK THESE to control draw order of UI elements)
	std::string panelLayer_ = "3";
	std::string dishLayer_ = "4";
	std::string ingredientLayer_ = "4";
	std::string stationLayer_ = "4";

	// Panel slide-in config
	glm::vec2 panelTargetPos_ = { 460.f, 64.f };
	glm::vec2 panelSize_ = { 168.f, 124.f };
	float slideDuration_ = 0.45f;

	// Layout offsets relative to panel position (TWEAK THESE to match your panel art)
	glm::vec2 dishOffset_ = { 0.f, -44.f };
	glm::vec2 dishSize_ = { 75.f, 75.f };

	// Ingredient and station icons are arranged in a 2-column grid below the dish icon, with these offsets from the panel position as the center of each icon
	std::vector<glm::vec2> ingredientOffsets_ = { { -20.f, 4.f }, { 20.f, 4.f } };
	glm::vec2 ingredientSize_ = { 40.f, 40.f };

	// Station icons are arranged in a 2-column grid below the ingredient icons, with these offsets from the panel position as the center of each icon
	std::vector<glm::vec2> stationOffsets_ = { { -20.f, 40.f }, { 20.f, 40.f } };
	glm::vec2 stationSize_ = { 35.f, 35.f };

	// Animation config
	float ticketGapY_ = 120.f; // space between tickets (used in SlotTargetPos)

private:
	// --------------------
	// Order Collection
	// --------------------
	void CollectWaitingOrders(Scene& scene, std::vector<WaitingOrder>& outOrders);

	// Slot helpers
	glm::vec2 SlotTargetPos(int slotIndex) const;
	void ClearSlot(Scene& scene, OrderSlot& slot);

	// Spawn/maintain UI
	void EnsurePanel(Scene& scene, int slotIndex, OrderSlot& slot);
	void EnsureDishIcon(Scene& scene, OrderSlot& slot);
	void EnsureRecipeIcons(Scene& scene, OrderSlot& slot);
	void FollowPanel(Scene& scene, OrderSlot& slot);

	// Texture updates
	const char* DishToIconPath(DishType t) const;
	void SetIconTexture(Scene& scene, int iconId, const char* texPath);

	// Update the dish icon and recipe icons for a slot based on the given dish type
	void UpdateRecipeIcons(Scene& scene, OrderSlot& slot, DishType dish);
	void GetRecipeIconPaths(DishType dish,
		std::vector<const char*>& outIngredientTex,
		std::vector<const char*>& outStationTex) const;

	// Generic spawn helper
	void EnsureSubSprite(Scene& scene, int panelId, int& spriteId,
		const glm::vec2& offset, const glm::vec2& size,
		const std::string& layer);

	// Utility
	int FindOrderIndexByCustomer(const std::vector<WaitingOrder>& orders, int customerId) const;
};
