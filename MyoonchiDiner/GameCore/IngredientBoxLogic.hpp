/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         IngredientBoxLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Defines IngredientBoxLogic, a table-like object that spawns
					ingredient items for the player. The box can be configured to
					spawn vegetables, meat, mushrooms, or plates depending on the
					setup. Provides logic for spawning, clearing, and controlling
					which ingredient type appears.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "GameCore/FoodTypes.hpp"
#include "GameCore/IngredientLogic.hpp"
#include "GameCore/PlateLogic.hpp"
#include "GameCore/TableLogic.hpp"

 // Forward declare if needed
class IngredientLogic;
class PlateLogic;

// A table-like object that acts as an ingredient source.
// When player interacts while empty-handed, it spawns a Vegetable.
class IngredientBoxLogic : public TableLogic {
public:
	enum class BoxSpawnMode {
		Ingredient, // spawn IngredientLogic
		Plate       // spawn PlateLogic
	};

	/**
	 * @brief Constructs an ingredient-box controller for the specified owner object.
	 * @param ownerID Runtime ID of the box GameObject that owns this logic.
	 */
	explicit IngredientBoxLogic(int ownerID);

	/**
	 * @brief Initializes the box and auto-configures its spawn mode from scene metadata.
	 * @param scene Active scene containing the ingredient box object.
	 */
	void Start(Scene& scene) override;

	/**
	 * @brief Updates the ingredient box for one frame.
	 * @param dt Delta time for the frame.
	 * @param scene Active scene containing the ingredient box object.
	 * @param input Input manager forwarded by the logic system.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Returns whether this box can accept a dropped item.
	 * @param scene Active scene containing the box and candidate item.
	 * @param itemID Runtime ID of the item being tested.
	 * @return Always false because ingredient boxes are sources, not containers.
	 */
	bool CanAcceptItem(Scene& scene, int itemID) const override;

	/**
	 * @brief Spawns the box's configured output item at the box position.
	 * @param scene Active scene containing the ingredient box object.
	 * @return Runtime ID of the spawned item, or `-1` on failure.
	 */
	int SpawnIngredient(Scene& scene);

	/**
	 * @brief Sets whether this box should spawn ingredients or plates.
	 * @param mode Desired spawn mode for future interactions.
	 */
	void SetSpawnMode(BoxSpawnMode mode) {
		// Persist the selected spawn mode so future interactions create the correct item family.
		spawnMode_ = mode;
	}

	/**
	 * @brief Configures this box to spawn vegetable ingredients.
	 */
	void ConfigureAsVegetableBox();

	/**
	 * @brief Configures this box to spawn meat ingredients.
	 */
	void ConfigureAsMeatBox();

	/**
	 * @brief Configures this box to spawn mushroom ingredients.
	 */
	void ConfigureAsShroomBox();

	/**
	 * @brief Configures this box to spawn carrot ingredients.
	 */
	void ConfigureAsCarrotBox();

	/**
	 * @brief Configures this box to spawn empty plates instead of ingredients.
	 */
	void ConfigureAsPlateBox();

private:
	// What this box spawns
	BoxSpawnMode spawnMode_ = BoxSpawnMode::Ingredient;

	IngredientType spawnType_ = IngredientType::Vegetable;

	// Config for the spawned ingredient sprite, tweak as needed:
	const char* ingredientTexture_ = "../assets/Food/Cabbage_Ingredient.png"; // change to your real asset
	float ingredientWidth_ = 64.0f;
	float ingredientHeight_ = 64.0f;
	const char* ingredientLayer_ = "3";

	// Config for the spawned plate sprite, tweak as needed:
	const char* plateTexture_ = "../assets/Food/Plate.png";
	float       plateWidth_ = 64.0f;
	float       plateHeight_ = 64.0f;
	const char* plateLayer_ = "2";
};
