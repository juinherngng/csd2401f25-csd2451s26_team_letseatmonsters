/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         PlateLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (75%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu   (25%)

 DESCRIPTION:       Implements PlateLogic, which manages the assembly of processed
					ingredients into completed dishes. Handles ingredient validation,
					dish-recipe matching, storing ingredient types, and determining
					the final dish output (VegDish, MeatDish, SoupDish, etc.).

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "Core/FoodTypes.hpp"
#include "Core/GameObjectLogic.hpp"
#include "Core/IngredientLogic.hpp"

#include <vector>

 // PlateLogic
 //  - Represents a plate that can accept refined ingredients and assemble them into a dish.
 //  - Works on logical types (IngredientType, DishType) rather than directly spawning objects.
 //  - External systems (PlayerLogic, TableLogic, Factory) can:
 //      * Call CanAcceptIngredientType / AddIngredientType when the player drops an ingredient.
 //      * Call TryAssembleDish to compute the resulting DishType and know which ingredients are consumed.
 //      * Use GetDishType / HasPreparedDish to query state when serving customers.
class PlateLogic : public GameObjectLogic {
public:
	/**
	 * @brief Constructs a `PlateLogic` instance.
	 * @param ownerID Parameter for owner id.
	 * @return Result produced by this operation.
	 */
	explicit PlateLogic(int ownerID);

	/**
	 * @brief Performs start.
	 * @param scene Scene being processed.
	 */
	void Start(Scene& scene) override;

	/**
	 * @brief Updates this object.
	 * @param dt Frame delta time in seconds.
	 * @param scene Scene being processed.
	 * @param input Input manager for the current frame.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Performs on destroy.
	 * @param scene Scene being processed.
	 */
	void OnDestroy(Scene& scene) override;

	// ----- Ingredient placement -----

	/**
	 * @brief Returns whether accept ingredient type.
	 * @param type Parameter for type.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool CanAcceptIngredientType(IngredientType type) const;

	/**
	 * @brief Adds ingredient type.
	 * @param type Parameter for type.
	 */
	void AddIngredientType(IngredientType type);

	/**
	 * @brief Returns ingredients.
	 * @return Requested value.
	 */
	const std::vector<IngredientType>& GetIngredients() const {
		return ingredients_;
	}

	/**
	 * @brief Returns whether any ingredient.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasAnyIngredient() const {
		return !ingredients_.empty();
	}

	/**
	 * @brief Returns whether two ingredients.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasTwoIngredients() const {
		return ingredients_.size() >= 2;
	}

	/**
	 * @brief Clears ingredients.
	 */
	void ClearIngredients();

	// ----- Dish assembly -----

	/**
	 * @brief Returns whether prepared dish.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasPreparedDish() const {
		return dishPrepared_;
	}

	/**
	 * @brief Returns dish type.
	 * @return Requested value.
	 */
	DishType GetDishType() const {
		return dishType_;
	}

	// Attempt to assemble a dish from the currently placed ingredients.
	//
	// Rules:
	//   - Require at least two refined ingredients.
	//   - Combinations:
	//       Refined_Meat + Refined_Veg  => MeatDish
	//       Refined_Shroom + Refined_Meat => SoupDish
	//       Refined_Veg + Refined_Veg  => VegDish
	//       Anything else              => PoopDish
	//
	// On success:
	//   - Sets dishPrepared_ = true and dishType_ accordingly.
	//   - Fills outConsumedIngredients with *all* currently stored ingredients
	//     (caller can use that info to despawn ingredient objects if desired).
	//   - Returns true.
	//
	// On failure: returns false, does not change internal state.

	/**
	 * @brief Attempts to assemble dish.
	 * @param outDishType Output value for out dish type.
	 * @param outConsumedIngredients Output value for out consumed ingredients.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool TryAssembleDish(DishType& outDishType,
		std::vector<IngredientType>& outConsumedIngredients);

	/**
	 * @brief Clears prepared dish.
	 */
	void ClearPreparedDish();

	// High-level helper: try to add a specific ingredient object to this plate.
	//
	// This uses IngredientLogic to:
	//   - Check if it is processed (refined).
	//   - Get its IngredientType.
	//   - Call CanAcceptIngredientType / AddIngredientType.
	//
	// outConsumedNow:
	//   - For now, this will always be false; we keep assembly as a separate step.
	//   - In the future, you could set this true if adding this ingredient directly
	//     completes and "consumes" it into a dish.
	//
	// Returns true if the ingredient type was added to the plate successfully.

	/**
	 * @brief Attempts to add ingredient.
	 * @param ingredient Parameter for ingredient.
	 * @param outConsumedNow Output value for out consumed now.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool TryAddIngredient(const IngredientLogic& ingredient, bool& outConsumedNow);

	/**
	 * @brief Returns ingredient count.
	 * @return Requested value.
	 */
	int GetIngredientCount() const {
		return static_cast<int>(ingredients_.size());
	}

	/**
	 * @brief Sets first ingredient object id.
	 * @param id Parameter for id.
	 */
	void SetFirstIngredientObjectID(int id) {
		firstIngredientObjectID_ = id;
	}

	/**
	 * @brief Returns first ingredient object id.
	 * @return Requested value.
	 */
	int  GetFirstIngredientObjectID() const {
		return firstIngredientObjectID_;
	}

	/**
	 * @brief Applies dish visual.
	 * @param scene Scene being processed.
	 */
	void ApplyDishVisual(Scene& scene);

protected:
	/**
	 * @brief Computes dish from pair.
	 * @param a Parameter for a.
	 * @param b Parameter for b.
	 * @return Result produced by this operation.
	 */
	DishType ComputeDishFromPair(IngredientType a, IngredientType b) const;

	std::vector<IngredientType> ingredients_;
	bool dishPrepared_;
	DishType dishType_;

	// Visual tracking of the first ingredient object on this plate
	int firstIngredientObjectID_ = -1;

	/**
	 * @brief Returns the stable name for this object.
	 * @return Requested value.
	 */
	std::string GetName() const override {
		return "PlateLogic";
	}
};

