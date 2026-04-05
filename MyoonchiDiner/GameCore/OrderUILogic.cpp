/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         OrderUILogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (85%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (15%)

 DESCRIPTION:       Defines the OrderUILogic component, responsible for collecting
					active customer orders and displaying them as animated order
					tickets with dish, ingredient, and station icons.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>

#include "EngineCore/LogicManager.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/CustomerTableLogic.hpp"
#include "GameCore/OrderUILogic.hpp"
#include "GameCore/SimpleNpcLogic.hpp"

std::unordered_map<int, glm::vec2> OrderUILogic::sCustomerPanelCenters_{};
std::unordered_set<int> OrderUILogic::sPendingStampCustomers_{};

namespace {
	constexpr int kCustomerBubbleTopLayer = 101;
	constexpr int kOrderTicketMinBaseLayer = kCustomerBubbleTopLayer + 1;

	/**
	 * @brief Parses a numeric layer name and falls back when parsing fails.
	 * @param layerName Layer name string to parse.
	 * @param fallback Fallback layer number when parsing is invalid.
	 * @return Parsed layer number or the supplied fallback.
	 */
	int ParseNumericLayerOrFallback(const std::string& layerName, int fallback) {
		if (layerName.empty()) {
			return fallback;
		}

		int value = 0;
		for (char c : layerName) {
			if (c < '0' || c > '9') {
				return fallback;
			}
			value = value * 10 + (c - '0');
		}

		return value;
	}

	/**
	 * @brief Returns the ingredient-box icon that represents an ingredient type.
	 * @param type Ingredient type to convert into an icon path.
	 * @return Texture path for the matching ingredient-box icon.
	 */
	const char* IngredientBoxIconPath(IngredientType type) {
		switch (type) {
		case IngredientType::Vegetable:
		case IngredientType::Refined_Veg:
			return "../assets/Food/VegIngredientBox.png";
		case IngredientType::Meat:
		case IngredientType::Refined_Meat:
			return "../assets/Food/MeatIngredientBox.png";
		case IngredientType::Shroom:
		case IngredientType::Refined_Shroom:
			return "../assets/Food/ShroomIngredientBox.png";
		case IngredientType::Carrot:
		case IngredientType::Refined_Carrot:
			return "../assets/Food/CarrotIngredientBox.png";
		default:
			return "../assets/Food/VegIngredientBox.png";
		}
	}

}

/**
 * @brief Returns the current order-panel center for a specific customer when available.
 * @param customerId Runtime ID of the customer being queried.
 * @param outPos Output panel-center position in world space.
 * @return True when the customer currently owns a visible order panel.
 */
bool OrderUILogic::TryGetPanelCenterForCustomer(int customerId, glm::vec2& outPos) {
	auto it = sCustomerPanelCenters_.find(customerId);
	if (it == sCustomerPanelCenters_.end()) {
		return false;
	}
	outPos = it->second;
	return true;
}

/**
 * @brief Queues a completion stamp animation for the specified customer's order panel.
 * @param customerId Runtime ID of the customer whose panel should be stamped.
 */
void OrderUILogic::RequestCompletionStampForCustomer(int customerId) {
	if (customerId >= 0) {
		// Defer the actual stamp spawn so callers can request it from outside the HUD update loop safely.
		sPendingStampCustomers_.insert(customerId);
	}
}

/**
 * @brief Clamps a scalar to the normalized range `[0, 1]`.
 * @param v Value to clamp.
 * @return Clamped normalized value.
 */
static float Clamp01(float v) {
	if (v < 0.f) return 0.f;
	if (v > 1.f) return 1.f;
	return v;
}

/**
 * @brief Evaluates an ease-out cubic curve on a normalized input.
 * @param x Normalized interpolation input.
 * @return Eased output value.
 */
static float EaseOutCubic(float x) {
	x = Clamp01(x);
	float inv = 1.0f - x;
	return 1.0f - inv * inv * inv;
}

/**
 * @brief Evaluates an ease-in cubic curve on a normalized input.
 * @param x Normalized interpolation input.
 * @return Eased output value.
 */
static float EaseInCubic(float x) {
	x = Clamp01(x);
	return x * x * x;
}

/**
 * @brief Evaluates an ease-out-back curve that overshoots slightly before settling.
 * @param x Normalized interpolation input.
 * @return Eased output value.
 */
static float EaseOutBack(float x) {
	x = Clamp01(x);
	constexpr float c1 = 1.70158f;
	constexpr float c3 = c1 + 1.0f;
	float y = x - 1.0f;
	return 1.0f + c3 * y * y * y + c1 * y * y;
}

/**
 * @brief Despawns an object if its ID still points at a live scene object.
 * @param scene Active scene containing the object.
 * @param id In-out object ID to despawn and invalidate.
 */
static void DespawnIfAlive(Scene& scene, int& id) {
	if (id >= 0) {
		scene.DespawnByID(id); id = -1;
	}
}

/**
 * @brief Returns the dish icon path used by this HUD for a specific dish type.
 * @param t Dish type to convert into an icon path.
 * @return Texture path for the corresponding dish icon.
 */
const char* OrderUILogic::DishToIconPath(DishType t) const {
	switch (t) {
	case DishType::VegDish:  return "../assets/Food/Food_Salad.png";
	case DishType::MeatDish: return "../assets/Food/Food_Meat.png";
	case DishType::SoupDish: return "../assets/Food/Food_Mushroom.png";
	case DishType::SkewerDish: return "../assets/Food/Food_Meat_n_carrot.png";
	case DishType::CarrotSaladDish: return "../assets/Food/Food_Salad_n_carrot.png";
	case DishType::PoopDish: return "../assets/Food/poop.png";
	default:                 return "../assets/Food/Food_Salad.png";
	}
}

/**
 * @brief Initializes HUD slots, shared lookup state, and authored layer settings.
 * @param scene Active scene containing the order UI anchor object.
 */
void OrderUILogic::Start(Scene& scene) {
	slots_.clear();
	slots_.resize(kMaxOrders);

	sCustomerPanelCenters_.clear();
	sPendingStampCustomers_.clear();

	for (auto& slot : slots_) {
		slot.ingredientIconIds.assign(kRecipeCols, -1);
		slot.stationIconIds.assign(kRecipeCols, -1);
		slot.completing = false;
		slot.completionTimer = 0.0f;
		slot.completionStartPanelPos = { 0.0f, 0.0f };

		slot.stampId = -1;
		slot.stampActive = false;
		slot.stampTimer = 0.0f;
	}

	// Use the authored level anchor so each level can place the order HUD where it fits best.
	if (GameObject* anchor = scene.GetGameObjectByID(GetOwnerID())) {
		const glm::vec3 anchorPos = anchor->GetPositionGLM();
		panelTargetPos_ = { anchorPos.x, anchorPos.y };
	}

	const Scene::Defaults defaults = scene.GetDefaults(GetOwnerID());
	const int authoredLayer = ParseNumericLayerOrFallback(defaults.layer, 99);
	const int baseLayer = std::max(authoredLayer, kOrderTicketMinBaseLayer);

	// Reserve the order-ticket stack above customer speech/thought UI while still
	// honoring any authored layer that is already higher.
	panelLayer_ = std::to_string(baseLayer);
	dishLayer_ = std::to_string(baseLayer + 1);
	ingredientLayer_ = std::to_string(baseLayer + 1);
	stationLayer_ = std::to_string(baseLayer + 1);
	stampLayer_ = std::to_string(baseLayer + 2);
}

/**
 * @brief Despawns every spawned order-panel object owned by this HUD.
 * @param scene Active scene containing the order UI objects.
 */
void OrderUILogic::OnDestroy(Scene& scene) {
	for (auto& slot : slots_)
		ClearSlot(scene, slot);
}

/**
 * @brief Returns the target position for a slot within the order-ticket strip.
 * @param slotIndex Zero-based slot index.
 * @return World-space target position for the slot's panel.
 */
glm::vec2 OrderUILogic::SlotTargetPos(int slotIndex) const {
	// Anchor the first order beside the quota UI, then grow additional tickets rightward.
	return glm::vec2(
		panelTargetPos_.x + slotIndex * (panelSize_.x + ticketGapY_),
		panelTargetPos_.y
	);
}

/**
 * @brief Despawns all spawned visuals owned by a slot and resets its state.
 * @param scene Active scene containing the order UI objects.
 * @param slot Slot record to clear.
 */
void OrderUILogic::ClearSlot(Scene& scene, OrderSlot& slot) {
	DespawnIfAlive(scene, slot.dishIconId);

	for (int& id : slot.ingredientIconIds) DespawnIfAlive(scene, id);
	for (int& id : slot.stationIconIds)    DespawnIfAlive(scene, id);

	DespawnIfAlive(scene, slot.panelId);
	DespawnIfAlive(scene, slot.stampId);

	slot.customerId = -1;
	slot.tableId = -1;
	slot.panelSpawned = false;
	slot.lastDish = DishType::PoopDish;
	slot.hasLastDish = false;

	slot.completing = false;
	slot.completionTimer = 0.0f;
	slot.completionStartPanelPos = { 0.0f, 0.0f };

	slot.stampActive = false;
	slot.stampTimer = 0.0f;

	if ((int)slot.ingredientIconIds.size() != kRecipeCols)
		slot.ingredientIconIds.assign(kRecipeCols, -1);
	if ((int)slot.stationIconIds.size() != kRecipeCols)
		slot.stationIconIds.assign(kRecipeCols, -1);
}

/**
 * @brief Compacts active slots toward the left after completed cards are removed.
 */
void OrderUILogic::CompactSlotsLeft() {
	for (int i = 0; i < (int)slots_.size(); ++i) {
		if (slots_[i].customerId >= 0 || slots_[i].completing) {
			continue;
		}

		for (int j = i + 1; j < (int)slots_.size(); ++j) {
			if (slots_[j].customerId >= 0 && !slots_[j].completing) {
				std::swap(slots_[i], slots_[j]);
				break;
			}
		}
	}
}

/**
 * @brief Updates a live slot's panel position and its child visuals.
 * @param scene Active scene containing the slot objects.
 * @param slotIndex Zero-based slot index.
 * @param slot Slot record to update.
 * @param dt Delta time for the frame.
 */
void OrderUILogic::UpdateLivePanelLayout(Scene& scene, int slotIndex, OrderSlot& slot, float dt) {
	if (slot.panelId < 0) {
		return;
	}

	GameObject* panel = scene.GetGameObjectByID(slot.panelId);
	if (!panel) {
		return;
	}

	const glm::vec2 target = SlotTargetPos(slotIndex);
	glm::vec3 cur = panel->GetPositionGLM();

	const float lerpT = std::clamp(dt * livePanelMoveSpeed_, 0.0f, 1.0f);
	cur.x = cur.x + (target.x - cur.x) * lerpT;
	cur.y = cur.y + (target.y - cur.y) * lerpT;

	panel->SetPosition(glm::vec3(cur.x, cur.y, cur.z));
	FollowPanel(scene, slot);
	UpdateDishIconVisualState(scene, slot, glm::vec2(cur.x, cur.y), 1.0f, 1.0f);

	if (slot.customerId >= 0) {
		sCustomerPanelCenters_[slot.customerId] = glm::vec2(cur.x, cur.y);
	}

	UpdateCompletionStamp(scene, slot, dt, glm::vec2(cur.x, cur.y), 1.0f);
}

/**
 * @brief Starts the completion-stamp animation for a slot if one is not already active.
 * @param scene Active scene containing the order UI objects.
 * @param slot Slot record whose stamp should begin animating.
 */
void OrderUILogic::StartCompletionStamp(Scene& scene, OrderSlot& slot) {
	if (slot.stampActive) {
		return;
	}

	glm::vec2 panelPos = slot.completionStartPanelPos;
	if (GameObject* panel = scene.GetGameObjectByID(slot.panelId)) {
		glm::vec3 p = panel->GetPositionGLM();
		panelPos = { p.x, p.y };
	}

	if (slot.stampId < 0) {
		if (GameObject* stamp = scene.SpawnStaticSprite(
			completionStampTex_,
			{ panelPos.x + completionStampOffset_.x, panelPos.y + completionStampOffset_.y, 0.0f },
			completionStampSize_,
			stampLayer_)) {
			slot.stampId = stamp->GetID();
			stamp->SetColliderSize(Math::Vector2D(0.f, 0.f));
			stamp->SetRenderSortOrder(25);
			scene.SetObjectTexturePath(slot.stampId, completionStampTex_);
		}
	}

	slot.stampActive = (slot.stampId >= 0);
	slot.stampTimer = 0.0f;
}

/**
 * @brief Advances the completion-stamp animation and updates its transform and alpha.
 * @param scene Active scene containing the stamp object.
 * @param slot Slot record owning the stamp.
 * @param dt Delta time for the frame.
 * @param panelPos Current panel center position.
 * @param parentAlpha Parent panel alpha multiplier.
 */
void OrderUILogic::UpdateCompletionStamp(Scene& scene, OrderSlot& slot,
	float dt, const glm::vec2& panelPos, float parentAlpha) {
	if (!slot.stampActive || slot.stampId < 0) {
		return;
	}

	GameObject* stamp = scene.GetGameObjectByID(slot.stampId);
	if (!stamp) {
		slot.stampId = -1;
		slot.stampActive = false;
		slot.stampTimer = 0.0f;
		return;
	}

	slot.stampTimer += dt;
	const float t = Clamp01(slot.stampTimer / std::max(0.001f, completionStampDuration_));

	float scaleMul = 1.0f;
	if (t < 0.42f) {
		const float local = t / 0.42f;
		scaleMul = 0.55f + (completionStampImpactScale_ - 0.55f) * EaseOutBack(local);
	}
	else {
		const float local = (t - 0.42f) / 0.58f;
		scaleMul = completionStampImpactScale_ + (1.0f - completionStampImpactScale_) * EaseOutCubic(local);
	}

	float alpha = parentAlpha;
	if (t > 0.72f) {
		const float fadeT = (t - 0.72f) / 0.28f;
		alpha *= (1.0f - EaseInCubic(fadeT));
	}

	stamp->SetPosition(glm::vec3(
		panelPos.x + completionStampOffset_.x,
		panelPos.y + completionStampOffset_.y,
		stamp->GetPositionGLM().z));

	stamp->SetScale(glm::vec3(
		completionStampSize_.x * scaleMul,
		completionStampSize_.y * scaleMul,
		1.0f));

	stamp->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, Clamp01(alpha)));

	if (slot.stampTimer >= completionStampDuration_) {
		DespawnIfAlive(scene, slot.stampId);
		slot.stampActive = false;
		slot.stampTimer = 0.0f;
	}
}

