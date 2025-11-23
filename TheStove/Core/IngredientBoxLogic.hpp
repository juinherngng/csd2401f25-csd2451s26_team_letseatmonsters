#pragma once

#include "TableLogic.hpp"
#include "FoodTypes.hpp"

// A table-like object that acts as an ingredient source.
// When player interacts while empty-handed, it spawns a Vegetable.
class IngredientBoxLogic : public TableLogic
{
public:
    explicit IngredientBoxLogic(int ownerID);

    void Start(Scene& scene) override;
    void Update(float dt, Scene& scene, InputManager& input) override;

    // Ingredient box is a source – it should never accept dropped items.
    bool CanAcceptItem(Scene& scene, int itemID) const override;

    // Called by PlayerLogic when interacting while empty-handed.
    // Returns new ingredient GameObject ID (or -1 on failure).
    int SpawnIngredient(Scene& scene);

private:
    IngredientType spawnType_ = IngredientType::Vegetable;

    // Config for the spawned ingredient sprite – tweak as needed:
    const char* ingredientTexture_ = "../assets/Cabbage_Ingredient.png"; // change to your real asset
    float ingredientWidth_ = 64.0f;
    float ingredientHeight_ = 64.0f;
    const char* ingredientLayer_ = "1";
};
