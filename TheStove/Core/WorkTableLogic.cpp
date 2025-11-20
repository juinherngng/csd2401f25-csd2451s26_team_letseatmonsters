#include "WorkTableLogic.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Graphics/GameObject.hpp"

// ------------------- Constructor -------------------

WorkTableLogic::WorkTableLogic(int ownerID)
    : TableLogic(ownerID)
    , isProcessing_(false)
    , processingTime_(3.0f)   // default: 3 seconds to process
    , timer_(0.0f)
{
}

// ------------------- Update -------------------

void WorkTableLogic::Update(float dt, Scene& scene, InputManager&)
{
    // Only do anything if we have an item and are currently processing
    if (!isProcessing_ || !HasItem())
        return;

    timer_ += dt;
    if (timer_ >= processingTime_)
    {
        timer_ = processingTime_;
        isProcessing_ = false;

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

    // Do not accept new items while already processing something.
    if (isProcessing_)
        return false;

    GameObject* item = scene.GetGameObjectByID(itemID);
    if (!item)
        return false;

    return IsItemProcessable(scene, *item);
}

bool WorkTableLogic::IsItemProcessable(Scene& /*scene*/, const GameObject& /*item*/) const
{
    // Base implementation: allow any item.
    // Later, you can override this in derived classes or update this to check
    // ingredient type, e.g. via an IngredientLogic component or tag.
    return true;
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

void WorkTableLogic::CancelProcessing(Scene& /*scene*/)
{
    isProcessing_ = false;
    timer_ = 0.0f;
}

// ------------------- TableLogic hooks -------------------

void WorkTableLogic::OnItemPlaced(Scene& scene, GameObject& item)
{
    // Auto-start processing when an item is placed.
    if (!isProcessing_ && IsItemProcessable(scene, item))
    {
        isProcessing_ = true;
        timer_ = 0.0f;
    }
}

void WorkTableLogic::OnItemTaken(Scene& scene, GameObject& /*item*/)
{
    // If the player removes the item mid-process, cancel.
    CancelProcessing(scene);
}

// ------------------- Processing complete hook -------------------

void WorkTableLogic::OnProcessingComplete(Scene& /*scene*/, GameObject& /*item*/)
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
}

bool WorkTableLogic::CanProcessIngredient(const IngredientLogic& ingredient) const
{
    // Base rule: only raw ingredients are meaningful to process.
    // You can relax this if you want "double processing".
    return ingredient.IsRaw();
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