/**
 * @brief Starts the slot-completion animation for an order that just resolved.
 * @param scene Active scene containing the order UI objects.
 * @param slotIndex Zero-based slot index.
 * @param slot Slot record to animate out.
 */
void OrderUILogic::BeginCompleteAnimation(Scene& scene, int slotIndex, OrderSlot& slot) {
	if (slot.completing) {
		return;
	}

	slot.completing = true;
	slot.completionTimer = 0.0f;
	slot.completionStartPanelPos = SlotTargetPos(slotIndex);

	if (GameObject* panel = scene.GetGameObjectByID(slot.panelId)) {
		glm::vec3 p = panel->GetPositionGLM();
		slot.completionStartPanelPos = { p.x, p.y };
	}
}

/**
 * @brief Applies a shared transform, scale, and alpha state to all visuals in a slot.
 * @param scene Active scene containing the slot objects.
 * @param slot Slot record whose visuals should be updated.
 * @param panelPos Current panel center position.
 * @param scaleMul Uniform scale multiplier for the slot.
 * @param alpha Alpha multiplier for the slot.
 */
void OrderUILogic::ApplySlotVisualState(Scene& scene, OrderSlot& slot,
	const glm::vec2& panelPos, float scaleMul, float alpha) {
	alpha = Clamp01(alpha);

	auto applySprite = [&](int id, const glm::vec2& localOffset, const glm::vec2& baseSize) {
		if (id < 0) return;

		GameObject* obj = scene.GetGameObjectByID(id);
		if (!obj) return;

		glm::vec3 current = obj->GetPositionGLM();
		obj->SetPosition(glm::vec3(panelPos.x + localOffset.x, panelPos.y + localOffset.y, current.z));
		obj->SetScale(glm::vec3(baseSize.x * scaleMul, baseSize.y * scaleMul, 1.0f));
		obj->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, alpha));
		};

	applySprite(slot.panelId, glm::vec2(0.0f, 0.0f), panelSize_);
	UpdateDishIconVisualState(scene, slot, panelPos, scaleMul, alpha);

	for (int i = 0; i < kRecipeCols; ++i) {
		if (i < (int)ingredientOffsets_.size()) {
			applySprite(slot.ingredientIconIds[i], ingredientOffsets_[i], ingredientSize_);
		}
		if (i < (int)stationOffsets_.size()) {
			applySprite(slot.stationIconIds[i], stationOffsets_[i], stationSize_);
		}
	}
}

