/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         DishLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Implements dish object behaviour used by customers and
                    tables. Stores the dish type and eaten state, and provides
                    interfaces for marking a dish as eaten. Currently does not
                    contain per-frame behaviour.

         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */


#include "DishLogic.hpp"
#include "../Graphics/SceneManager.hpp"

DishLogic::DishLogic(int ownerID, DishType type) : GameObjectLogic(ownerID), dishType_(type), isEaten_(false)
{
}

void DishLogic::Start(Scene& /*scene*/)
{
    // Nothing special yet.
}

void DishLogic::Update(float /*dt*/, Scene& /*scene*/, InputManager& /*input*/)
{
    // No per-frame logic yet.
    // Customer / game rules can call MarkEaten() when appropriate.
}
