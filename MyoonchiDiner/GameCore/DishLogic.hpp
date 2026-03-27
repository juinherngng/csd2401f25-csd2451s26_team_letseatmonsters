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
	// Constructor takes ownerID and dish type
	DishLogic(int ownerID, DishType type);

	// GameObjectLogic overrides
	void Start(Scene& scene) override;
	void Update(float dt, Scene& scene, InputManager& input) override;

	// Accessors and mutators
	DishType GetDishType() const {
		return dishType_;
	}

	bool IsEaten() const {
		return isEaten_;
	}

	void MarkEaten() {
		isEaten_ = true;
	}

protected:
	// Optional: override GetName for debugging purposes
	DishType dishType_;
	bool isEaten_;

	std::string GetName() const override {
		return "DishLogic";
	}
};
