/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         OrderUILogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Defines the OrderUILogic component, responsible for collecting
					active customer orders and displaying them as animated order
					tickets with dish, ingredient, and station icons.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "Core/CustomerTableLogic.hpp"
#include "Core/LogicManager.hpp"
#include "Core/OrderUILogic.hpp"
#include "Core/SimpleNpcLogic.hpp"
#include "Graphics/GameObject.hpp"
#include "Graphics/ResourceManager.hpp"
#include "Graphics/SceneManager.hpp"

#include <algorithm>

std::unordered_map<int, glm::vec2> OrderUILogic::sCustomerPanelCenters_{};
std::unordered_set<int> OrderUILogic::sPendingStampCustomers_{};

bool OrderUILogic::TryGetPanelCenterForCustomer(int customerId, glm::vec2& outPos) {
	auto it = sCustomerPanelCenters_.find(customerId);
	if (it == sCustomerPanelCenters_.end()) {
		return false;
	}
	outPos = it->second;
	return true;
}

void OrderUILogic::RequestCompletionStampForCustomer(int customerId) {
	if (customerId >= 0) {
		sPendingStampCustomers_.insert(customerId);
	}
}

static float Clamp01(float v) {
	if (v < 0.f) return 0.f;
	if (v > 1.f) return 1.f;
	return v;
}

static float EaseOutCubic(float x) {
	x = Clamp01(x);
	float inv = 1.0f - x;
	return 1.0f - inv * inv * inv;
}

static float EaseInCubic(float x) {
	x = Clamp01(x);
	return x * x * x;
}

static float EaseOutBack(float x) {
	x = Clamp01(x);
	constexpr float c1 = 1.70158f;
	constexpr float c3 = c1 + 1.0f;
	float y = x - 1.0f;
	return 1.0f + c3 * y * y * y + c1 * y * y;
}

static void DespawnIfAlive(Scene& scene, int& id) {
	if (id >= 0) {
		scene.DespawnByID(id); id = -1;
	}
}

const char* OrderUILogic::DishToIconPath(DishType t) const {
	switch (t) {
	case DishType::VegDish:  return "../assets/Salad.png";
	case DishType::MeatDish: return "../assets/Meat.png";
	case DishType::SoupDish: return "../assets/Soup.png";
	case DishType::SkewerDish: return "../assets/Food_Meat_n_carrot.png";
	case DishType::CarrotSaladDish: return "../assets/Food_Salad_n_carrot.png";
	case DishType::PoopDish: return "../assets/PoopDish.png";
	default:                 return "../assets/Salad.png";
	}
}

void OrderUILogic::Start(Scene& /*scene*/) {
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
}

void OrderUILogic::OnDestroy(Scene& scene) {
	for (auto& slot : slots_)
		ClearSlot(scene, slot);
}

glm::vec2 OrderUILogic::SlotTargetPos(int slotIndex) const {
	// NOTE: you used panelSize_.y for x spacing before.
	// Usually you want panelSize_.x for horizontal spacing.
	return glm::vec2(
		panelTargetPos_.x + slotIndex * (panelSize_.x + ticketGapY_),
		panelTargetPos_.y
	);
}

void OrderUILogic::ClearSlot(Scene& scene, OrderSlot& slot) {
	DespawnIfAlive(scene, slot.dishIconId);

	for (int& id : slot.ingredientIconIds) DespawnIfAlive(scene, id);
	for (int& id : slot.stationIconIds)    DespawnIfAlive(scene, id);

	DespawnIfAlive(scene, slot.panelId);
	DespawnIfAlive(scene, slot.stampId);

	slot.customerId = -1;
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

	if (slot.customerId >= 0) {
		sCustomerPanelCenters_[slot.customerId] = glm::vec2(cur.x, cur.y);
	}

	UpdateCompletionStamp(scene, slot, dt, glm::vec2(cur.x, cur.y), 1.0f);
}

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
	applySprite(slot.dishIconId, dishOffset_, dishSize_);

	for (int i = 0; i < kRecipeCols; ++i) {
		if (i < (int)ingredientOffsets_.size()) {
			applySprite(slot.ingredientIconIds[i], ingredientOffsets_[i], ingredientSize_);
		}
		if (i < (int)stationOffsets_.size()) {
			applySprite(slot.stationIconIds[i], stationOffsets_[i], stationSize_);
		}
	}
}

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

