#include "IngredientLogic.hpp"
#include "../Graphics/SceneManager.hpp"

IngredientLogic::IngredientLogic(int ownerID, IngredientType initialType)
    : GameObjectLogic(ownerID)
    , type_(initialType)
    , isProcessed_(false)
{
}

void IngredientLogic::Start(Scene& /*scene*/)
{
    // Nothing special for now.
}

void IngredientLogic::Update(float /*dt*/, Scene& /*scene*/, InputManager& /*input*/)
{
    // No per-frame logic needed yet.
    // Processing is triggered explicitly via MarkProcessed().
}

void IngredientLogic::MarkProcessed()
{
    if (isProcessed_)
        return;

    isProcessed_ = true;

    // Update type from raw → refined
    switch (type_)
    {
    case IngredientType::Vegetable:
        type_ = IngredientType::Refined_Veg;
        break;
    case IngredientType::Meat:
        type_ = IngredientType::Refined_Meat;
        break;
    case IngredientType::Shroom:
        type_ = IngredientType::Refined_Shroom;
        break;
    default:
        // Already refined; nothing to do.
        break;
    }
}
