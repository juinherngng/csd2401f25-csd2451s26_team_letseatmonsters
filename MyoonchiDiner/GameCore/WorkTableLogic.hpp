/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         WorkTableLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu   (70%)
 CO-AUTHORS:        Ng Juin Herng, juinherng.ng@digipen.edu (10%)
					Yat Chun Wee, y.chunwee@digipen.edu	    (20%)

 DESCRIPTION:       Declares CustomerManagerSystem, a simple scene-level system that pairs
					SimpleNpcLogic "customers" with CustomerTableLogic tables and assigns them
					seat targets once per scene.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <glm/vec2.hpp>

#include "GameCore/IngredientLogic.hpp"
#include "GameCore/TableLogic.hpp"

 /**
  * @brief Shared base logic for ingredient-processing workstations.
  * @details
  * Extends `TableLogic` with processing timers, station typing, cooking VFX,
  * timer bars, and item restrictions so authored work tables can refine raw
  * ingredients into processed versions.
  */
class WorkTableLogic : public TableLogic {
public:
	/**
	 * @brief Constructs workstation logic for the owning scene object.
	 * @param ownerID Runtime object ID that owns this logic component.
	 */
	explicit WorkTableLogic(int ownerID);

	/**
	 * @brief Initializes station type and processing duration from authored data.
	 * @param scene Active scene containing the workstation object.
	 */
	void Start(Scene& scene) override;

	/**
	 * @brief Stops active effects and cleans up spawned workstation UI before destruction.
	 * @param scene Active scene containing the workstation object.
	 */
	void OnDestroy(Scene& scene) override;

	/**
	 * @brief Updates processing timers, VFX, and timer-bar visuals for one frame.
	 * @param dt Delta time for the frame.
	 * @param scene Active scene containing the workstation and held item.
	 * @param input Unused input manager forwarded by the logic system.
	 */
	void Update(float dt, Scene& scene, InputManager&) override;

	/**
	 * @brief Returns whether the workstation can accept the specified item.
	 * @param scene Active scene containing the workstation and candidate item.
	 * @param itemID Runtime ID of the item being tested.
	 * @return True when the base table rules pass and the item is processable here.
	 */
	bool CanAcceptItem(Scene& scene, int itemID) const override;

	/**
	 * @brief Refreshes held-item tracking and recovers nearby occupants when possible.
	 * @param scene Active scene containing the workstation and item objects.
	 */
	void RefreshHeldItemState(Scene& scene) override;

	/**
	 * @brief Returns whether this workstation is currently processing an item.
	 * @return True when the processing timer is active.
	 */
	bool  IsProcessing() const {
		// Expose the active processing flag so player interaction can lock or unlock the station.
		return isProcessing_;
	}

	/**
	 * @brief Returns the configured processing duration for this station.
	 * @return Processing duration in seconds.
	 */
	float GetProcessingTime() const {
		// Surface the station-specific cook time for UI and balancing systems.
		return processingTime_;
	}

	/**
	 * @brief Returns the elapsed time on the current processing cycle.
	 * @return Elapsed processing time in seconds.
	 */
	float GetProcessingElapsed() const {
		// Expose the raw timer so callers can build custom progress displays if needed.
		return timer_;
	}

	/**
	 * @brief Returns the normalized processing progress for the current cycle.
	 * @return Progress in the range `[0, 1]`, or `0` when idle.
	 */
	float GetProcessingProgress() const;

	/**
	 * @brief Starts processing manually when the held item is valid for this station.
	 * @param scene Active scene containing the workstation and held item.
	 */
	void StartProcessing(Scene& scene);

	/**
	 * @brief Cancels the active processing cycle and clears transient visuals.
	 * @param scene Active scene containing the workstation object.
	 */
	void CancelProcessing(Scene& scene);

	/**
	 * @brief Returns whether a specific ingredient may be processed at this station.
	 * @param ingredient Ingredient logic being tested.
	 * @return True when the ingredient matches the station's raw-input rules.
	 */
	bool CanProcessIngredient(const IngredientLogic& ingredient) const;

	/**
	 * @brief Processes an ingredient immediately, bypassing the timer.
	 * @param ingredient Ingredient logic to mark as processed.
	 * @return True when the ingredient was valid and was processed successfully.
	 */
	bool ProcessIngredientInstant(IngredientLogic& ingredient);

	/**
	 * @brief Returns whether this station should lock player movement while processing.
	 * @return True for stations that require the player to remain engaged.
	 */
	bool LocksPlayerMovementWhileProcessing() const {
		// Cutting boards currently keep the player committed while the chop interaction runs.
		return stationType_ == StationType::CuttingBoard;
	}

	/**
	* @brief Returns whether the current held item may be taken from this station.
	* Raw / still-processing ingredients are locked on the station until finished.
	* @param scene Scene being processed.
	* @return True when the station's current item is allowed to be removed.
	*/
	bool CanTakeHeldItem(Scene& scene) const;

protected:
	enum class StationType {
		CuttingBoard, // veg
		Grill,        // meat
		Stove,        // shroom
		Generic
	};

	// Derived workstations can override the detected type when they need custom behavior.
	StationType stationType_ = StationType::Generic;

	/**
	 * @brief Infers the workstation type from the authored texture path.
	 * @param texPath Texture path assigned to the workstation.
	 * @return Detected station type.
	 */
	StationType DetectStationTypeFromTexture(const std::string& texPath) const;

