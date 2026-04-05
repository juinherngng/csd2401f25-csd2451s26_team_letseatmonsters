/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         OrderUILogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (90%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (10%)

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
	/**
	 * @brief Returns the current panel center for a specific customer's order ticket.
	 * @param customerId Runtime ID of the customer whose ticket should be located.
	 * @param outPos Output position populated when the ticket exists.
	 * @return True if the customer currently has an active panel center.
	 */
	static bool TryGetPanelCenterForCustomer(int customerId, glm::vec2& outPos);

	/**
	 * @brief Requests the completion stamp animation for a customer's ticket.
	 * @param customerId Runtime ID of the customer whose ticket should be stamped.
	 */
	static void RequestCompletionStampForCustomer(int customerId);

	/**
	 * @brief Stores the runtime UI state for one active order slot.
	 */
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

		// Runtime state for the panel completion pop-and-slide animation.
		bool completing = false;
		float completionTimer = 0.0f;
		glm::vec2 completionStartPanelPos = { 0.0f, 0.0f };

		// Runtime state for the green completion stamp overlay.
		int stampId = -1;
		bool stampActive = false;
		float stampTimer = 0.0f;
	};

	using GameObjectLogic::GameObjectLogic;

	/**
	 * @brief Initializes order-ticket UI state when the scene starts.
	 * @param scene Active scene containing the order UI anchor object.
	 */
	void Start(Scene& scene) override;

	/**
	 * @brief Updates active order tickets and their animations for one frame.
	 * @param dt Delta time for the frame.
	 * @param scene Active scene containing the order UI and customers.
	 * @param input Input manager forwarded by the logic system.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Destroys all spawned ticket UI owned by this logic.
	 * @param scene Active scene containing the spawned order-ticket objects.
	 */
	void OnDestroy(Scene& scene) override;

	/**
	 * @brief Returns the stable runtime logic name used by the engine.
	 * @return Name string for this logic component.
	 */
	std::string GetName() const override {
		// Keep the logic name stable for debugging and runtime registration.
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

	/**
	 * @brief Starts the completion animation for a specific order slot.
	 * @param scene Active scene containing the slot's ticket objects.
	 * @param slotIndex Zero-based slot index being completed.
	 * @param slot Order slot whose animation should begin.
	 */
	void BeginCompleteAnimation(Scene& scene, int slotIndex, OrderSlot& slot);

	/**
	 * @brief Advances one frame of the completion animation for a slot.
	 * @param scene Active scene containing the slot's ticket objects.
	 * @param slot Order slot whose animation should advance.
	 * @param dt Delta time for the frame.
	 * @return True while the completion animation is still active.
	 */
	bool UpdateCompleteAnimation(Scene& scene, OrderSlot& slot, float dt);

	/**
	 * @brief Applies panel, icon, and alpha state for a slot's current visual pose.
	 * @param scene Active scene containing the slot's ticket objects.
	 * @param slot Order slot whose visuals should be updated.
	 * @param panelPos World-space panel position for the slot.
	 * @param scaleMul Scale multiplier applied to the slot visuals.
	 * @param alpha Alpha value applied to the slot visuals.
	 */
	void ApplySlotVisualState(Scene& scene, OrderSlot& slot,
		const glm::vec2& panelPos, float scaleMul, float alpha);

	/**
	 * @brief Updates the live left-compaction movement for an active slot.
	 * @param scene Active scene containing the slot's ticket objects.
	 * @param slotIndex Zero-based slot index being updated.
	 * @param slot Order slot whose layout should be updated.
	 * @param dt Delta time for the frame.
	 */
	void UpdateLivePanelLayout(Scene& scene, int slotIndex, OrderSlot& slot, float dt);

	/**
	 * @brief Compacts active slots leftward after removals.
	 */
	void CompactSlotsLeft();

	/**
	 * @brief Starts the green completion stamp animation for a slot.
	 * @param scene Active scene containing the slot's ticket objects.
	 * @param slot Order slot whose stamp animation should begin.
	 */
	void StartCompletionStamp(Scene& scene, OrderSlot& slot);

	/**
	 * @brief Updates the completion stamp overlay for one frame.
	 * @param scene Active scene containing the slot's ticket objects.
	 * @param slot Order slot whose stamp should be updated.
	 * @param dt Delta time for the frame.
	 * @param panelPos Current world-space panel position.
	 * @param parentAlpha Parent alpha inherited from the slot visuals.
	 */
	void UpdateCompletionStamp(Scene& scene, OrderSlot& slot,
		float dt, const glm::vec2& panelPos, float parentAlpha);

	/**
	 * @brief Collects all currently waiting customer orders from the scene.
	 * @param scene Active scene containing customers and tables.
	 * @param outOrders Output array populated with waiting-order data.
	 */
	void CollectWaitingOrders(Scene& scene, std::vector<WaitingOrder>& outOrders);

	/**
	 * @brief Returns the target panel position for a given slot index.
	 * @param slotIndex Zero-based slot index.
	 * @return Target position for that slot's ticket panel.
	 */
	glm::vec2 SlotTargetPos(int slotIndex) const;

	/**
	 * @brief Destroys all UI objects associated with a slot and resets its state.
	 * @param scene Active scene containing the slot's ticket objects.
	 * @param slot Order slot to clear.
	 */
	void ClearSlot(Scene& scene, OrderSlot& slot);

	/**
	 * @brief Ensures the main ticket panel exists for a slot.
	 * @param scene Active scene containing the order UI.
	 * @param slotIndex Zero-based slot index being ensured.
	 * @param slot Order slot whose panel should exist.
	 */
	void EnsurePanel(Scene& scene, int slotIndex, OrderSlot& slot);

	/**
	 * @brief Ensures the dish icon exists for a slot.
	 * @param scene Active scene containing the order UI.
	 * @param slot Order slot whose dish icon should exist.
	 */
	void EnsureDishIcon(Scene& scene, OrderSlot& slot);

	/**
	 * @brief Ensures the ingredient and station recipe icons exist for a slot.
	 * @param scene Active scene containing the order UI.
	 * @param slot Order slot whose recipe icons should exist.
	 */
	void EnsureRecipeIcons(Scene& scene, OrderSlot& slot);

	/**
	 * @brief Keeps a slot's child icons aligned to its panel.
	 * @param scene Active scene containing the slot's ticket objects.
	 * @param slot Order slot whose child visuals should follow the panel.
	 */
	void FollowPanel(Scene& scene, OrderSlot& slot);

	/**
	 * @brief Returns the dish-icon offset for a given dish type.
	 * @param dish Dish type displayed on the ticket.
	 * @return Offset applied to the dish icon relative to the panel.
	 */
	glm::vec2 GetDishIconOffset(DishType dish) const;

	/**
	 * @brief Returns the dish-icon size for a given dish type.
	 * @param dish Dish type displayed on the ticket.
	 * @return Size applied to the dish icon.
	 */
	glm::vec2 GetDishIconSize(DishType dish) const;

	/**
	 * @brief Updates the dish icon's visual state using the slot's current pose.
	 * @param scene Active scene containing the slot's ticket objects.
	 * @param slot Order slot whose dish icon should be updated.
	 * @param panelPos Current world-space panel position.
	 * @param scaleMul Scale multiplier applied to the dish icon.
	 * @param alpha Alpha value applied to the dish icon.
	 */
	void UpdateDishIconVisualState(Scene& scene, OrderSlot& slot,
		const glm::vec2& panelPos, float scaleMul, float alpha);

	/**
	 * @brief Applies a texture to a ticket panel and synchronizes scene metadata.
	 * @param scene Active scene containing the ticket panel.
	 * @param panelId Runtime ID of the panel object.
	 * @param texPath Texture path that should be assigned to the panel.
	 */
	void SetPanelTexture(Scene& scene, int panelId, const char* texPath);

	/**
	 * @brief Returns the correct panel texture for the specified table.
	 * @param scene Active scene containing table data.
	 * @param tableId Runtime ID of the table associated with the order.
	 * @return Texture path for the panel background that should be used.
	 */
	const char* GetPanelTextureForTable(Scene& scene, int tableId) const;

	/**
	 * @brief Maps a dish type to the icon texture used on the ticket.
	 * @param t Dish type displayed by the slot.
	 * @return Texture path for the dish icon.
	 */
	const char* DishToIconPath(DishType t) const;

	/**
	 * @brief Applies a texture to a ticket sub-icon and synchronizes scene metadata.
	 * @param scene Active scene containing the icon object.
	 * @param iconId Runtime ID of the icon object.
	 * @param texPath Texture path that should be assigned to the icon.
	 */
	void SetIconTexture(Scene& scene, int iconId, const char* texPath);

	/**
	 * @brief Refreshes the recipe icons shown for the specified dish.
	 * @param scene Active scene containing the slot's ticket objects.
	 * @param slot Order slot whose recipe icons should be refreshed.
	 * @param dish Dish type currently shown in the slot.
	 */
	void UpdateRecipeIcons(Scene& scene, OrderSlot& slot, DishType dish);

	/**
	 * @brief Resolves the ingredient and station icon textures for a recipe.
	 * @param dish Dish type whose recipe icons should be resolved.
	 * @param outIngredientTex Output list populated with ingredient icon textures.
	 * @param outStationTex Output list populated with station icon textures.
	 */
	void GetRecipeIconPaths(DishType dish,
		std::vector<const char*>& outIngredientTex,
		std::vector<const char*>& outStationTex) const;

	/**
	 * @brief Ensures a child sprite exists for a ticket panel.
	 * @param scene Active scene containing the ticket panel.
	 * @param panelId Runtime ID of the parent panel.
	 * @param spriteId In-out runtime ID of the child sprite.
	 * @param offset Offset of the child sprite relative to the panel.
	 * @param size Size of the child sprite.
	 * @param layer Render layer assigned to the child sprite.
	 */
	void EnsureSubSprite(Scene& scene, int panelId, int& spriteId,
		const glm::vec2& offset, const glm::vec2& size,
		const std::string& layer);

	/**
	 * @brief Finds the waiting-order index associated with a specific customer.
	 * @param orders Waiting-order list currently collected from the scene.
	 * @param customerId Runtime ID of the customer to locate.
	 * @return Index into `orders`, or `-1` when no entry matches the customer.
	 */
	int FindOrderIndexByCustomer(const std::vector<WaitingOrder>& orders, int customerId) const;

	static std::unordered_map<int, glm::vec2> sCustomerPanelCenters_;
	static std::unordered_set<int> sPendingStampCustomers_;
};
