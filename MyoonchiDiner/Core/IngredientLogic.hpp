/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         IngreidnetLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Implements IngredientLogic, the behavior script for food
					ingredients placed in the world. Tracks ingredient type, raw vs.
					processed state, responds to interactions with worktables and
					plates, and exposes helper functions to query ingredient state.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "FoodTypes.hpp"
#include "GameObjectLogic.hpp"

 // IngredientLogic
 //  - Represents a single ingredient the player can carry, drop, and process.
 //  - Tracks whether it has been processed (refined) and its current type.
 //
 // Lifecycle example:
 //   - Start as IngredientType::Meat, isProcessed = false.
 //   - WorkTableLogic calls MarkProcessed() when cooking is done.
 //   - Type becomes IngredientType::Refined_Meat, isProcessed = true.
class IngredientLogic : public GameObjectLogic {
public:
	// Constructor takes initial type and defaults to raw (not processed)
	IngredientLogic(int ownerID, IngredientType initialType);

	// Lifecycle overrides
	void Start(Scene& scene) override;
	void Update(float dt, Scene& scene, InputManager& input) override;

	// Basic queries
	IngredientType GetType() const {
		return type_;
	}
	bool IsProcessed() const {
		return isProcessed_;
	}
	bool IsRaw() const {
		return !isProcessed_;
	}

	// Mark this ingredient as processed and update its type to the refined variant
	// (if applicable). Safe to call multiple times.
	void MarkProcessed();

protected:
	IngredientType type_;
	bool isProcessed_;

	std::string GetName() const override {
		return "IngredientLogic";
	}
};