/**
 * @brief Advances the completion animation for a slot and reports when it finishes.
 * @param scene Active scene containing the slot objects.
 * @param slot Slot record being animated out.
 * @param dt Delta time for the frame.
 * @return True when the completion animation has fully finished.
 */
bool OrderUILogic::UpdateCompleteAnimation(Scene& scene, OrderSlot& slot, float dt) {
	if (!slot.completing) {
		return false;
	}

	slot.completionTimer += dt;

	const float total = std::max(0.001f, completionDuration_);
	const float popPhase = std::clamp(completionPopDuration_ / total, 0.05f, 0.95f);
	const float t = Clamp01(slot.completionTimer / total);

	glm::vec2 panelPos = slot.completionStartPanelPos;
	float scaleMul = 1.0f;
	float alpha = 1.0f;

	if (t < popPhase) {
		const float local = t / popPhase;

		// faster rise to 1.5x, slower shrink back down
		if (local < 0.35f) {
			const float upT = local / 0.35f;
			scaleMul = 1.0f + (completionPopScale_ - 1.0f) * EaseOutBack(upT);
		}
		else {
			const float downT = (local - 0.35f) / 0.65f;
			scaleMul = completionPopScale_ + (1.0f - completionPopScale_) * EaseOutCubic(downT);
		}
	}
	else {
		const float local = (t - popPhase) / (1.0f - popPhase);
		panelPos.y = slot.completionStartPanelPos.y - completionRiseDistance_ * EaseOutCubic(local);
		alpha = 1.0f - EaseInCubic(local);
		scaleMul = 1.0f;
	}

	ApplySlotVisualState(scene, slot, panelPos, scaleMul, alpha);
	UpdateCompletionStamp(scene, slot, dt, panelPos, alpha);

	if (slot.customerId >= 0) {
		sCustomerPanelCenters_[slot.customerId] = panelPos;
	}

	return slot.completionTimer >= total;
}

