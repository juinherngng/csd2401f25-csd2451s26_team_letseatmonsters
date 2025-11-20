#pragma once

#include "GameObjectLogic.hpp"
#include "FoodTypes.hpp"

// IngredientLogic
//  - Represents a single ingredient the player can carry, drop, and process.
//  - Tracks whether it has been processed (refined) and its current type.
//
// Lifecycle example:
//   - Start as IngredientType::Meat, isProcessed = false.
//   - WorkTableLogic calls MarkProcessed() when cooking is done.
//   - Type becomes IngredientType::Refined_Meat, isProcessed = true.
class IngredientLogic : public GameObjectLogic
{
public:
    IngredientLogic(int ownerID, IngredientType initialType);

    void Start(Scene& scene) override;
    void Update(float dt, Scene& scene, InputManager& input) override;

    // Basic queries
    IngredientType GetType() const { return type_; }
    bool IsProcessed() const { return isProcessed_; }
    bool IsRaw() const { return !isProcessed_; }

    // Mark this ingredient as processed and update its type to the refined variant
    // (if applicable). Safe to call multiple times.
    void MarkProcessed();

protected:
    IngredientType type_;
    bool isProcessed_;

    std::string GetName() const override { return "IngredientLogic"; }
};
