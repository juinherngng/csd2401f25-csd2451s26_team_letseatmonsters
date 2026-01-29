/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         WorkTableLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung

 DESCRIPTION:       Declares CustomerManagerSystem, a simple scene-level system that pairs
                    SimpleNpcLogic "customers" with CustomerTableLogic tables and assigns them
                    seat targets once per scene.

         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */
#pragma once

#include "TableLogic.hpp"
#include "IngredientLogic.hpp"

// WorkTableLogic
//  - Base class for any table that processes ingredients.
//  - Holds exactly one item (via TableLogic).
//  - Runs a simple timer when an item is placed, and calls OnProcessingComplete
//    when finished, so derived classes can implement their own processing rules.
//
// Example future specializations:
//   class CuttingBoardLogic : public WorkTableLogic { ... }
//   class FryingPanLogic   : public WorkTableLogic { ... }
class WorkTableLogic : public TableLogic
{
public:
    explicit WorkTableLogic(int ownerID);

    void Start(Scene& scene) override;

    // Called every frame by the logic system.
    void Update(float dt, Scene& scene, InputManager&) override;

    // Only accept items that are processable and when not already processing.
    bool CanAcceptItem(Scene& scene, int itemID) const override;

    // External control / queries
    bool  IsProcessing() const { return isProcessing_; }
    float GetProcessingTime() const { return processingTime_; }
    float GetProcessingElapsed() const { return timer_; }
    float GetProcessingProgress() const; // 0..1 (clamped), or 0 if not processing.

    // Manually start or cancel processing (if you want external control).
    void StartProcessing(Scene& scene);
    void CancelProcessing(Scene& scene);

    // Quick checks when you already know you're dealing with an ingredient:
    bool CanProcessIngredient(const IngredientLogic& ingredient) const;

    // Convenience: immediately mark an ingredient as processed, bypassing the timer.
    // Useful for testing or for instant-process tables.
    bool ProcessIngredientInstant(IngredientLogic& ingredient);


protected:
        enum class StationType
    {
        CuttingBoard, // veg
        Grill,        // meat
        Stove,        // shroom
        Generic
    };

    StationType stationType_ = StationType::Generic;

    StationType DetectStationTypeFromTexture(const std::string& texPath) const;
    const char* GetProcessedTextureForRaw(IngredientType rawType) const;

    // Hooks from TableLogic when item changes.
    void OnItemPlaced(Scene& scene, GameObject& item) override;
    void OnItemTaken(Scene& scene, GameObject& item) override;

    // Called when the processing timer completes.
    // Default implementation does nothing; derived work-table types override
    // this to actually change the ingredient (e.g., mark as processed).
    virtual void OnProcessingComplete(Scene& scene, GameObject& item);

    // Base filter: is this item allowed to be processed?
    // For now, this returns true for any item; later you can refine it to check
    // for IngredientLogic or some tag/type.
    virtual bool IsItemProcessable(Scene& scene, const GameObject& item) const;

    // Optional helper that derived tables (e.g. Stove, CuttingBoard) can call
    // when the timer finishes and they have direct access to the ingredient logic.
    void CompleteProcessingForIngredient(IngredientLogic& ingredient);

    bool  isProcessing_ = false;
    float processingTime_ = 3.0f;  // seconds needed to process an ingredient
    float timer_ = 0.0f;

    std::string GetName() const override { return "WorkTableLogic"; }
};
