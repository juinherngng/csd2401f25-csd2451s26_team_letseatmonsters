#include "IngredientBoxLogic.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Core/LogicManager.hpp"
#include <iostream>

IngredientBoxLogic::IngredientBoxLogic(int ownerID)
    : TableLogic(ownerID)
{
}

void IngredientBoxLogic::ConfigureAsVegetableBox()
{
    spawnMode_ = BoxSpawnMode::Ingredient;
    spawnType_ = IngredientType::Vegetable;

    // You can tweak these if you have different ingredient types later
    ingredientTexture_ = "../assets/Cabbage_Ingredient.png";
    ingredientWidth_ = 64.0f;
    ingredientHeight_ = 64.0f;
    ingredientLayer_ = "3";
}

void IngredientBoxLogic::ConfigureAsPlateBox()
{
    spawnMode_ = BoxSpawnMode::Plate;

    // TODO: update these to match your plate asset
    plateTexture_ = "../assets/Plate.png";   // e.g. "../assets/Plate_Empty.png"
    plateWidth_ = 64.0f;
    plateHeight_ = 64.0f;
    plateLayer_ = "2";
}

void IngredientBoxLogic::Start(Scene& scene)
{
    TableLogic::Start(scene);

    ClearApproachOffsets();

    AddApproachOffset(Math::Vector2D(0.0f, 110.0f));
    std::cout << "[IngredientBoxLogic] Start on owner " << GetOwnerID() << "\n";

    // Debug: show world-space approach points for this box
    auto worldPoints = GetApproachPointsWorld(scene);
    for (std::size_t i = 0; i < worldPoints.size(); ++i)
    {
        std::cout << "  [IngredientBoxLogic] approach[" << i << "] world=("
            << worldPoints[i].x << ", " << worldPoints[i].y << ")\n";
    }

    // -------- Auto-config based on tag (optional) --------
// Assumes your GameObject has some kind of GetTag() / GetTagName().
// If API is different, just adjust this block.
    if (GameObject* owner = GetOwner(scene))
    {
        const std::string& tag = scene.GetDefaults(GetOwnerID()).tag;

        if (tag == "ingredient_box")
        {
            ConfigureAsVegetableBox();
        }
        else if (tag == "plate_box")
        {
            ConfigureAsPlateBox();
        }
        // else: keep whatever default configuration you want
        else
        {
            ConfigureAsVegetableBox();
            std::cout << "[IngredientBoxLogic] WARNING: owner " << ownerID
                << " has unknown tag '" << tag
                << "', defaulting to vegetable box\n";
        }
    }

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
    const int ownerID_ = GetOwnerID();
    LogicManager& logicMgr = scene.GetLogicManager();

    GameObject* spawnedObj = nullptr;
    int itemID = -1;

    if (spawnMode_ == BoxSpawnMode::Ingredient)
    {
        // Spawn a raw ingredient (e.g. cabbage)
        spawnedObj = scene.SpawnStaticSpriteAtSamePos(
            ownerID_,
            ingredientTexture_,
            ingredientWidth_,
            ingredientHeight_,
            ingredientLayer_
        );

        if (!spawnedObj)
        {
            std::cout << "[IngredientBoxLogic] Failed to spawn INGREDIENT from box "
                << ownerID_ << "\n";
            return -1;
        }

        itemID = spawnedObj->GetID();

        {
            Scene::Defaults def = scene.GetDefaults(itemID);
            def.tag = "ingredient";              // or "ingredient_item", up to you
            scene.SetDefaults(itemID, def);
        }

        // Attach IngredientLogic with the configured ingredient type.
        logicMgr.AddLogic<IngredientLogic>(itemID, spawnType_);

        std::cout << "[IngredientBoxLogic] Spawned INGREDIENT " << itemID
            << " of type=" << static_cast<int>(spawnType_)
            << " from box " << ownerID << "\n";
    }
    else // BoxSpawnMode::Plate
    {
        // Spawn a plate
        spawnedObj = scene.SpawnStaticSpriteAtSamePos(
            ownerID,
            plateTexture_,
            plateWidth_,
            plateHeight_,
            plateLayer_
        );

        if (!spawnedObj)
        {
            std::cout << "[IngredientBoxLogic] Failed to spawn PLATE from box "
                << ownerID << "\n";
            return -1;
        }

        itemID = spawnedObj->GetID();

        {
            Scene::Defaults def = scene.GetDefaults(itemID);
            def.tag = "plate";                   // choose whatever label you like
            scene.SetDefaults(itemID, def);
        }

        // Attach PlateLogic. This assumes PlateLogic has constructor PlateLogic(int ownerID).
        // If your constructor is different, just adjust this line.
        logicMgr.AddLogic<PlateLogic>(itemID);

        std::cout << "[IngredientBoxLogic] Spawned PLATE " << itemID
            << " from box " << ownerID << "\n";
    }

    return itemID;
}