/**
 * @brief Returns the dish-icon offset appropriate for a specific dish type.
 * @param dish Dish type whose icon offset should be resolved.
 * @return Local icon offset relative to the panel center.
 */
glm::vec2 OrderUILogic::GetDishIconOffset(DishType dish) const {
	switch (dish) {
	case DishType::SkewerDish:
	case DishType::CarrotSaladDish:
		return glm::vec2(0.0f, -36.0f);
	default:
		return dishOffset_;
	}
}

/**
 * @brief Returns the dish-icon size appropriate for a specific dish type.
 * @param dish Dish type whose icon size should be resolved.
 * @return Icon size in scene units.
 */
glm::vec2 OrderUILogic::GetDishIconSize(DishType dish) const {
	switch (dish) {
	case DishType::VegDish:
	case DishType::MeatDish:
	case DishType::SoupDish:
	case DishType::SkewerDish:
	case DishType::CarrotSaladDish:
		return glm::vec2(50.0f, 50.0f);
	default:
		return dishSize_;
	}
}

/**
 * @brief Updates the dish icon's transform, size, and alpha to match the panel state.
 * @param scene Active scene containing the dish icon object.
 * @param slot Slot record owning the dish icon.
 * @param panelPos Current panel center position.
 * @param scaleMul Uniform scale multiplier for the slot.
 * @param alpha Alpha multiplier for the icon.
 */
