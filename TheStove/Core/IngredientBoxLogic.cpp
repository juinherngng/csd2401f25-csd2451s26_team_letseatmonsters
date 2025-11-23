#include "IngredientBoxLogic.hpp"
#include "IngredientLogic.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Core/LogicManager.hpp"
#include <iostream>

IngredientBoxLogic::IngredientBoxLogic(int ownerID)
    : TableLogic(ownerID)
{
}

void IngredientBoxLogic::Start(Scene& scene)
{
    TableLogic::Start(scene);
    std::cout << "[IngredientBoxLogic] Start on owner " << GetOwnerID() << "\n";
}

void IngredientBoxLogic::Update(float dt, Scene& scene, InputManager& input)
{
    // Keep any base behaviour you want from TableLogic
    TableLogic::Update(dt, scene, input);
}

bool IngredientBoxLogic::CanAcceptItem(Scene& scene, int itemID) const
{
    (void)scene;
    (void)itemID;
    // Ingredient box is a source, not a container.
    return false;
}

int IngredientBoxLogic::SpawnIngredient(Scene& scene)
{
    const int ownerID = GetOwnerID();

    // Use a Scene helper so we avoid glm here.
    GameObject* ingredientObj = scene.SpawnStaticSpriteAtSamePos(
        ownerID,
        ingredientTexture_,
        ingredientWidth_,
        ingredientHeight_,
        ingredientLayer_
    );
    if (!ingredientObj)
    {
        std::cout << "[IngredientBoxLogic] Failed to spawn ingredient from box "
            << ownerID << "\n";
        return -1;
    }

    const int itemID = ingredientObj->GetID();

    // Attach IngredientLogic with type Vegetable.
    LogicManager& logicMgr = scene.GetLogicManager();
    logicMgr.AddLogic<IngredientLogic>(itemID, spawnType_);

    std::cout << "[IngredientBoxLogic] Spawned ingredient " << itemID
        << " of type=" << static_cast<int>(spawnType_)
        << " from box " << ownerID << "\n";

    return itemID;
}
