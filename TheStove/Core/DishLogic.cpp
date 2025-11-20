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
