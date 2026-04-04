/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         DishLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Declares the DishLogic class, storing information about
					dish type, eaten state, and providing utilities for game
					systems (e.g., customers or tables) to mark dishes as eaten.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "EngineCore/GameObjectLogic.hpp"
#include "GameCore/FoodTypes.hpp"

 // DishLogic
 //  - Represents a completed dish that can be placed on a plate,
 //    served to customers, and later marked as eaten.
class DishLogic : public GameObjectLogic {
public:
	/**
	 * @brief Constructs dish logic for a completed dish object.
	 * @param ownerID Runtime ID of the GameObject that owns this logic.
	 * @param type Dish type represented by this object.
	 */
	DishLogic(int ownerID, DishType type);

	/**
	 * @brief Initializes the dish's runtime state after scene creation.
	 * @param scene Active scene containing the dish object.
	 */
	void Start(Scene& scene) override;

	/**
	 * @brief Updates the dish for one frame.
	 * @param dt Delta time for the frame.
	 * @param scene Active scene containing the dish object.
	 * @param input Input manager forwarded by the logic system.
	 */
	void Update(float dt, Scene& scene, InputManager& input) override;

	/**
	 * @brief Returns the type of dish represented by this logic.
	 * @return Stored dish type.
	 */
	DishType GetDishType() const {
		// Expose the authored dish type so serving and scoring systems can query it.
		return dishType_;
	}

	/**
	 * @brief Returns whether the dish has already been consumed.
	 * @return True when the dish has been marked as eaten.
	 */
	bool IsEaten() const {
		// Keep the eaten flag readable for customer and cleanup logic.
		return isEaten_;
	}

	/**
	 * @brief Marks the dish as eaten.
	 */
	void MarkEaten() {
		// Flip the consumed flag so gameplay systems know this dish is no longer fresh.
		isEaten_ = true;
	}

protected:
	DishType dishType_;
	bool isEaten_;

	/**
	 * @brief Returns the stable runtime logic name used by the engine.
	 * @return Name string for this logic component.
	 */
	std::string GetName() const override {
		// Keep the logic name stable for debugging and runtime registration.
		return "DishLogic";
	}
};
