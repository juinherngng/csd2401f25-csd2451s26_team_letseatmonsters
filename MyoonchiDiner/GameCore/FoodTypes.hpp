/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         FoodTypes.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Declares enumerations for food-related types used in the
					game (e.g., DishType, IngredientType). Centralises all
					food type definitions for use by DishLogic, IngredientBoxLogic,
					and customer/table systems.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

 /**
  * @brief Ingredient categories used by the cooking and processing systems.
  */
enum class IngredientType {
	Vegetable,      // Raw vegetable ingredient.
	Meat,           // Raw meat ingredient.
	Shroom,         // Raw mushroom ingredient.
	Carrot,         // Raw carrot ingredient.
	Refined_Veg,    // Processed vegetable ingredient.
	Refined_Meat,   // Processed meat ingredient.
	Refined_Shroom, // Processed mushroom ingredient.
	Refined_Carrot  // Processed carrot ingredient.
};

/**
 * @brief Final dish categories that can be assembled and served to customers.
 */
enum class DishType {
	MeatDish,        // Plate assembled from the meat recipe.
	VegDish,         // Plate assembled from the vegetable recipe.
	SoupDish,        // Plate assembled from the soup recipe.
	SkewerDish,      // Plate assembled from the meat-and-carrot recipe.
	CarrotSaladDish, // Plate assembled from the vegetable-and-carrot recipe.
	PoopDish         // Failure dish produced by an invalid recipe.
};
