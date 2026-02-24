/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         FoodTypes.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Declares enumerations for food-related types used in the
                    game (e.g., DishType, IngredientType). Centralises all
                    food type definitions for use by DishLogic, IngredientBoxLogic,
                    and customer/table systems.

         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

// Types of ingredients used in the cooking system.
enum class IngredientType
{
    Vegetable,
    Meat,
    Shroom,
    Refined_Veg,
    Refined_Meat,
    Refined_Shroom
};

// Types of finished dishes that can be served to customers.
enum class DishType
{
    MeatDish,
    VegDish,
    SoupDish,
    PoopDish
};
