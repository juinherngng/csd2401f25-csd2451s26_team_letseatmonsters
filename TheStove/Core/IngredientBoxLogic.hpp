#pragma once

#include "TableLogic.hpp"
#include "FoodTypes.hpp"
#include "IngredientLogic.hpp"
#include "PlateLogic.hpp" 

// Forward declare if needed
class IngredientLogic;
class PlateLogic;

// A table-like object that acts as an ingredient source.
// When player interacts while empty-handed, it spawns a Vegetable.
class IngredientBoxLogic : public TableLogic
{
public:
    enum class BoxSpawnMode
    {
        Ingredient, // spawn IngredientLogic
        Plate       // spawn PlateLogic
    };

    explicit IngredientBoxLogic(int ownerID);

    void Start(Scene& scene) override;
    void Update(float dt, Scene& scene, InputManager& input) override;

    // Ingredient box is a source – it should never accept dropped items.
    bool CanAcceptItem(Scene& scene, int itemID) const override;

    // Called by PlayerLogic when interacting while empty-handed.
    // Returns new ingredient GameObject ID (or -1 on failure).
    int SpawnIngredient(Scene& scene);

    // Explicitly choose mode
    void SetSpawnMode(BoxSpawnMode mode) { spawnMode_ = mode; }

    // Convenience: configure as "vegetable box"
    void ConfigureAsVegetableBox();

    // Convenience: configure as "plate box"
    void ConfigureAsPlateBox();

private:
    // What this box spawns
    BoxSpawnMode spawnMode_ = BoxSpawnMode::Ingredient;

    IngredientType spawnType_ = IngredientType::Vegetable;

    // Config for the spawned ingredient sprite – tweak as needed:
    const char* ingredientTexture_ = "../assets/Cabbage_Ingredient.png"; // change to your real asset
    float ingredientWidth_ = 64.0f;
    float ingredientHeight_ = 64.0f;
    const char* ingredientLayer_ = "3";

    // Config for the spawned plate sprite – tweak as needed:
    const char* plateTexture_ = "../assets/Plate.png";
    float       plateWidth_ = 64.0f;
    float       plateHeight_ = 64.0f;
    const char* plateLayer_ = "2";
};
