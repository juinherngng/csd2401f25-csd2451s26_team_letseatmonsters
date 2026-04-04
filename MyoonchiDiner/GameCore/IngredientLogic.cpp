/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         IngredientLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Implements IngredientLogic, the behavior script for food
					ingredients placed in the world. Tracks ingredient type, raw vs.
					processed state, responds to interactions with worktables and
					plates, and exposes helper functions to query ingredient state

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/IngredientLogic.hpp"

 /**
  * @brief Constructs ingredient logic for a spawned world ingredient.
  * @param ownerID Runtime ID of the GameObject that owns this logic.
  * @param initialType Initial ingredient type assigned to the object.
  */
IngredientLogic::IngredientLogic(int ownerID, IngredientType initialType)
	: GameObjectLogic(ownerID)
	, type_(initialType)
	, isProcessed_(false) {
	// Start each spawned ingredient in the raw state until a station refines it.
}

/**
 * @brief Initializes the ingredient's runtime state after scene creation.
 * @param scene Unused active scene reference.
 */
void IngredientLogic::Start(Scene& /*scene*/) {
	// Ingredient runtime state is fully established by the constructor for now.
}

/**
 * @brief Updates the ingredient for one frame.
 * @param dt Unused delta time for the frame.
 * @param scene Unused active scene reference.
 * @param input Unused input manager reference.
 */
void IngredientLogic::Update(float /*dt*/, Scene& /*scene*/, InputManager& /*input*/) {
	// Ingredient behavior is event-driven right now, so no per-frame work is required.
	// Stations or recipes explicitly call MarkProcessed() when this ingredient changes state.
}

/**
 * @brief Marks this ingredient as processed and upgrades its type when applicable.
 */
void IngredientLogic::MarkProcessed() {
	// Ignore duplicate processing so callers can safely trigger this more than once.
	if (isProcessed_)
		return;

	isProcessed_ = true;

	// Promote raw ingredient types to their refined counterparts once processing finishes.
	switch (type_) {
	case IngredientType::Vegetable:
		type_ = IngredientType::Refined_Veg;
		break;
	case IngredientType::Meat:
		type_ = IngredientType::Refined_Meat;
		break;
	case IngredientType::Shroom:
		type_ = IngredientType::Refined_Shroom;
		break;
	case IngredientType::Carrot:
		type_ = IngredientType::Refined_Carrot;
		break;
	default:
		// Leave already refined or unsupported types unchanged.
		break;
	}
}
