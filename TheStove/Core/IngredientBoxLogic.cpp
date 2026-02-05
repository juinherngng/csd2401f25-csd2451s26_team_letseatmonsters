/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         IngredientBoxLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung

DESCRIPTION:     Defines IngredientBoxLogic, a table-like object that spawns
                 ingredient items for the player. The box can be configured to
                 spawn vegetables, meat, mushrooms, or plates depending on the
                 setup. Provides logic for spawning, clearing, and controlling
                 which ingredient type appears.



         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */
#include "IngredientBoxLogic.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Core/LogicManager.hpp"
#include <iostream>

IngredientBoxLogic::IngredientBoxLogic(int ownerID) : TableLogic(ownerID)
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

void IngredientBoxLogic::ConfigureAsMeatBox()
{
    spawnMode_ = BoxSpawnMode::Ingredient;
    spawnType_ = IngredientType::Meat;

    // You can tweak these if you have different ingredient types later
    ingredientTexture_ = "../assets/Meat_Ingredient.png";
    ingredientWidth_ = 64.0f;
    ingredientHeight_ = 64.0f;
    ingredientLayer_ = "3";
}

void IngredientBoxLogic::ConfigureAsShroomBox()
{
    spawnMode_ = BoxSpawnMode::Ingredient;
    spawnType_ = IngredientType::Shroom;

    // You can tweak these if you have different ingredient types later
    ingredientTexture_ = "../assets/Mushroom_Ingredient.png";
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
    // Base TableLogic will:
    //  - reset heldItemID_
    //  - override our default offset with Scene::Defaults.vel if non-zero
    TableLogic::Start(scene);

    // Debug: show world-space approach points for this box
    auto worldPoints = GetApproachPointsWorld(scene);
    for (std::size_t i = 0; i < worldPoints.size(); ++i)
    {
        std::cout << "  [IngredientBoxLogic] approach[" << i << "] world=("
            << worldPoints[i].x << ", " << worldPoints[i].y << ")\n";
    }

    // -------- Auto-config based on tag and texture --------
    Scene::Defaults def = scene.GetDefaults(GetOwnerID());
    const std::string& tag = def.tag;
    const std::string& tex = def.texture;

    if (tag == "plate_box")
    {
        ConfigureAsPlateBox();
        return;
    }

    if (tag == "ingredient_box")
    {
        // Decide which ingredient box by looking at the BOX texture name
        if (tex.find("VegIngredientBox") != std::string::npos)
        {
            ConfigureAsVegetableBox();
        }
        else if (tex.find("MeatIngredientBox") != std::string::npos)
        {
            ConfigureAsMeatBox();
        }
        else if (tex.find("ShroomIngredientBox") != std::string::npos)
        {
            ConfigureAsShroomBox();
        }
        else
        {
            ConfigureAsVegetableBox();
            std::cout << "[IngredientBoxLogic] WARNING: owner "
                << GetOwnerID()
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
        if (auto* ingLogic = logicMgr.AddLogic<IngredientLogic>(itemID, spawnType_)) {
            ingLogic->Start(scene);
        }

        std::cout << "[IngredientBoxLogic] Spawned INGREDIENT " << itemID
            << " of type=" << static_cast<int>(spawnType_)
            << " from box " << ownerID_ << "\n";
    }
    else // BoxSpawnMode::Plate
    {
        // Spawn a plate
        spawnedObj = scene.SpawnStaticSpriteAtSamePos(
            ownerID_,
            plateTexture_,
            plateWidth_,
            plateHeight_,
            plateLayer_
        );

        if (!spawnedObj)
        {
            std::cout << "[IngredientBoxLogic] Failed to spawn PLATE from box "
                << ownerID_ << "\n";
            return -1;
        }

        itemID = spawnedObj->GetID();

        {
            Scene::Defaults def = scene.GetDefaults(itemID);
            def.tag = "plate";                   // choose whatever label you like
            scene.SetDefaults(itemID, def);
        }

        if (auto* plateLogic = logicMgr.AddLogic<PlateLogic>(itemID)) {
            plateLogic->Start(scene);
        }


        std::cout << "[IngredientBoxLogic] Spawned PLATE " << itemID
            << " from box " << ownerID_ << "\n";
    }

    return itemID;
}