void OrderUILogic::UpdateDishIconVisualState(Scene& scene, OrderSlot& slot,
	const glm::vec2& panelPos, float scaleMul, float alpha) {
	if (slot.dishIconId < 0) {
		return;
	}

	GameObject* icon = scene.GetGameObjectByID(slot.dishIconId);
	if (!icon) {
		return;
	}

	const DishType dish = slot.hasLastDish ? slot.lastDish : DishType::PoopDish;
	const glm::vec2 iconOffset = GetDishIconOffset(dish);
	const glm::vec2 iconSize = GetDishIconSize(dish);

	icon->SetPosition(glm::vec3(
		panelPos.x + iconOffset.x,
		panelPos.y + iconOffset.y,
		icon->GetPositionGLM().z
	));
	icon->SetScale(glm::vec3(iconSize.x * scaleMul, iconSize.y * scaleMul, 1.0f));
	icon->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, Clamp01(alpha)));
}

/**
 * @brief Returns the panel texture to use for a customer table in the current level.
 * @param scene Active scene containing the order UI and table defaults.
 * @param tableId Runtime ID of the customer table associated with the order.
 * @return Texture path for the correct panel style.
 */
const char* OrderUILogic::GetPanelTextureForTable(Scene& scene, int tableId) const {
	const std::string levelPath = scene.GetCurrentLevelPath();
	if (levelPath.find("kitchen02") == std::string::npos) {
		return panelTex_;
	}

	if (tableId < 0) {
		return panelTex_;
	}

	const Scene::Defaults tableDefaults = scene.GetDefaults(tableId);
	return tableDefaults.texture == "../assets/Furniture/Customertable_new.png"
		? darkPanelTex_
		: panelTex_;
}

/**
 * @brief Swaps a panel object's texture and updates its stored defaults.
 * @param scene Active scene containing the panel object.
 * @param panelId Runtime ID of the panel object.
 * @param texPath Texture path to assign.
 */
void OrderUILogic::SetPanelTexture(Scene& scene, int panelId, const char* texPath) {
	GameObject* panel = scene.GetGameObjectByID(panelId);
	if (!panel) {
		return;
	}

	panel->SetTexture(ResourceManager::Instance().LoadTexture(texPath, texPath));
	scene.SetObjectTexturePath(panelId, texPath);

	Scene::Defaults d = scene.GetDefaults(panelId);
	d.texture = texPath;
	scene.SetDefaults(panelId, d);
}

/**
 * @brief Ensures the slot owns a spawned panel and that it uses the correct texture.
 * @param scene Active scene containing the order UI objects.
 * @param slotIndex Zero-based slot index.
 * @param slot Slot record to populate.
 */
void OrderUILogic::EnsurePanel(Scene& scene, int slotIndex, OrderSlot& slot) {
	const char* panelTexPath = GetPanelTextureForTable(scene, slot.tableId);

	if (slot.panelSpawned && slot.panelId >= 0 && scene.GetGameObjectByID(slot.panelId)) {
		SetPanelTexture(scene, slot.panelId, panelTexPath);
		return;
	}

	slot.panelId = scene.TriggerOrderUiSlideIn(
		SlotTargetPos(slotIndex),
		panelSize_,
		panelLayer_,
		panelTexPath,
		slideDuration_
	);

	slot.panelSpawned = (slot.panelId >= 0);
}

