/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         PlateLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (60%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu   (40%)

 DESCRIPTION:       Implements PlateLogic, which manages the assembly of processed
					ingredients into completed dishes. Handles ingredient validation,
					dish-recipe matching, storing ingredient types, and determining
					the final dish output (VegDish, MeatDish, SoupDish, etc.).

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <vector>

#include "EngineCore/GameObjectLogic.hpp"
#include "GameCore/FoodTypes.hpp"
#include "GameCore/IngredientLogic.hpp"

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
	 * @brief Constructs plate logic for a spawned plate object.
	 * @param ownerID Runtime ID of the GameObject that owns this logic.
	 */
	explicit PlateLogic(int ownerID);

	/**
	 * @brief Initializes the plate's runtime state after scene creation.
	 * @param scene Active scene containing the plate object.
	 */
	void Start(Scene& scene) override;

	/**
	 * @brief Updates the plate for one frame.
	 * @param dt Delta time for the frame.
	 * @param scene Active scene containing the plate object.
	 * @param input Input manager forwarded by the logic system.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Cleans up any child visuals owned by this plate before destruction.
	 * @param scene Active scene containing the plate object.
	 */
	void OnDestroy(Scene& scene) override;

	// ----- Ingredient placement -----

	/**
	 * @brief Returns whether the plate can accept the supplied ingredient type.
	 * @param type Ingredient type being considered for placement.
	 * @return True if the ingredient type is valid and the plate still has room.
	 */
	bool CanAcceptIngredientType(IngredientType type) const;

	/**
	 * @brief Adds an ingredient type to the plate's logical ingredient list.
	 * @param type Ingredient type to append.
	 */
	void AddIngredientType(IngredientType type);

	/**
	 * @brief Returns the current logical ingredient list stored on the plate.
	 * @return Reference to the plate's ingredient list.
	 */
	const std::vector<IngredientType>& GetIngredients() const {
		// Expose the logical recipe state so serving and assembly systems can inspect it.
		return ingredients_;
	}

	/**
	 * @brief Returns whether the plate currently holds any ingredients.
	 * @return True when at least one ingredient has been added.
	 */
	bool HasAnyIngredient() const {
		// Treat any non-empty ingredient list as proof that the plate is no longer empty.
		return !ingredients_.empty();
	}

	/**
	 * @brief Returns whether the plate currently holds at least two ingredients.
	 * @return True when the plate has enough ingredients to attempt dish assembly.
	 */
	bool HasTwoIngredients() const {
		// Most recipes currently require two refined ingredients before assembly can begin.
		return ingredients_.size() >= 2;
	}

	/**
	 * @brief Clears all logical ingredients from the plate.
	 */
	void ClearIngredients();

	// ----- Dish assembly -----

	/**
	 * @brief Returns whether the plate has already been assembled into a finished dish.
	 * @return True when `dishPrepared_` is set.
	 */
	bool HasPreparedDish() const {
		// Finished dishes block additional ingredient placement until the plate is cleared.
		return dishPrepared_;
	}

	/**
	 * @brief Returns the prepared dish type currently represented by the plate.
	 * @return Prepared dish type, or `PoopDish` when nothing valid has been assembled.
	 */
	DishType GetDishType() const {
		// Expose the final assembled dish type for serving and scoring systems.
		return dishType_;
	}

	/**
	 * @brief Attempts to assemble a finished dish from the currently stored ingredients.
	 * @param outDishType Output dish type produced by the attempted recipe.
	 * @param outConsumedIngredients Output list populated with the ingredients that formed the recipe.
	 * @return True when the plate successfully assembles into a dish.
	 */
	bool TryAssembleDish(DishType& outDishType,
		std::vector<IngredientType>& outConsumedIngredients);

	/**
	 * @brief Clears the prepared-dish state and resets the plate back to empty.
	 */
	void ClearPreparedDish();

	/**
	 * @brief Attempts to add a specific ingredient object to this plate.
	 * @param ingredient Ingredient logic representing the candidate ingredient object.
	 * @param outConsumedNow Output flag indicating whether the ingredient was consumed immediately.
	 * @return True if the ingredient was accepted onto the plate.
	 */
	bool TryAddIngredient(const IngredientLogic& ingredient, bool& outConsumedNow);

	/**
	 * @brief Returns how many logical ingredients are currently stored on the plate.
	 * @return Number of stored ingredients.
	 */
	int GetIngredientCount() const {
		// Convert the container size once so callers do not repeat the cast everywhere.
		return static_cast<int>(ingredients_.size());
	}

	/**
	 * @brief Stores the runtime ID of the first ingredient visual attached to this plate.
	 * @param id Runtime ID of the attached ingredient object.
	 */
	void SetFirstIngredientObjectID(int id) {
		// Track the child visual so it can be cleaned up or replaced later.
		firstIngredientObjectID_ = id;
	}

	/**
	 * @brief Returns the runtime ID of the first ingredient visual attached to this plate.
	 * @return Attached ingredient object ID, or `-1` when none is tracked.
	 */
	int  GetFirstIngredientObjectID() const {
		// Expose the tracked child visual for cleanup and presentation logic.
		return firstIngredientObjectID_;
	}

	/**
	 * @brief Applies the correct dish texture to the plate after assembly.
	 * @param scene Active scene containing the plate object.
	 */
	void ApplyDishVisual(Scene& scene);

protected:
	/**
	 * @brief Computes the resulting dish for a pair of refined ingredients.
	 * @param a First refined ingredient type.
	 * @param b Second refined ingredient type.
	 * @return Dish type produced by that ingredient combination.
	 */
	DishType ComputeDishFromPair(IngredientType a, IngredientType b) const;

	std::vector<IngredientType> ingredients_;
	bool dishPrepared_;
	DishType dishType_;

	// Visual tracking of the first ingredient object on this plate
	int firstIngredientObjectID_ = -1;

	/**
	 * @brief Returns the stable runtime logic name used by the engine.
	 * @return Name string for this logic component.
	 */
	std::string GetName() const override {
		// Keep the logic name stable for debugging and runtime registration.
		return "PlateLogic";
	}
};
