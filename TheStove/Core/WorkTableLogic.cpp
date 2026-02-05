/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         WorkTableLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung

 DESCRIPTION:       Implements WorkTableLogic, the type of table that accepts raw
                    ingredients, processes them into refined ingredients, and allows
                    players to interact with workstations for cooking or preparation.
                    This class overrides base TableLogic behavior to restrict what
                    items can be placed, manage processing states, and output the
                    refined ingredient when complete.

         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "WorkTableLogic.hpp"
#include "../Core/AudioManager.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Graphics/GameObject.hpp"

static bool Contains(const std::string& s, const char* sub)
{
    return s.find(sub) != std::string::npos;
}

WorkTableLogic::StationType WorkTableLogic::DetectStationTypeFromTexture(const std::string& texPath) const
{
    // Detect by the workstation sprite (the table's texture)
    if (Contains(texPath, "Cutting_Board")) return StationType::CuttingBoard;
    if (Contains(texPath, "Grills"))        return StationType::Grill;
    if (Contains(texPath, "Stove"))         return StationType::Stove;
    return StationType::Generic;
}


const char* WorkTableLogic::GetProcessedTextureForRaw(IngredientType rawType) const
{
    // IMPORTANT: Replace these 2 paths with your actual cooked meat/shroom assets.
    switch (rawType)
    {
    case IngredientType::Vegetable: return "../assets/Cabbage_CUT_Ingredient.png";
    case IngredientType::Meat:      return "../assets/Meat_CUT_Ingredient.png";
    case IngredientType::Shroom:    return "../assets/Mushroom_CUT_Ingredient.png";
    default:                        return "../assets/Cabbage_CUT_Ingredient.png";
    }
}

const char* WorkTableLogic::GetProcessingSoundName() const
{
    switch (stationType_)
    {
    case StationType::CuttingBoard: return "sfx_chopping";
    case StationType::Grill:        return "sfx_grill";
    case StationType::Stove:        return "sfx_boiling_sound";
    default:                        return nullptr;
    }
}

// ------------------- Constructor -------------------

WorkTableLogic::WorkTableLogic(int ownerID) : TableLogic(ownerID)
{

}

void WorkTableLogic::Start(Scene& scene)
{
    //std::cout << "[WorkTableLogic] Start ownerID=" << GetOwnerID() << "\n";

    TableLogic::Start(scene);

    //Figure out what kind of station THIS table is, from its texture
    Scene::Defaults def = scene.GetDefaults(GetOwnerID());
    stationType_ = DetectStationTypeFromTexture(def.texture);

    //different speeds per station
    switch (stationType_)
    {
    case StationType::CuttingBoard: processingTime_ = 3.0f; break;
    case StationType::Grill:        processingTime_ = 5.0f; break;
    case StationType::Stove:        processingTime_ = 7.0f; break;
    default:                        processingTime_ = 3.0f; break;
    }
}

// ------------------- Update -------------------

void WorkTableLogic::Update(float dt, Scene& scene, InputManager&)
{
    // Only do anything if we have an item and are currently processing
    if (!isProcessing_ || !HasItem())
        return;

    timer_ += dt;

    //std::cout << "[WorkTableLogic] processing... t=" << timer_
    //    << "/" << processingTime_ << "\n";

    if (timer_ >= processingTime_)
    {
        timer_ = processingTime_;
        isProcessing_ = false;
        
        // Stop station-specific processing sound when complete (release mode only)
#ifndef _DEBUG
        if (AudioManager* audioMgr = scene.GetAudioManager()) {
            const char* soundName = GetProcessingSoundName();
            if (soundName && audioMgr->HasSound(soundName)) {
                audioMgr->StopSound(soundName);
            }
        }
#endif

        GameObject* item = scene.GetGameObjectByID(GetHeldItemID());
        if (item)
        {
            OnProcessingComplete(scene, *item);
        }
    }
}

float WorkTableLogic::GetProcessingProgress() const
{
    if (!isProcessing_ || processingTime_ <= 0.0f)
        return 0.0f;

    float t = timer_ / processingTime_;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t;
}

bool WorkTableLogic::CanAcceptItem(Scene& scene, int itemID) const
{
    if (!TableLogic::CanAcceptItem(scene, itemID))
        return false;

    GameObject* item = scene.GetGameObjectByID(itemID);
    if (!item)
        return false;

    return IsItemProcessable(scene, *item);
}

bool WorkTableLogic::IsItemProcessable(Scene& scene, const GameObject& item) const
{
    // Base implementation: allow any item.
    // Later, you can override this in derived classes or update this to check
    // ingredient type, e.g. via an IngredientLogic component or tag.

        // Try to get IngredientLogic attached to this item.
    //
    // NOTE: This assumes you have something like:
    //   template<typename T>
    //   T* Scene::GetLogicForObject(int objectID);
    //
    // If your actual API is different, just swap this one line accordingly.
    IngredientLogic* ing = scene.GetLogicManager().GetLogicForObject<IngredientLogic>(item.GetID());
    if (!ing)
    {
        // Not an ingredient – this table doesn’t know how to process it.
        return false;
    }

    // Delegate to the helper: only raw ingredients are worth processing.
    return CanProcessIngredient(*ing);
}

// ------------------- Processing control -------------------

void WorkTableLogic::StartProcessing(Scene& scene)
{
    if (!HasItem())
        return;

    GameObject* item = scene.GetGameObjectByID(GetHeldItemID());
    if (!item)
        return;

    if (!IsItemProcessable(scene, *item))
        return;

    isProcessing_ = true;
    timer_ = 0.0f;
}