/**
 * @brief Ensures a child sprite exists relative to a panel.
 * @param scene Active scene containing the panel object.
 * @param panelId Runtime ID of the parent panel.
 * @param spriteId In-out child sprite ID to create when missing.
 * @param offset Local offset from the panel center.
 * @param size Spawn size for the child sprite.
 * @param layer Layer name to use for the child sprite.
 */
void OrderUILogic::EnsureSubSprite(Scene& scene, int panelId, int& spriteId,
	const glm::vec2& offset, const glm::vec2& size,
	const std::string& layer) {
	if (spriteId >= 0 && scene.GetGameObjectByID(spriteId))
		return;

	GameObject* panel = scene.GetGameObjectByID(panelId);
	if (!panel) return;

	glm::vec3 p = panel->GetPositionGLM();

	if (GameObject* icon = scene.SpawnStaticSprite(
		invisTex_,
		{ p.x + offset.x, p.y + offset.y, p.z },
		size,
		layer)) {
		spriteId = icon->GetID();
		icon->SetColliderSize(Math::Vector2D(0.f, 0.f));
		scene.SetObjectTexturePath(spriteId, invisTex_);
	}
}

/**
 * @brief Ensures the slot owns a spawned dish icon sized for its current dish.
 * @param scene Active scene containing the order UI objects.
 * @param slot Slot record to populate.
 */
void OrderUILogic::EnsureDishIcon(Scene& scene, OrderSlot& slot) {
	const DishType dish = slot.hasLastDish ? slot.lastDish : DishType::PoopDish;
	EnsureSubSprite(scene, slot.panelId, slot.dishIconId,
		GetDishIconOffset(dish), GetDishIconSize(dish), dishLayer_);
}

/**
 * @brief Ensures the slot owns all ingredient and station icons needed for recipe display.
 * @param scene Active scene containing the order UI objects.
 * @param slot Slot record to populate.
 */
void OrderUILogic::EnsureRecipeIcons(Scene& scene, OrderSlot& slot) {
	// safety: if user forgot to set sizes to 2 in header, don't crash
	if ((int)ingredientOffsets_.size() < kRecipeCols || (int)stationOffsets_.size() < kRecipeCols)
		return;

	for (int i = 0; i < kRecipeCols; ++i) {
		EnsureSubSprite(scene, slot.panelId, slot.ingredientIconIds[i],
			ingredientOffsets_[i], ingredientSize_, ingredientLayer_);

		EnsureSubSprite(scene, slot.panelId, slot.stationIconIds[i],
			stationOffsets_[i], stationSize_, stationLayer_);
	}
}

/**
 * @brief Keeps every child visual aligned to its parent order panel.
 * @param scene Active scene containing the slot objects.
 * @param slot Slot record whose visuals should follow the panel.
 */
void OrderUILogic::FollowPanel(Scene& scene, OrderSlot& slot) {
	if (slot.panelId < 0) return;

	GameObject* panel = scene.GetGameObjectByID(slot.panelId);
	if (!panel) return;

	glm::vec3 p = panel->GetPositionGLM();

	auto move = [&](int spriteId, const glm::vec2& off) {
		if (spriteId < 0) return;
		GameObject* go = scene.GetGameObjectByID(spriteId);
		if (!go) return;
		go->SetPosition(Math::Vector3D(p.x + off.x, p.y + off.y, p.z));
		};

	if (slot.panelId >= 0) {
		UpdateDishIconVisualState(scene, slot, glm::vec2(p.x, p.y), 1.0f, 1.0f);
	}

	for (int i = 0; i < kRecipeCols; ++i) {
		if (i < (int)ingredientOffsets_.size())
			move(slot.ingredientIconIds[i], ingredientOffsets_[i]);

		if (i < (int)stationOffsets_.size())
			move(slot.stationIconIds[i], stationOffsets_[i]);
	}
}

/**
 * @brief Swaps an icon sprite's texture and updates its stored defaults.
 * @param scene Active scene containing the icon object.
 * @param iconId Runtime ID of the icon object.
 * @param texPath Texture path to assign.
 */
void OrderUILogic::SetIconTexture(Scene& scene, int iconId, const char* texPath) {
	GameObject* icon = scene.GetGameObjectByID(iconId);
	if (!icon) return;

	icon->SetTexture(ResourceManager::Instance().LoadTexture(texPath, texPath));
	scene.SetObjectTexturePath(iconId, texPath);

	Scene::Defaults d = scene.GetDefaults(iconId);
	d.texture = texPath;
	scene.SetDefaults(iconId, d);
}

/**
 * @brief Finds the index of a waiting order for a specific customer.
 * @param orders Current waiting-order list.
 * @param customerId Runtime ID of the customer to search for.
 * @return Matching order index, or `-1` when not found.
 */
