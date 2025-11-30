/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         PlateLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung

DESCRIPTION:     Implements PlateLogic, which manages the assembly of processed
                 ingredients into completed dishes. Handles ingredient validation,
                 dish-recipe matching, storing ingredient types, and determining
                 the final dish output (VegDish, MeatDish, SoupDish, etc.).


         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <vector>
#include "GameObjectLogic.hpp"
#include "FoodTypes.hpp"
#include "IngredientLogic.hpp"

// PlateLogic
//  - Represents a plate that can accept refined ingredients and assemble them into a dish.
//  - Works on logical types (IngredientType, DishType) rather than directly spawning objects.
//  - External systems (PlayerLogic, TableLogic, Factory) can:
//      * Call CanAcceptIngredientType / AddIngredientType when the player drops an ingredient.
//      * Call TryAssembleDish to compute the resulting DishType and know which ingredients are consumed.
//      * Use GetDishType / HasPreparedDish to query state when serving customers.
class PlateLogic : public GameObjectLogic
{
public:
    explicit PlateLogic(int ownerID);

    void Start(Scene& scene) override;
    void Update(float dt, Scene& scene, InputManager& input) override;

    // ----- Ingredient placement -----

    // Return true if this plate can accept another ingredient of the given type.
    // Base rules:
    //   - No dish already prepared.
    //   - Limited to two ingredients.
    //   - Only refined ingredients are allowed (Refined_*).
    bool CanAcceptIngredientType(IngredientType type) const;

    // Add an ingredient type to the plate's contents.
    // Assumes CanAcceptIngredientType(type) has been checked by caller.
    void AddIngredientType(IngredientType type);

    // Current ingredients placed on the plate (by type only).
    const std::vector<IngredientType>& GetIngredients() const { return ingredients_; }

    bool HasAnyIngredient() const { return !ingredients_.empty(); }
    bool HasTwoIngredients() const { return ingredients_.size() >= 2; }

    void ClearIngredients();

    // ----- Dish assembly -----

    // Returns true if a dish has been successfully assembled and is ready.
    bool HasPreparedDish() const { return dishPrepared_; }

    // If HasPreparedDish() is true, this returns the resulting dish type.
    DishType GetDishType() const { return dishType_; }

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
    bool TryAssembleDish(DishType& outDishType,
        std::vector<IngredientType>& outConsumedIngredients);

    // Reset the prepared dish (e.g., after serving or throwing away).
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
    bool TryAddIngredient(const IngredientLogic& ingredient, bool& outConsumedNow);

    // How many logical ingredients are currently on this plate?
    int GetIngredientCount() const { return static_cast<int>(ingredients_.size()); }

    // Track the GameObject ID of the first ingredient that was put on this plate
    void SetFirstIngredientObjectID(int id) { firstIngredientObjectID_ = id; }
    int  GetFirstIngredientObjectID() const { return firstIngredientObjectID_; }

protected:
    // Helper that computes the resulting dish type from two refined ingredient types.
    // This corresponds to the big if/else in your Unity Plate.AssembleDish().
    DishType ComputeDishFromPair(IngredientType a, IngredientType b) const;

    std::vector<IngredientType> ingredients_;
    bool dishPrepared_;
    DishType dishType_;

    // NEW: visual tracking of the first ingredient object on this plate
    int         firstIngredientObjectID_ = -1;

    std::string GetName() const override { return "PlateLogic"; }
};