void WorkTableLogic::CancelProcessing(Scene& scene)
{
    if (isProcessing_) {
        // Stop station-specific processing sound when cancelled (release mode only)
#ifndef _DEBUG
        if (AudioManager* audioMgr = scene.GetAudioManager()) {
            const char* soundName = GetProcessingSoundName();
            if (soundName && audioMgr->HasSound(soundName)) {
                audioMgr->StopSound(soundName);
            }
        }
#endif
    }
    isProcessing_ = false;
    timer_ = 0.0f;
#ifdef _DEBUG
    (void)scene;
#endif
}

// ------------------- TableLogic hooks -------------------

void WorkTableLogic::OnItemPlaced(Scene& scene, GameObject& item)
{
    //IngredientLogic* ing = scene.GetLogicManager().GetLogicForObject<IngredientLogic>(item.GetID());
    //std::cout << "[WorkTable] placed item=" << item.GetID()
    //    << " hasIngredientLogic=" << (ing ? "YES" : "NO") << "\n";

    CancelProcessing(scene); // always reset
    if (IsItemProcessable(scene, item)) {
        isProcessing_ = true;
        timer_ = 0.0f;
        
        // Play station-specific processing sound (release mode only)
#ifndef _DEBUG
        if (AudioManager* audioMgr = scene.GetAudioManager()) {
            const char* soundName = GetProcessingSoundName();
            if (soundName && audioMgr->HasSound(soundName)) {
                audioMgr->PlaySound(soundName, audioMgr->GetVfxVolume(), false);
                // Lower volume specifically for cutting board sound
                if (stationType_ == StationType::CuttingBoard) {
                    audioMgr->SetVolume(soundName, audioMgr->GetVfxVolume() * 0.1f);
                }
            }
        }
#endif
    }
}

void WorkTableLogic::OnItemTaken(Scene& scene, GameObject& item)
{
    (void)item;
    // If the player removes the item mid-process, cancel.
    if (isProcessing_)
    {
        //std::cout << "[WorkTableLogic] Item " << item.GetID()
        //    << " TAKEN while still processing! t=" << processingTime_
        //    << "/" << timer_ << "\n";
        CancelProcessing(scene);
    }
}

// ------------------- Processing complete hook -------------------

void WorkTableLogic::OnProcessingComplete(Scene& scene, GameObject& item)
{
    // Base implementation: do nothing.
    // Example for a future derived table:
    //
    //   void CuttingBoardLogic::OnProcessingComplete(Scene& scene, GameObject& item) override {
    //       auto* ingredient = scene.GetLogic<IngredientLogic>(item.GetID());
    //       if (ingredient) ingredient->MarkProcessed();
    //   }

        // PSEUDO: you will adapt this to your actual logic lookup
    // IngredientLogic* ing = scene.GetLogicForObject<IngredientLogic>(item.GetID());
    // if (ing)
    //     CompleteProcessingForIngredient(*ing);

        // Look up the IngredientLogic for this item.
    IngredientLogic* ing = scene.GetLogicManager().GetLogicForObject<IngredientLogic>(item.GetID());
    if (!ing)
    {
        //// Not an ingredient – nothing to do.
        //std::cout << "[WorkTableLogic] OnProcessingComplete: item "
        //    << item.GetID() << " has no IngredientLogic\n";
        return;
    }

    // If your rule is “only raw gets processed”, respect that:
    if (!CanProcessIngredient(*ing))
    {
        //std::cout << "[WorkTableLogic] OnProcessingComplete: ingredient "
        //    << item.GetID() << " is not processable\n";
        return;
    }

    // Remember RAW type before MarkProcessed changes it
    IngredientType rawType = ing->GetType();

    // Update logic (raw -> refined)
    CompleteProcessingForIngredient(*ing);

    // Update sprite based on what was cooked
    const char* texPath = GetProcessedTextureForRaw(rawType);
    item.SetTexture(ResourceManager::Instance().LoadTexture(texPath, texPath));
    scene.SetObjectTexturePath(item.GetID(), texPath);

    // This is where the magic happens:
    //  - IngredientLogic::MarkProcessed()
    //  - internally flips Vegetable -> Refined_Veg, Meat -> Refined_Meat, etc.
    CompleteProcessingForIngredient(*ing);

    //std::cout << "[WorkTableLogic] Finished processing item " << item.GetID()
    //    << ", new type=" << static_cast<int>(ing->GetType()) << "\n";
}

bool WorkTableLogic::CanProcessIngredient(const IngredientLogic& ingredient) const
{
    if (!ingredient.IsRaw())
        return false;

    // Restrict by station
    switch (stationType_)
    {
    case StationType::CuttingBoard: return ingredient.GetType() == IngredientType::Vegetable;
    case StationType::Grill:        return ingredient.GetType() == IngredientType::Meat;
    case StationType::Stove:        return ingredient.GetType() == IngredientType::Shroom;
    default:                        return true; // Generic accepts any raw ingredient
    }
}

bool WorkTableLogic::ProcessIngredientInstant(IngredientLogic& ingredient)
{
    if (!CanProcessIngredient(ingredient))
        return false;

    CompleteProcessingForIngredient(ingredient);
    return true;
}

void WorkTableLogic::CompleteProcessingForIngredient(IngredientLogic& ingredient)
{
    // This is the actual "logic" of processing:
    // raw -> refined, via IngredientLogic.
    ingredient.MarkProcessed();
}