	/**
	 * @brief Returns the processed texture path that matches a raw ingredient type.
	 * @param rawType Raw ingredient type before processing.
	 * @return Texture path for the processed ingredient variant.
	 */
	const char* GetProcessedTextureForRaw(IngredientType rawType) const;

	/**
	 * @brief Returns the processing-loop sound name for the current station type.
	 * @return Audio event name, or `nullptr` when no sound should play.
	 */
	const char* GetProcessingSoundName() const;

	/**
	 * @brief Starts processing-side effects when a new item is placed onto the station.
	 * @param scene Active scene containing the workstation and placed item.
	 * @param item Item that was just placed.
	 */
	void OnItemPlaced(Scene& scene, GameObject& item) override;

	/**
	 * @brief Cancels processing-side effects when the held item is removed.
	 * @param scene Active scene containing the workstation and removed item.
	 * @param item Item that was just taken.
	 */
	void OnItemTaken(Scene& scene, GameObject& item) override;

	/**
	 * @brief Completes processing for the held item after the timer finishes.
	 * @param scene Active scene containing the workstation and held item.
	 * @param item Item whose processing just completed.
	 */
	virtual void OnProcessingComplete(Scene& scene, GameObject& item);

	/**
	 * @brief Returns whether a placed object is processable by this workstation.
	 * @param scene Active scene containing the workstation and candidate item.
	 * @param item Candidate item object.
	 * @return True when the item resolves to a valid raw ingredient for this station.
	 */
	virtual bool IsItemProcessable(Scene& scene, const GameObject& item) const;

	/**
	 * @brief Applies the final raw-to-processed state change to an ingredient.
	 * @param ingredient Ingredient logic to mutate.
	 */
	void CompleteProcessingForIngredient(IngredientLogic& ingredient);

	// Internal state for processing timer.
	bool  isProcessing_ = false;
	float processingTime_ = 3.0f;  // seconds needed to process an ingredient
	float timer_ = 0.0f;

	/**
	 * @brief Returns the runtime logic name used for debugging and registration.
	 * @return Stable logic name string.
	 */
	std::string GetName() const override {
		// Keep the logic name stable so workstation logs and logic lookups stay readable.
		return "WorkTableLogic";
	}

	struct ProcessingVfxTuning {
		glm::vec2 offset{ 0.0f, 0.0f };
		glm::vec2 size{ 0.0f, 0.0f };
	};

	int vfxObjectID_ = -1;

	/**
	 * @brief Spawns the station's processing VFX if one is not already active.
	 * @param scene Active scene containing the workstation.
	 */
	void SpawnProcessingVfx(Scene& scene);

	/**
	 * @brief Removes any active processing VFX owned by this station.
	 * @param scene Active scene containing the workstation.
	 */
	void DespawnProcessingVfx(Scene& scene);

	/**
	 * @brief Keeps the processing VFX aligned to the workstation each frame.
	 * @param scene Active scene containing the workstation and VFX object.
	 */
	void UpdateProcessingVfxTransform(Scene& scene);

	/**
	 * @brief Returns the position and size tuning appropriate for the current station type.
	 * @return Offset/size data for the workstation VFX sprite.
	 */
	ProcessingVfxTuning GetProcessingVfxTuning() const;

	/**
	 * @brief Returns the VFX texture path for the current station type.
	 * @return Texture path used to spawn the workstation VFX.
	 */
	const char* GetVfxTextureForStation() const;

	/**
	 * @brief Returns the tag used to attach the correct VFX animation set.
	 * @return Tag string for the spawned workstation VFX object.
	 */
	const char* GetVfxTagForStation() const;

	/**
	 * @brief Spawns the timer-bar UI used to show processing progress.
	 * @param scene Active scene containing the workstation.
	 */
	void EnsureCookingTimerBar(Scene& scene);

	/**
	 * @brief Removes the timer-bar UI owned by this station.
	 * @param scene Active scene containing the workstation.
	 */
	void DestroyCookingTimerBar(Scene& scene);

	/**
	 * @brief Keeps the timer-bar UI aligned to the workstation each frame.
	 * @param scene Active scene containing the workstation and timer bar.
	 */
	void FollowCookingTimerBar(Scene& scene);

	/**
	 * @brief Updates the width and position of the timer-bar fill sprite.
	 * @param scene Active scene containing the timer-bar objects.
	 * @param ratio01 Remaining-fill ratio in the range `[0, 1]`.
	 */
	void UpdateCookingTimerFill(Scene& scene, float ratio01);

	/**
	 * @brief Starts the looping processing sound for this station.
	 * @param scene Active scene containing the workstation and audio manager.
	 */
	void StartProcessingSound(Scene& scene);

	/**
	 * @brief Stops the active processing sound for this station.
	 * @param scene Active scene containing the workstation and audio manager.
	 */
	void StopProcessingSound(Scene& scene);
	bool processingSoundActive_ = false;

	int timerBarBG_ID_ = -1;
	int timerBarFill_ID_ = -1;

	std::string timerBarLayerBG_ = "50";
	std::string timerBarLayerTop_ = "51";

	glm::vec2 timerBarOffset_ = { 0.f, 58.f };     // a few pixels below workstation
	glm::vec2 timerBarBGSize_ = { 120.f, 14.f };
	glm::vec2 timerBarFillSize_ = { 112.f, 10.f };

	const char* timerBarBGPath_ = "../assets/UI/Customer_Timer_Red.png";
	const char* timerBarFillPath_ = "../assets/UI/Customer_Timer_Green.png";
};
