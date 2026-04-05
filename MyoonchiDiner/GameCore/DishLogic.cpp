/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         DishLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Implements dish object behaviour used by customers and
					tables. Stores the dish type and eaten state, and provides
					interfaces for marking a dish as eaten. Currently does not
					contain per-frame behaviour.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/DishLogic.hpp"

 /**
  * @brief Constructs dish logic for a completed dish object.
  * @param ownerID Runtime ID of the GameObject that owns this logic.
  * @param type Dish type represented by this object.
  */
DishLogic::DishLogic(int ownerID, DishType type)
	: GameObjectLogic(ownerID), dishType_(type), isEaten_(false) {
	// Start each spawned dish in the uneaten state.
}

/**
 * @brief Initializes the dish's runtime state after scene creation.
 * @param scene Unused active scene reference.
 */
void DishLogic::Start(Scene& /*scene*/) {
	// This logic currently relies only on constructor state, so no extra setup is required.
}

/**
 * @brief Updates the dish for one frame.
 * @param dt Unused delta time for the frame.
 * @param scene Unused active scene reference.
 * @param input Unused input manager reference.
 */
void DishLogic::Update(float /*dt*/, Scene& /*scene*/, InputManager& /*input*/) {
	// Dish state is event-driven for now, so per-frame updates intentionally do nothing.
	// Customer and table logic call MarkEaten() or inspect the dish type when needed.
}
