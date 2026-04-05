/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         PlateLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (75%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (25%)

 DESCRIPTION:		Implements PlateLogic, which manages the assembly of processed
					ingredients into completed dishes. Handles ingredient validation,
					dish-recipe matching, storing ingredient types, and determining
					the final dish output (VegDish, MeatDish, SoupDish, etc.).

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineCore/Logger.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/PlateLogic.hpp"

 /**
  * @brief Returns the texture path that corresponds to a prepared dish type.
  * @param t Prepared dish type to display.
  * @return Texture path for the requested dish.
  */
static const char* GetDishTexturePath(DishType t) {
	// Centralize dish-to-texture mapping so visuals stay consistent across all plate flows.
	switch (t) {
	case DishType::VegDish:  return "../assets/Food/Food_Salad.png";
	case DishType::MeatDish: return "../assets/Food/Food_Meat.png";
	case DishType::SoupDish: return "../assets/Food/Food_Mushroom.png";
	case DishType::SkewerDish: return "../assets/Food/Food_Meat_n_carrot.png";
	case DishType::CarrotSaladDish: return "../assets/Food/Food_Salad_n_carrot.png";
	case DishType::PoopDish: return "../assets/Food/poop.png";
	default:                return "../assets/Food/Food_Salad.png";
	}
}

/**
 * @brief Applies the correct dish texture to the plate after assembly.
 * @param scene Active scene containing the plate object.
 */
void PlateLogic::ApplyDishVisual(Scene& scene) {
	// Resolve the live plate object before attempting to update its sprite.
	GameObject* plateObj = scene.GetGameObjectByID(GetOwnerID());
	if (!plateObj) return;

	const char* texPath = GetDishTexturePath(dishType_);

	// Swap the runtime texture and keep the serialized defaults synchronized.
	plateObj->SetTexture(ResourceManager::Instance().LoadTexture(texPath, texPath));
	scene.SetObjectTexturePath(GetOwnerID(), texPath);

	// Keep Defaults in sync too so later reloads and editor queries see the same dish texture.
	Scene::Defaults d = scene.GetDefaults(GetOwnerID());
	d.texture = texPath;
	scene.SetDefaults(GetOwnerID(), d);
}

/**
 * @brief Constructs plate logic for a spawned plate object.
 * @param ownerID Runtime ID of the GameObject that owns this logic.
 */
PlateLogic::PlateLogic(int ownerID)
	: GameObjectLogic(ownerID), dishPrepared_(false), dishType_(DishType::PoopDish), firstIngredientObjectID_(-1) {
	// Start the plate empty and unassembled until ingredients are added.
}

/**
 * @brief Initializes the plate's runtime state after scene creation.
 * @param scene Unused active scene reference.
 */
void PlateLogic::Start(Scene& /*scene*/) {
	// Reset all logical recipe state so reused plate objects begin from a clean slate.
	ingredients_.clear();
	dishPrepared_ = false;
	dishType_ = DishType::PoopDish;

	TS_LOG_DEBUG("[PlateLogic] Start owner=" << GetOwnerID());
}

/**
 * @brief Updates the plate for one frame.
 * @param dt Unused delta time for the frame.
 * @param scene Unused active scene reference.
 * @param input Unused input manager reference.
 */
void PlateLogic::Update(float /*dt*/, Scene& /*scene*/, InputManager& /*input*/) {
	// Plate behavior is currently event-driven, so no per-frame work is required here.
}

/**
 * @brief Cleans up any child visuals owned by this plate before destruction.
 * @param scene Active scene containing the plate object.
 */
void PlateLogic::OnDestroy(Scene& scene) {
	// Despawn the tracked ingredient visual as well so no orphan child object is left behind.
	if (firstIngredientObjectID_ >= 0) {
		if (scene.GetGameObjectByID(firstIngredientObjectID_)) {
			scene.DespawnByID(firstIngredientObjectID_);
		}
		firstIngredientObjectID_ = -1;
	}
}

/**
 * @brief Returns whether the plate can accept the supplied ingredient type.
 * @param type Ingredient type being considered for placement.
 * @return True if the ingredient type is valid and the plate still has room.
 */
bool PlateLogic::CanAcceptIngredientType(IngredientType type) const {
	// Finished dishes no longer accept additional ingredients.
	if (dishPrepared_)
		return false;

	// Only refined ingredients are valid inputs for recipe assembly.
	switch (type) {
	case IngredientType::Refined_Veg:
	case IngredientType::Refined_Meat:
	case IngredientType::Refined_Shroom:
	case IngredientType::Refined_Carrot:
		break;
	default:
		return false;
	}

	// Cap the logical recipe to two ingredients to match the authored dish rules.
	if (ingredients_.size() >= 2)
		return false;

	return true;
}

/**
 * @brief Adds an ingredient type to the plate's logical ingredient list.
 * @param type Ingredient type to append.
 */
void PlateLogic::AddIngredientType(IngredientType type) {
	// Record the ingredient type exactly as supplied; validation happens before this call.
	ingredients_.push_back(type);
}

/**
 * @brief Clears all logical ingredients from the plate.
 */
void PlateLogic::ClearIngredients() {
	// Drop both the logical recipe state and the tracked ingredient visual reference.
	ingredients_.clear();
	firstIngredientObjectID_ = -1;
}

/**
 * @brief Attempts to assemble a finished dish from the currently stored ingredients.
 * @param outDishType Output dish type produced by the attempted recipe.
 * @param outConsumedIngredients Output list populated with the ingredients that formed the recipe.
 * @return True when the plate successfully assembles into a dish.
 */
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

	// Use the first two logical ingredients to compute the authored two-item recipe result.
	IngredientType a = ingredients_[0];
	IngredientType b = ingredients_[1];

	DishType result = ComputeDishFromPair(a, b);

	// Commit the assembled dish into the plate's internal state.
	dishPrepared_ = true;
	dishType_ = result;

	// Return both the resulting dish and the consumed logical ingredients to the caller.
	outDishType = result;
	outConsumedIngredients = ingredients_;

	return true;
}