void OrderUILogic::EnsurePanel(Scene& scene, int slotIndex, OrderSlot& slot) {
	if (slot.panelSpawned && slot.panelId >= 0 && scene.GetGameObjectByID(slot.panelId))
		return;

	slot.panelId = scene.TriggerOrderUiSlideIn(
		SlotTargetPos(slotIndex),
		panelSize_,
		panelLayer_,
		panelTex_,
		slideDuration_
	);

	slot.panelSpawned = (slot.panelId >= 0);
}

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

void OrderUILogic::EnsureDishIcon(Scene& scene, OrderSlot& slot) {
	EnsureSubSprite(scene, slot.panelId, slot.dishIconId, dishOffset_, dishSize_, dishLayer_);
}

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

	move(slot.dishIconId, dishOffset_);

	for (int i = 0; i < kRecipeCols; ++i) {
		if (i < (int)ingredientOffsets_.size())
			move(slot.ingredientIconIds[i], ingredientOffsets_[i]);

		if (i < (int)stationOffsets_.size())
			move(slot.stationIconIds[i], stationOffsets_[i]);
	}
}

void OrderUILogic::SetIconTexture(Scene& scene, int iconId, const char* texPath) {
	GameObject* icon = scene.GetGameObjectByID(iconId);
	if (!icon) return;

	icon->SetTexture(ResourceManager::Instance().LoadTexture(texPath, texPath));
	scene.SetObjectTexturePath(iconId, texPath);

	Scene::Defaults d = scene.GetDefaults(iconId);
	d.texture = texPath;
	scene.SetDefaults(iconId, d);
}

int OrderUILogic::FindOrderIndexByCustomer(const std::vector<WaitingOrder>& orders, int customerId) const {
	for (int i = 0; i < (int)orders.size(); ++i) {
		if (orders[i].customerId == customerId)
			return i;
	}
	return -1;
}

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

// ------------------------------------------------------------
// Recipe Mapping
// ------------------------------------------------------------
void OrderUILogic::GetRecipeIconPaths(DishType dish,
	std::vector<const char*>& outIngredientTex,
	std::vector<const char*>& outStationTex) const {
	outIngredientTex.clear();
	outStationTex.clear();

	// Must return up to 2 ingredients and up to 2 stations.

	switch (dish) {
	case DishType::VegDish:
		outIngredientTex.push_back("../assets/Cabbage_Ingredient.png");
		outIngredientTex.push_back("../assets/Cabbage_Ingredient.png");
		outStationTex.push_back("../assets/Cutting_Board.png");
		outStationTex.push_back("../assets/Cutting_Board.png");
		break;

	case DishType::MeatDish:
		outIngredientTex.push_back("../assets/Meat_Ingredient.png");
		outIngredientTex.push_back("../assets/Cabbage_Ingredient.png");
		outStationTex.push_back("../assets/Grills_2.png");
		outStationTex.push_back("../assets/Cutting_Board.png");
		break;

	case DishType::SoupDish:
		outIngredientTex.push_back("../assets/Meat_Ingredient.png");
		outIngredientTex.push_back("../assets/Mushroom_Ingredient.png");
		outStationTex.push_back("../assets/Grills_2.png");
		outStationTex.push_back("../assets/Stove_2.png");
		break;

	case DishType::SkewerDish:
		outIngredientTex.push_back("../assets/Meat_Ingredient.png");
		outIngredientTex.push_back("../assets/Ingredient_Carrot.png");
		outStationTex.push_back("../assets/Grills_2.png");
		outStationTex.push_back("../assets/Stove_2.png");
		break;

	case DishType::CarrotSaladDish:
		outIngredientTex.push_back("../assets/Cabbage_Ingredient.png");
		outIngredientTex.push_back("../assets/Ingredient_Carrot.png");
		outStationTex.push_back("../assets/Cutting_Board.png");
		outStationTex.push_back("../assets/Stove_2.png");
		break;
	default:
		// no recipe -> show nothing
		break;
	}
}

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

