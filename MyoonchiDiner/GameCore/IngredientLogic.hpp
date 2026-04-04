/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         IngreidnetLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Implements IngredientLogic, the behavior script for food
					ingredients placed in the world. Tracks ingredient type, raw vs.
					processed state, responds to interactions with worktables and
					plates, and exposes helper functions to query ingredient state.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "EngineCore/GameObjectLogic.hpp"
#include "GameCore/FoodTypes.hpp"

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
	/**
	 * @brief Constructs ingredient logic for a spawned world ingredient.
	 * @param ownerID Runtime ID of the GameObject that owns this logic.
	 * @param initialType Initial ingredient type assigned to the object.
	 */
	IngredientLogic(int ownerID, IngredientType initialType);

	/**
	 * @brief Initializes the ingredient's runtime state after scene creation.
	 * @param scene Active scene containing the ingredient object.
	 */
	void Start(Scene& scene) override;

	/**
	 * @brief Updates the ingredient for one frame.
	 * @param dt Delta time for the frame.
	 * @param scene Active scene containing the ingredient object.
	 * @param input Input manager forwarded by the logic system.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Returns the ingredient's current type.
	 * @return Current ingredient type, raw or refined.
	 */
	IngredientType GetType() const {
		// Expose the current type so cooking and recipe systems can inspect this ingredient.
		return type_;
	}

	/**
	 * @brief Returns whether the ingredient has already been processed.
	 * @return True when the ingredient is in a refined state.
	 */
	bool IsProcessed() const {
		// Keep the processed flag readable for station and recipe logic.
		return isProcessed_;
	}

	/**
	 * @brief Returns whether the ingredient is still in its raw state.
	 * @return True when the ingredient has not been processed yet.
	 */
	bool IsRaw() const {
		// Express the common inverse check directly for callers that only care about rawness.
		return !isProcessed_;
	}

	/**
	 * @brief Marks this ingredient as processed and upgrades its type when applicable.
	 */
	void MarkProcessed();

protected:
	IngredientType type_;
	bool isProcessed_;

	/**
	 * @brief Returns the stable runtime logic name used by the engine.
	 * @return Name string for this logic component.
	 */
	std::string GetName() const override {
		// Keep the logic name stable for debugging and runtime registration.
		return "IngredientLogic";
	}
};