/**
 * @brief Clears the prepared-dish state and resets the plate back to empty.
 */
void PlateLogic::ClearPreparedDish() {
	// Reset both the assembled dish and the ingredient list so the plate can be reused.
	dishPrepared_ = false;
	dishType_ = DishType::PoopDish;
	ingredients_.clear();
	firstIngredientObjectID_ = -1;
}

/**
 * @brief Computes the resulting dish for a pair of refined ingredients.
 * @param a First refined ingredient type.
 * @param b Second refined ingredient type.
 * @return Dish type produced by that ingredient combination.
 */
DishType PlateLogic::ComputeDishFromPair(IngredientType a, IngredientType b) const {
	const bool aMeat = (a == IngredientType::Refined_Meat);
	const bool bMeat = (b == IngredientType::Refined_Meat);
	const bool aVeg = (a == IngredientType::Refined_Veg);
	const bool bVeg = (b == IngredientType::Refined_Veg);
	const bool aShroom = (a == IngredientType::Refined_Shroom);
	const bool bShroom = (b == IngredientType::Refined_Shroom);
	const bool aCarrot = (a == IngredientType::Refined_Carrot);
	const bool bCarrot = (b == IngredientType::Refined_Carrot);

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

	// Meat + Carrot (any order) => SkewerDish
	if ((aMeat && bCarrot) || (aCarrot && bMeat)) {
		return DishType::SkewerDish;
	}

	// Veg + Carrot (any order) => CarrotSaladDish
	if ((aVeg && bCarrot) || (aCarrot && bVeg)) {
		return DishType::CarrotSaladDish;
	}

	// Anything else => PoopDish
	return DishType::PoopDish;
}

/**
 * @brief Attempts to add a specific ingredient object to this plate.
 * @param ingredient Ingredient logic representing the candidate ingredient object.
 * @param outConsumedNow Output flag indicating whether the ingredient was consumed immediately.
 * @return True if the ingredient was accepted onto the plate.
 */
bool PlateLogic::TryAddIngredient(const IngredientLogic& ingredient, bool& outConsumedNow) {
	// Plate assembly does not consume the ingredient object immediately in the current design.
	outConsumedNow = false;

	// Reject raw ingredients until they have been processed by the correct station.
	if (!ingredient.IsProcessed())
		return false;

	IngredientType type = ingredient.GetType();

	// Reuse the shared logical acceptance checks before storing the ingredient type.
	if (!CanAcceptIngredientType(type))
		return false;

	// Record only the logical ingredient type; ownership and visuals stay elsewhere.
	AddIngredientType(type);

	// The caller remains responsible for the ingredient object's visual and lifetime.
	return true;
}
