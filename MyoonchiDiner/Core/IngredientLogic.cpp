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

#include "Graphics/SceneManager.hpp"

#include "IngredientLogic.hpp"

IngredientLogic::IngredientLogic(int ownerID, IngredientType initialType)
	: GameObjectLogic(ownerID)
	, type_(initialType)
	, isProcessed_(false) {
}

void IngredientLogic::Start(Scene& /*scene*/) {
	// Nothing special for now.
}

void IngredientLogic::Update(float /*dt*/, Scene& /*scene*/, InputManager& /*input*/) {
	// No per-frame logic needed yet.
	// Processing is triggered explicitly via MarkProcessed().
}

void IngredientLogic::MarkProcessed() {
	if (isProcessed_)
		return;

	isProcessed_ = true;

	// Update type from raw to refined
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
		// Already refined; nothing to do.
		break;
	}
}