int OrderUILogic::FindOrderIndexByCustomer(const std::vector<WaitingOrder>& orders, int customerId) const {
	for (int i = 0; i < (int)orders.size(); ++i) {
		if (orders[i].customerId == customerId)
			return i;
	}
	return -1;
}

/**
 * @brief Collects every active waiting-for-food customer order from the scene.
 * @param scene Active scene containing customer tables and NPCs.
 * @param outOrders Output list of waiting orders, sorted by urgency.
 */
void OrderUILogic::CollectWaitingOrders(Scene& scene, std::vector<WaitingOrder>& outOrders) {
	outOrders.clear();

	LogicManager& logicMgr = scene.GetLogicManager();

	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		if (!obj) continue;
		const int tableId = obj->GetID();

		auto* table = logicMgr.GetLogicForObject<CustomerTableLogic>(tableId);
		if (!table) continue;
		if (!table->HasSeatedCustomer()) continue;

		for (int custId : table->GetSeatedCustomerIDs()) {
			if (custId < 0) continue;

			auto* npc = logicMgr.GetLogicForObject<SimpleNpcLogic>(custId);
			if (!npc) continue;

			if (npc->IsWaitingForFood() && npc->HasOrderBeenTaken() && !npc->HasDishServed()) {
				outOrders.push_back({ tableId, custId, npc->GetDesiredDishType(),npc->GetPatienceRemaining(),npc->GetPatienceRatio01() });
			}
		}
	}

	std::sort(outOrders.begin(), outOrders.end(),
		[](const WaitingOrder& a, const WaitingOrder& b) {
			if (a.patienceRemaining != b.patienceRemaining)
				return a.patienceRemaining < b.patienceRemaining; // lowest time first
			if (a.tableId != b.tableId)
				return a.tableId < b.tableId;
			return a.customerId < b.customerId;
		});
}

/**
 * @brief Returns the ingredient and station icon paths needed to display a recipe.
 * @param dish Dish type whose recipe should be expanded.
 * @param outIngredientTex Output ingredient icon paths.
 * @param outStationTex Output station icon paths.
 */
void OrderUILogic::GetRecipeIconPaths(DishType dish,
	std::vector<const char*>& outIngredientTex,
	std::vector<const char*>& outStationTex) const {
	outIngredientTex.clear();
	outStationTex.clear();

	// Must return up to 2 ingredients and up to 2 stations.

	switch (dish) {
	case DishType::VegDish:
		outIngredientTex.push_back(IngredientBoxIconPath(IngredientType::Vegetable));
		outIngredientTex.push_back(IngredientBoxIconPath(IngredientType::Vegetable));
		outStationTex.push_back("../assets/Stations/Cutting_Board.png");
		outStationTex.push_back("../assets/Stations/Cutting_Board.png");
		break;

	case DishType::MeatDish:
		outIngredientTex.push_back(IngredientBoxIconPath(IngredientType::Meat));
		outIngredientTex.push_back(IngredientBoxIconPath(IngredientType::Vegetable));
		outStationTex.push_back("../assets/Stations/Grills_2.png");
		outStationTex.push_back("../assets/Stations/Cutting_Board.png");
		break;

	case DishType::SoupDish:
		outIngredientTex.push_back(IngredientBoxIconPath(IngredientType::Meat));
		outIngredientTex.push_back(IngredientBoxIconPath(IngredientType::Shroom));
		outStationTex.push_back("../assets/Stations/Grills_2.png");
		outStationTex.push_back("../assets/Stations/Stove_2.png");
		break;

	case DishType::SkewerDish:
		outIngredientTex.push_back(IngredientBoxIconPath(IngredientType::Meat));
		outIngredientTex.push_back(IngredientBoxIconPath(IngredientType::Carrot));
		outStationTex.push_back("../assets/Stations/Grills_2.png");
		outStationTex.push_back("../assets/Stations/Stove_2.png");
		break;

	case DishType::CarrotSaladDish:
		outIngredientTex.push_back(IngredientBoxIconPath(IngredientType::Vegetable));
		outIngredientTex.push_back(IngredientBoxIconPath(IngredientType::Carrot));
		outStationTex.push_back("../assets/Stations/Cutting_Board.png");
		outStationTex.push_back("../assets/Stations/Stove_2.png");
		break;
	default:
		// no recipe -> show nothing
		break;
	}
}

/**
 * @brief Updates the recipe icon textures for a slot to match a dish type.
 * @param scene Active scene containing the slot's icon objects.
 * @param slot Slot record whose recipe icons should change.
 * @param dish Dish type whose recipe should be displayed.
 */
