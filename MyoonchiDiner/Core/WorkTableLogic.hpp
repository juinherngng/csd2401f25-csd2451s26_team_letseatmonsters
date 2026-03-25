/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         WorkTableLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu   (90%)
 CO-AUTHOR:         Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:       Declares CustomerManagerSystem, a simple scene-level system that pairs
					SimpleNpcLogic "customers" with CustomerTableLogic tables and assigns them
					seat targets once per scene.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */
#pragma once

#include "IngredientLogic.hpp"
#include "TableLogic.hpp"

 // WorkTableLogic
 //  - Base class for any table that processes ingredients.
 //  - Holds exactly one item (via TableLogic).
 //  - Runs a simple timer when an item is placed, and calls OnProcessingComplete
 //    when finished, so derived classes can implement their own processing rules.
 //
 // Example future specializations:
 //   class CuttingBoardLogic : public WorkTableLogic { ... }
 //   class FryingPanLogic   : public WorkTableLogic { ... }
class WorkTableLogic : public TableLogic {
public:
	// Constructor takes ownerID and passes to base TableLogic.
	explicit WorkTableLogic(int ownerID);

	// Called once when the scene starts (after Awake). Default implementation does nothing.
	void Start(Scene& scene) override;
	void OnDestroy(Scene& scene) override;

	// Called every frame by the logic system.
	void Update(float dt, Scene& scene, InputManager&) override;

	// Only accept items that are processable and when not already processing.
	bool CanAcceptItem(Scene& scene, int itemID) const override;

	// External control / queries
	bool  IsProcessing() const {
		return isProcessing_;
	}
	float GetProcessingTime() const {
		return processingTime_;
	}
	float GetProcessingElapsed() const {
		return timer_;
	}
	float GetProcessingProgress() const; // 0..1 (clamped), or 0 if not processing.

	// Manually start or cancel processing (if you want external control).
	void StartProcessing(Scene& scene);
	void CancelProcessing(Scene& scene);

	// Quick checks when you already know you're dealing with an ingredient:
	bool CanProcessIngredient(const IngredientLogic& ingredient) const;

	// Convenience: immediately mark an ingredient as processed, bypassing the timer.
	// Useful for testing or for instant-process tables.
	bool ProcessIngredientInstant(IngredientLogic& ingredient);
	// WorkTableLogic.hpp
	bool LocksPlayerMovementWhileProcessing() const {
		return stationType_ == StationType::CuttingBoard;
	}


protected:
	enum class StationType {
		CuttingBoard, // veg
		Grill,        // meat
		Stove,        // shroom
		Generic
	};

	// You can set this in the constructor of derived classes to customize behavior based on station type.
	StationType stationType_ = StationType::Generic;

	//Helper to determine station type based on texture path (if you want to auto-assign based on visuals).
	StationType DetectStationTypeFromTexture(const std::string& texPath) const;
	const char* GetProcessedTextureForRaw(IngredientType rawType) const;
	const char* GetProcessingSoundName() const;

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

	// Internal state for processing timer.
	bool  isProcessing_ = false;
	float processingTime_ = 3.0f;  // seconds needed to process an ingredient
	float timer_ = 0.0f;

	std::string GetName() const override {
		return "WorkTableLogic";
	}

	int vfxObjectID_ = -1;
	glm::vec2 vfxOffset_{ -1.0f, -37.0f }; // tweak per station if needed

	void SpawnProcessingVfx(Scene& scene);
	void DespawnProcessingVfx(Scene& scene);
	void UpdateProcessingVfxTransform(Scene& scene);

	const char* GetVfxTextureForStation() const;
	const char* GetVfxTagForStation() const;

	void EnsureCookingTimerBar(Scene& scene);
	void DestroyCookingTimerBar(Scene& scene);
	void FollowCookingTimerBar(Scene& scene);
	void UpdateCookingTimerFill(Scene& scene, float ratio01);

	int timerBarBG_ID_ = -1;
	int timerBarFill_ID_ = -1;

	std::string timerBarLayerBG_ = "50";
	std::string timerBarLayerTop_ = "51";

	glm::vec2 timerBarOffset_ = { 0.f, 58.f };     // a few pixels below workstation
	glm::vec2 timerBarBGSize_ = { 120.f, 14.f };
	glm::vec2 timerBarFillSize_ = { 112.f, 10.f };

	const char* timerBarBGPath_ = "../assets/Customer_Timer_Red.png";
	const char* timerBarFillPath_ = "../assets/Customer_Timer_Green.png";
};

