/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         OrderUILogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Defines the OrderUILogic component, responsible for collecting
					active customer orders and displaying them as animated order
					tickets with dish, ingredient, and station icons.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/CustomerTableLogic.hpp"
#include "../Core/LogicManager.hpp"
#include "../Core/OrderUILogic.hpp"
#include "../Core/SimpleNpcLogic.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"

#include <algorithm>

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

	for (auto& slot : slots_) {
		slot.ingredientIconIds.assign(kRecipeCols, -1);
		slot.stationIconIds.assign(kRecipeCols, -1);
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

	slot.customerId = -1;
	slot.panelSpawned = false;
	slot.lastDish = DishType::PoopDish;
	slot.hasLastDish = false;

	// Keep vectors sized correctly
	if ((int)slot.ingredientIconIds.size() != kRecipeCols)
		slot.ingredientIconIds.assign(kRecipeCols, -1);
	if ((int)slot.stationIconIds.size() != kRecipeCols)
		slot.stationIconIds.assign(kRecipeCols, -1);
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

		const int custId = table->GetSeatedCustomerID();
		auto* npc = logicMgr.GetLogicForObject<SimpleNpcLogic>(custId);
		if (!npc) continue;

		if (npc->IsWaitingForFood() && npc->HasOrderBeenTaken() && !npc->HasDishServed()) {
			outOrders.push_back({ tableId, custId, npc->GetDesiredDishType() });
		}
	}

	std::sort(outOrders.begin(), outOrders.end(),
		[](const WaitingOrder& a, const WaitingOrder& b) { return a.tableId < b.tableId; });
}

// ------------------------------------------------------------
// Recipe Mapping (EDIT THESE PATHS TO YOUR REAL ASSETS)
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

void OrderUILogic::Update(float /*dt*/, Scene& scene, InputManager& /*input*/) {
	std::vector<WaitingOrder> orders;
	orders.reserve(kMaxOrders);
	CollectWaitingOrders(scene, orders);

	std::vector<bool> used(orders.size(), false);

	// 1) Keep existing customers in their slots if they’re still waiting
	for (int s = 0; s < (int)slots_.size(); ++s) {
		OrderSlot& slot = slots_[s];
		if (slot.customerId < 0) continue;

		const int idx = FindOrderIndexByCustomer(orders, slot.customerId);
		if (idx < 0) {
			ClearSlot(scene, slot);
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

		FollowPanel(scene, slot);
	}

	// 2) Fill empty slots with new waiting customers (up to 4 total)
	for (int i = 0; i < (int)orders.size(); ++i) {
		if (used[i]) continue;

		int emptySlot = -1;
		for (int s = 0; s < (int)slots_.size(); ++s) {
			if (slots_[s].customerId < 0) {
				emptySlot = s;
				break;
			}
		}

		if (emptySlot < 0)
			break;

		OrderSlot& slot = slots_[emptySlot];

		slot.customerId = orders[i].customerId;
		slot.lastDish = DishType::PoopDish;
		slot.hasLastDish = false;
		slot.panelSpawned = false;

		// reset ids to force respawn (optional but safe)
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

		FollowPanel(scene, slot);
	}
}
