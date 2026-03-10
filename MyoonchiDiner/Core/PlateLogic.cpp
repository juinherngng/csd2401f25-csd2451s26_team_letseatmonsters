/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         PlateLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:		Implements PlateLogic, which manages the assembly of processed
					ingredients into completed dishes. Handles ingredient validation,
					dish-recipe matching, storing ingredient types, and determining
					the final dish output (VegDish, MeatDish, SoupDish, etc.).

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/PlateLogic.hpp"
#include "../Graphics/SceneManager.hpp"

#include "PlateLogic.hpp"

static const char* GetDishTexturePath(DishType t) {
	switch (t) {
	case DishType::VegDish:  return "../assets/Salad.png";
	case DishType::MeatDish: return "../assets/Meat.png";
	case DishType::SoupDish: return "../assets/Soup.png";
	case DishType::PoopDish: return "../assets/PoopDish.png";
	default:                return "../assets/PoopDish.png";
	}
}

void PlateLogic::ApplyDishVisual(Scene& scene) {
	GameObject* plateObj = scene.GetGameObjectByID(GetOwnerID());
	if (!plateObj) return;

	const char* texPath = GetDishTexturePath(dishType_);

	plateObj->SetTexture(ResourceManager::Instance().LoadTexture(texPath, texPath));
	scene.SetObjectTexturePath(GetOwnerID(), texPath);

	// Keep Defaults in sync too (important in your engine)
	Scene::Defaults d = scene.GetDefaults(GetOwnerID());
	d.texture = texPath;
	scene.SetDefaults(GetOwnerID(), d);
}



PlateLogic::PlateLogic(int ownerID) : GameObjectLogic(ownerID), dishPrepared_(false), dishType_(DishType::PoopDish), firstIngredientObjectID_(-1) // default
{
}

void PlateLogic::Start(Scene& /*scene*/) {
	ingredients_.clear();
	dishPrepared_ = false;
	dishType_ = DishType::PoopDish;

	std::cout << "[PlateLogic] Start owner=" << GetOwnerID() << "\n";
}

void PlateLogic::Update(float /*dt*/, Scene& /*scene*/, InputManager& /*input*/) {
	// No per-frame logic needed yet.
}

void PlateLogic::OnDestroy(Scene& scene)
{
	// If this plate had an attached ingredient visual, delete it too.
	if (firstIngredientObjectID_ >= 0)
	{
		if (scene.GetGameObjectByID(firstIngredientObjectID_))
		{
			scene.DespawnByID(firstIngredientObjectID_);
		}
		firstIngredientObjectID_ = -1;
	}
}

bool PlateLogic::CanAcceptIngredientType(IngredientType type) const
{
	if (dishPrepared_)
		return false;

	// Only allow refined ingredients
	switch (type) {
	case IngredientType::Refined_Veg:
	case IngredientType::Refined_Meat:
	case IngredientType::Refined_Shroom:
		break;
	default:
		return false;
	}

	// Limit to two ingredients (like your Unity script).
	if (ingredients_.size() >= 2)
		return false;

	return true;
}

void PlateLogic::AddIngredientType(IngredientType type) {
	ingredients_.push_back(type);
}

void PlateLogic::ClearIngredients() {
	ingredients_.clear();
	firstIngredientObjectID_ = -1;
}

// ----- Dish assembly -----

bool PlateLogic::TryAssembleDish(DishType& outDishType,
	std::vector<IngredientType>& outConsumedIngredients) {
	if (dishPrepared_) {
		// Already have a dish; do not assemble again.
		return false;
	}

	if (ingredients_.size() < 2) {
		// Need at least two ingredients to assemble a dish.
		return false;
	}

	// For simplicity, we only look at the first two ingredients,
	// mirroring your Unity example (ingredient1 and ingredient2).
	IngredientType a = ingredients_[0];
	IngredientType b = ingredients_[1];

	DishType result = ComputeDishFromPair(a, b);

	// Mark internal state
	dishPrepared_ = true;
	dishType_ = result;

	// Report results to caller
	outDishType = result;
	outConsumedIngredients = ingredients_;

	return true;
}

void PlateLogic::ClearPreparedDish() {
	dishPrepared_ = false;
	dishType_ = DishType::PoopDish;
	ingredients_.clear();
	firstIngredientObjectID_ = -1;
}

// Helper: map two refined ingredient types to a dish type.
DishType PlateLogic::ComputeDishFromPair(IngredientType a, IngredientType b) const {
	const bool aMeat = (a == IngredientType::Refined_Meat);
	const bool bMeat = (b == IngredientType::Refined_Meat);
	const bool aVeg = (a == IngredientType::Refined_Veg);
	const bool bVeg = (b == IngredientType::Refined_Veg);
	const bool aShroom = (a == IngredientType::Refined_Shroom);
	const bool bShroom = (b == IngredientType::Refined_Shroom);

	// Meat + Veg (any order) => MeatDish
	if ((aMeat && bVeg) || (aVeg && bMeat)) {
		return DishType::MeatDish;
	}

	// Shroom + Meat (any order) => SoupDish
	if ((aShroom && bMeat) || (aMeat && bShroom)) {
		return DishType::SoupDish;
	}

	// Veg + Veg => VegDish
	if ((aVeg && bVeg)) {
		return DishType::VegDish;
	}

	// Anything else => PoopDish
	return DishType::PoopDish;
}

bool PlateLogic::TryAddIngredient(const IngredientLogic& ingredient, bool& outConsumedNow) {
	// Do NOT consume the ingredient's GameObject here.
	outConsumedNow = false;

	// Only allow processed (refined) ingredients on the plate.
	if (!ingredient.IsProcessed())
		return false;

	IngredientType type = ingredient.GetType();

	if (!CanAcceptIngredientType(type))
		return false;

	// Just store the logical type
	AddIngredientType(type);

	// We keep the GameObject alive; visual + destruction are handled elsewhere.
	return true;
}

