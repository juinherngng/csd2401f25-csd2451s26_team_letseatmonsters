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
    ClearApproachOffsets();

    AddApproachOffset(Math::Vector2D(0.0f, 110.0f));
}

void WorkTableLogic::Start(Scene& scene)
{
    std::cout << "[WorkTableLogic] Start ownerID=" << GetOwnerID() << "\n";

    ClearApproachOffsets();

    AddApproachOffset(Math::Vector2D(0.0f, 110.0f));
}

// ------------------- Update -------------------

void WorkTableLogic::Update(float dt, Scene& scene, InputManager&)
{
    // Only do anything if we have an item and are currently processing
    if (!isProcessing_ || !HasItem())
        return;

    timer_ += dt;

    std::cout << "[WorkTableLogic] processing... t=" << timer_
        << "/" << processingTime_ << "\n";

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

    std::cout << "[WorkTableLogic] Started processing item " << item.GetID()
        << " on table " << GetOwnerID() << "\n";
}

void WorkTableLogic::OnItemTaken(Scene& scene, GameObject& item)
{
    // If the player removes the item mid-process, cancel.
    if (isProcessing_)
    {
        std::cout << "[WorkTableLogic] Item " << item.GetID()
            << " TAKEN while still processing! t=" << processingTime_
            << "/" << timer_ << "\n";
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
        // Not an ingredient – nothing to do.
        std::cout << "[WorkTableLogic] OnProcessingComplete: item "
            << item.GetID() << " has no IngredientLogic\n";
        return;
    }

    // If your rule is “only raw gets processed”, respect that:
    if (!CanProcessIngredient(*ing))
    {
        std::cout << "[WorkTableLogic] OnProcessingComplete: ingredient "
            << item.GetID() << " is not processable\n";
        return;
    }

    // 2) Swap the sprite to the cut cabbage texture
//    (for now we assume this table is a cutting board for vegetables).
    item.SetTexture(
        ResourceManager::Instance().LoadTexture(
            "../assets/Cabbage_CUT_Ingredient.png",
            "../assets/Cabbage_CUT_Ingredient.png"
        )
    );

    // This is where the magic happens:
    //  - IngredientLogic::MarkProcessed()
    //  - internally flips Vegetable -> Refined_Veg, Meat -> Refined_Meat, etc.
    CompleteProcessingForIngredient(*ing);

    std::cout << "[WorkTableLogic] Finished processing item " << item.GetID()
        << ", new type=" << static_cast<int>(ing->GetType()) << "\n";
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