void OrderUILogic::UpdateRecipeIcons(Scene& scene, OrderSlot& slot, DishType dish) {
	std::vector<const char*> ing;
	std::vector<const char*> st;
	GetRecipeIconPaths(dish, ing, st);

	for (int i = 0; i < kRecipeCols; ++i) {
		const char* ingTex = (i < (int)ing.size()) ? ing[i] : invisTex_;
		const char* stTex = (i < (int)st.size()) ? st[i] : invisTex_;

		if (slot.ingredientIconIds[i] >= 0)
			SetIconTexture(scene, slot.ingredientIconIds[i], ingTex);

		if (slot.stationIconIds[i] >= 0)
			SetIconTexture(scene, slot.stationIconIds[i], stTex);
	}
}

/**
 * @brief Rebuilds and animates the order-ticket HUD for one frame.
 * @param dt Delta time for the frame.
 * @param scene Active scene containing the order UI, tables, and customers.
 * @param input Unused input manager forwarded by the logic system.
 */
void OrderUILogic::Update(float dt, Scene& scene, InputManager& /*input*/) {
	std::vector<WaitingOrder> orders;
	orders.reserve(kMaxOrders);
	CollectWaitingOrders(scene, orders);

	std::vector<bool> used(orders.size(), false);
	sCustomerPanelCenters_.clear();

	// consume pending stamp requests
	if (!sPendingStampCustomers_.empty()) {
		for (auto& slot : slots_) {
			if (slot.customerId >= 0 &&
				sPendingStampCustomers_.find(slot.customerId) != sPendingStampCustomers_.end()) {
				// Start the stamp only for panels that are still alive and mapped to the requested customer.
				StartCompletionStamp(scene, slot);
			}
		}
		sPendingStampCustomers_.clear();
	}

	bool anySlotCleared = false;

	// keep/update existing slots
	for (int s = 0; s < (int)slots_.size(); ++s) {
		OrderSlot& slot = slots_[s];

		if (slot.completing) {
			if (UpdateCompleteAnimation(scene, slot, dt)) {
				ClearSlot(scene, slot);
				anySlotCleared = true;
			}
			continue;
		}

		if (slot.customerId < 0) {
			continue;
		}

		const int idx = FindOrderIndexByCustomer(orders, slot.customerId);
		if (idx < 0) {
			BeginCompleteAnimation(scene, s, slot);
			if (UpdateCompleteAnimation(scene, slot, dt)) {
				ClearSlot(scene, slot);
				anySlotCleared = true;
			}
			continue;
		}

		used[idx] = true;
		slot.tableId = orders[idx].tableId;

		EnsurePanel(scene, s, slot);
		EnsureDishIcon(scene, slot);
		EnsureRecipeIcons(scene, slot);

		if (!slot.hasLastDish || slot.lastDish != orders[idx].dish) {
			SetIconTexture(scene, slot.dishIconId, DishToIconPath(orders[idx].dish));
			UpdateRecipeIcons(scene, slot, orders[idx].dish);

			slot.lastDish = orders[idx].dish;
			slot.hasLastDish = true;
		}

		UpdateLivePanelLayout(scene, s, slot, dt);
	}

	// only compact after a completed card is fully gone
	if (anySlotCleared) {
		CompactSlotsLeft();
	}

	// fill empty slots with new waiting customers
	for (int i = 0; i < (int)orders.size(); ++i) {
		if (used[i]) continue;

		int emptySlot = -1;
		for (int s = 0; s < (int)slots_.size(); ++s) {
			if (!slots_[s].completing && slots_[s].customerId < 0) {
				emptySlot = s;
				break;
			}
		}

		if (emptySlot < 0) {
			break;
		}

		OrderSlot& slot = slots_[emptySlot];

		slot.customerId = orders[i].customerId;
		slot.tableId = orders[i].tableId;
		slot.lastDish = DishType::PoopDish;
		slot.hasLastDish = false;
		slot.panelSpawned = false;

		slot.completing = false;
		slot.completionTimer = 0.0f;
		slot.completionStartPanelPos = { 0.0f, 0.0f };

		DespawnIfAlive(scene, slot.stampId);
		slot.stampActive = false;
		slot.stampTimer = 0.0f;

		slot.panelId = -1;
		slot.dishIconId = -1;
		slot.ingredientIconIds.assign(kRecipeCols, -1);
		slot.stationIconIds.assign(kRecipeCols, -1);

		// Spawn and populate the newly occupied slot in one pass so the ticket appears fully formed.
		EnsurePanel(scene, emptySlot, slot);
		EnsureDishIcon(scene, slot);
		EnsureRecipeIcons(scene, slot);

		SetIconTexture(scene, slot.dishIconId, DishToIconPath(orders[i].dish));
		UpdateRecipeIcons(scene, slot, orders[i].dish);

		slot.lastDish = orders[i].dish;
		slot.hasLastDish = true;

		UpdateLivePanelLayout(scene, emptySlot, slot, dt);
	}
}
