/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         WorkTableLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu   (70%)
 CO-AUTHORS:        Ng Juin Herng, juinherng.ng@digipen.edu (20%)
					Yat Chun Wee, y.chunwee@digipen.edu	    (10%)

 DESCRIPTION:       Implements WorkTableLogic, the type of table that accepts raw
					ingredients, processes them into refined ingredients, and allows
					players to interact with workstations for cooking or preparation.
					This class overrides base TableLogic behavior to restrict what
					items can be placed, manage processing states, and output the
					refined ingredient when complete.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <glm/vec4.hpp>
#include <limits>
#include <unordered_map>

#include "EngineCore/AudioManager.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/PlayerLogic.hpp"
#include "GameCore/WorkTableLogic.hpp"

 /**
  * @brief Returns whether a string contains a substring.
  * @param s Source string to inspect.
  * @param sub Substring to search for.
  * @return True when `sub` appears inside `s`.
  */
static bool Contains(const std::string& s, const char* sub) {
	return s.find(sub) != std::string::npos;
}

/**
 * @brief Despawns an object if its ID still refers to a live scene object.
 * @param scene Active scene containing the object.
 * @param id In-out object ID to despawn and invalidate.
 */
static void DespawnIfAlive(Scene& scene, int& id) {
	if (id >= 0) {
		scene.DespawnByID(id);
		id = -1;
	}
}

/**
 * @brief Clamps a scalar to the normalized range `[0, 1]`.
 * @param v Value to clamp.
 * @return Clamped normalized value.
 */
static float Clamp01Value(float v) {
	if (v < 0.f) return 0.f;
	if (v > 1.f) return 1.f;
	return v;
}

/**
 * @brief Returns an object's world position when it exists in the scene.
 * @param scene Active scene containing the object.
 * @param objectID Runtime ID of the object to inspect.
 * @param outPos Output world position.
 * @return True when the object exists and `outPos` was filled.
 */
static bool TryGetObjectWorldPos(Scene& scene, int objectID, glm::vec3& outPos) {
	GameObject* obj = scene.GetGameObjectByID(objectID);
	if (!obj) {
		return false;
	}

	outPos = obj->GetPositionGLM();
	return true;
}

static constexpr float kRecoveredStationOccupantMaxDistSq = 18.0f * 18.0f;

/**
 * @brief Infers the workstation type from the authored workstation texture.
 * @param texPath Texture path assigned to the workstation object.
 * @return Detected station type.
 */
WorkTableLogic::StationType WorkTableLogic::DetectStationTypeFromTexture(const std::string& texPath) const {
	// Detect by the workstation sprite (the table's texture)
	if (Contains(texPath, "Cutting_Board")) return StationType::CuttingBoard;
	if (Contains(texPath, "Grills"))        return StationType::Grill;
	if (Contains(texPath, "Stove"))         return StationType::Stove;
	return StationType::Generic;
}

/**
 * @brief Returns the processed texture that matches a raw ingredient type.
 * @param rawType Raw ingredient type before processing.
 * @return Texture path for the processed ingredient variant.
 */
const char* WorkTableLogic::GetProcessedTextureForRaw(IngredientType rawType) const {
	// IMPORTANT: Replace these 2 paths with your actual cooked meat/shroom assets.
	switch (rawType) {
	case IngredientType::Vegetable: return "../assets/Food/Cabbage_CUT_Ingredient.png";
	case IngredientType::Meat:      return "../assets/Food/Meat_CUT_Ingredient.png";
	case IngredientType::Shroom:    return "../assets/Food/Mushroom_CUT_Ingredient.png";
	case IngredientType::Carrot:    return "../assets/Food/CUT_Carrot_Ingredient.png";
	default:                        return "../assets/Food/Cabbage_CUT_Ingredient.png";
	}
}

/**
 * @brief Returns the looping processing sound name for the current station type.
 * @return Audio event name, or `nullptr` when no looping sound should play.
 */
const char* WorkTableLogic::GetProcessingSoundName() const {
	switch (stationType_) {
	case StationType::CuttingBoard: return "sfx_chopping";
	case StationType::Grill:        return "sfx_grilling_sizzle";
	case StationType::Stove:        return "sfx_boiling_sound";
	default:                        return nullptr;
	}
}

/**
 * @brief Constructs workstation logic for the owning scene object.
 * @param ownerID Runtime object ID that owns this logic component.
 */
WorkTableLogic::WorkTableLogic(int ownerID) : TableLogic(ownerID) {
	// Station type and processing speed are resolved from authored scene data during Start().
}

/**
 * @brief Initializes station type and default processing duration from authored data.
 * @param scene Active scene containing the workstation object.
 */
void WorkTableLogic::Start(Scene& scene) {
	TableLogic::Start(scene);

	// Detect which workstation flavor this table represents from its authored texture.
	Scene::Defaults def = scene.GetDefaults(GetOwnerID());
	stationType_ = DetectStationTypeFromTexture(def.texture);

	// Seed a default processing duration for the station before per-item adjustments happen.
	switch (stationType_) {
	case StationType::CuttingBoard: processingTime_ = 3.0f; break;
	case StationType::Grill:        processingTime_ = 5.0f; break;
	case StationType::Stove:        processingTime_ = 7.0f; break;
	default:                        processingTime_ = 3.0f; break;
	}
}

/**
 * @brief Stops active audio, timer UI, and VFX before the workstation is destroyed.
 * @param scene Active scene containing the workstation object.
 */
void WorkTableLogic::OnDestroy(Scene& scene) {
	StopProcessingSound(scene);
	DestroyCookingTimerBar(scene);
	DespawnProcessingVfx(scene);
	TableLogic::OnDestroy(scene);
}

/**
 * @brief Updates processing timers, spawned VFX, and timer-bar UI for one frame.
 * @param dt Delta time for the frame.
 * @param scene Active scene containing the workstation and held item.
 * @param input Unused input manager forwarded by the logic system.
 */
void WorkTableLogic::Update(float dt, Scene& scene, InputManager&) {
	if (!scene.IsSimulationActive()) return;

	// Reconcile held-item state first so processing logic never runs on stale occupancy.
	RefreshHeldItemState(scene);

	if (!HasItem()) {
		if (isProcessing_) {
			CancelProcessing(scene);
		}
		DestroyCookingTimerBar(scene);
		return;
	}

	if (!isProcessing_) {
		DestroyCookingTimerBar(scene);
		return;
	}

	timer_ += dt;

	if (isProcessing_) {
		// Keep workstation feedback attached and synchronized while the timer is active.
		UpdateProcessingVfxTransform(scene);
		EnsureCookingTimerBar(scene);
		FollowCookingTimerBar(scene);
		UpdateCookingTimerFill(scene, 1.0f - GetProcessingProgress());
	}

	if (timer_ >= processingTime_) {
		timer_ = processingTime_;
		isProcessing_ = false;
		DespawnProcessingVfx(scene);
		DestroyCookingTimerBar(scene);

		// Stop station-specific processing sound when complete (release mode only)
		if (scene.ShouldUseRuntimeParityMode()) {
			StopProcessingSound(scene);
		}

		GameObject* item = scene.GetGameObjectByID(GetHeldItemID());
		if (item) {
			OnProcessingComplete(scene, *item);
		}
	}
}

/**
 * @brief Returns whether the held item may currently be removed from the workstation.
 * @param scene Active scene containing the workstation and held item.
 * @return True when the held item is processed and no active processing is running.
 */
bool WorkTableLogic::CanTakeHeldItem(Scene& scene) const {
	if (!HasItem()) {
		return false;
	}

	const int heldItemID = GetHeldItemID();
	GameObject* item = scene.GetGameObjectByID(heldItemID);
	if (!item) {
		return false;
	}

	// Main rule: while the workstation is actively processing,
	// the player is not allowed to take the ingredient back out.
	if (isProcessing_) {
		return false;
	}

	// Safety rule: even if processing was somehow cancelled or paused,
	// a RAW ingredient should still not be removable from a workstation.
	if (IngredientLogic* ing =
		scene.GetLogicManager().GetLogicForObject<IngredientLogic>(heldItemID)) {
		if (ing->IsRaw()) {
			return false;
		}
	}

	// Processed ingredient is allowed to be taken.
	return true;
}

/**
 * @brief Returns the normalized processing progress for the current cycle.
 * @return Progress in the range `[0, 1]`, or `0` when idle.
 */
float WorkTableLogic::GetProcessingProgress() const {
	if (!isProcessing_ || processingTime_ <= 0.0f)
		return 0.0f;

	float t = timer_ / processingTime_;
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	return t;
}

/**
 * @brief Returns whether the workstation can accept the specified item.
 * @param scene Active scene containing the workstation and candidate item.
 * @param itemID Runtime ID of the item being tested.
 * @return True when base table rules pass and the item is processable here.
 */
bool WorkTableLogic::CanAcceptItem(Scene& scene, int itemID) const {
	if (!TableLogic::CanAcceptItem(scene, itemID))
		return false;

	GameObject* item = scene.GetGameObjectByID(itemID);
	if (!item)
		return false;

	return IsItemProcessable(scene, *item);
}

/**
 * @brief Repairs held-item tracking and attempts to recover nearby station occupants.
 * @param scene Active scene containing the workstation and nearby ingredient objects.
 */
void WorkTableLogic::RefreshHeldItemState(Scene& scene) {
	TableLogic::RefreshHeldItemState(scene);

	if (heldItemID_ != kInvalidID) {
		return;
	}

	const Math::Vector3D anchor3 = GetItemPlacementPosition(scene);
	const glm::vec2 anchor(anchor3.x, anchor3.y);

	int carriedItemID = -1;
	if (PlayerLogic* playerLogic = scene.GetLogicManager().GetLogicForObject<PlayerLogic>(scene.GetPlayerID())) {
		// Ignore the player's carried item so the station does not accidentally reclaim it.
		carriedItemID = playerLogic->GetCarriedItemID();
	}

	int recoveredItemID = -1;
	float bestDistSq = std::numeric_limits<float>::max();
	IngredientLogic* recoveredIngredient = nullptr;

	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		if (!obj) {
			continue;
		}

		const int objectID = obj->GetID();
		if (objectID == GetOwnerID() || objectID == carriedItemID) {
			continue;
		}

		IngredientLogic* ingredient = scene.GetLogicManager().GetLogicForObject<IngredientLogic>(objectID);
		if (!ingredient) {
			continue;
		}

		if (!ingredient->IsProcessed() && !CanProcessIngredient(*ingredient)) {
			continue;
		}

		const glm::vec3 objectPos = obj->GetPositionGLM();
		const float dx = objectPos.x - anchor.x;
		const float dy = objectPos.y - anchor.y;
		const float distSq = dx * dx + dy * dy;
		if (distSq > kRecoveredStationOccupantMaxDistSq) {
			continue;
		}

		if (distSq < bestDistSq) {
			// Recover the closest plausible station occupant near the item anchor.
			bestDistSq = distSq;
			recoveredItemID = objectID;
			recoveredIngredient = ingredient;
		}
	}

	if (recoveredItemID < 0) {
		if (isProcessing_) {
			CancelProcessing(scene);
		}
		return;
	}

	heldItemID_ = recoveredItemID;

	if (!recoveredIngredient) {
		return;
	}

	if (recoveredIngredient->IsProcessed()) {
		if (isProcessing_) {
			CancelProcessing(scene);
		}
		return;
	}

	if (!isProcessing_) {
		if (GameObject* recoveredItem = scene.GetGameObjectByID(recoveredItemID)) {
			OnItemPlaced(scene, *recoveredItem);
		}
	}
}

/**
 * @brief Returns whether a specific object can be processed by this workstation.
 * @param scene Active scene containing the workstation and candidate item.
 * @param item Candidate item object.
 * @return True when the item resolves to a valid raw ingredient for this station.
 */
bool WorkTableLogic::IsItemProcessable(Scene& scene, const GameObject& item) const {
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
	if (!ing) {
		// Not an ingredient this table does not know how to process it.
		return false;
	}

	// Delegate to the helper: only raw ingredients are worth processing.
	return CanProcessIngredient(*ing);
}

/**
 * @brief Starts processing manually when the held item is valid for this station.
 * @param scene Active scene containing the workstation and held item.
 */
void WorkTableLogic::StartProcessing(Scene& scene) {
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

/**
 * @brief Cancels the active processing cycle and clears all transient feedback.
 * @param scene Active scene containing the workstation object.
 */
void WorkTableLogic::CancelProcessing(Scene& scene) {
	if (isProcessing_) {
		// Stop station-specific processing sound when cancelled (release mode only)
		if (scene.ShouldUseRuntimeParityMode()) {
			StopProcessingSound(scene);
		}
	}
	isProcessing_ = false;
	timer_ = 0.0f;
	DespawnProcessingVfx(scene);
	DestroyCookingTimerBar(scene);
}

/**
 * @brief Starts processing-side effects when an item is placed onto the station.
 * @param scene Active scene containing the workstation and placed item.
 * @param item Item that was just placed.
 */
void WorkTableLogic::OnItemPlaced(Scene& scene, GameObject& item) {
	CancelProcessing(scene); // always reset
	if (IsItemProcessable(scene, item)) {
		if (IngredientLogic* ing = scene.GetLogicManager().GetLogicForObject<IngredientLogic>(item.GetID())) {
			// Adjust processing duration from both station type and ingredient type.
			switch (stationType_) {
			case StationType::CuttingBoard: processingTime_ = 1.5f; break;
			case StationType::Grill:        processingTime_ = 5.0f; break;
			case StationType::Stove:        processingTime_ = (ing->GetType() == IngredientType::Carrot) ? 3.0f : 7.0f; break;
			default:                        processingTime_ = 3.0f; break;
			}
		}
		isProcessing_ = true;
		timer_ = 0.0f;
		SpawnProcessingVfx(scene);
		// Play station-specific processing sound (release mode only)
		if (scene.ShouldUseRuntimeParityMode()) {
			StartProcessingSound(scene);
		}
	}
}

/**
 * @brief Starts the looping processing sound for the current station type.
 * @param scene Active scene containing the workstation and audio manager.
 */
void WorkTableLogic::StartProcessingSound(Scene& scene) {
	if (processingSoundActive_) {
		return;
	}

	AudioManager* audioMgr = scene.GetAudioManager();
	if (!audioMgr) {
		return;
	}

	const char* soundName = GetProcessingSoundName();
	if (!soundName || !audioMgr->HasSound(soundName)) {
		return;
	}

	glm::vec3 worldPos{};
	if (TryGetObjectWorldPos(scene, GetOwnerID(), worldPos)) {
		audioMgr->PlaySound3D(
			soundName,
			worldPos.x,
			worldPos.y,
			0.0f,
			audioMgr->GetVfxVolume(),
			120.0f,
			1200.0f,
			false);
	}
	else {
		audioMgr->PlaySound(soundName, audioMgr->GetVfxVolume(), false);
	}

	if (stationType_ == StationType::CuttingBoard) {
		// Chopping is intentionally quieter than stove and grill ambience.
		audioMgr->SetVolume(soundName, audioMgr->GetVfxVolume() * 0.2f);
	}

	processingSoundActive_ = true;
}

/**
 * @brief Stops any looping processing sound currently owned by this workstation.
 * @param scene Active scene containing the workstation and audio manager.
 */
void WorkTableLogic::StopProcessingSound(Scene& scene) {
	if (!processingSoundActive_) {
		return;
	}

	processingSoundActive_ = false;

	AudioManager* audioMgr = scene.GetAudioManager();
	if (!audioMgr) {
		return;
	}

	const char* soundName = GetProcessingSoundName();
	if (!soundName) {
		return;
	}

	if (audioMgr->HasSound(soundName)) {
		audioMgr->StopOneSoundInstance(soundName);
	}
}

/**
 * @brief Cancels processing if the held item is removed from the workstation.
 * @param scene Active scene containing the workstation and removed item.
 * @param item Item that was just taken.
 */
void WorkTableLogic::OnItemTaken(Scene& scene, GameObject& item) {
	(void)item;
	// If the player removes the item mid-process, cancel.
	if (isProcessing_) {
		CancelProcessing(scene);
	}
}

/**
 * @brief Converts the held raw ingredient into its processed version when the timer ends.
 * @param scene Active scene containing the workstation and held item.
 * @param item Item whose processing just completed.
 */
void WorkTableLogic::OnProcessingComplete(Scene& scene, GameObject& item) {
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
	if (!ing) {
		return;
	}

	// If your rule is only raw gets processed, respect that:
	if (!CanProcessIngredient(*ing)) {
		return;
	}

	// Remember RAW type before MarkProcessed changes it
	IngredientType rawType = ing->GetType();

	// Update sprite based on what was cooked
	const char* texPath = GetProcessedTextureForRaw(rawType);
	item.SetTexture(ResourceManager::Instance().LoadTexture(texPath, texPath));
	scene.SetObjectTexturePath(item.GetID(), texPath);

	// This is where the magic happens:
	//  - IngredientLogic::MarkProcessed()
	//  - internally flips Vegetable -> Refined_Veg, Meat -> Refined_Meat, etc.
	CompleteProcessingForIngredient(*ing);

}

/**
 * @brief Returns whether a raw ingredient matches this station's accepted types.
 * @param ingredient Ingredient logic being tested.
 * @return True when the ingredient is raw and allowed at this station.
 */
bool WorkTableLogic::CanProcessIngredient(const IngredientLogic& ingredient) const {
	if (!ingredient.IsRaw())
		return false;

	// Restrict by station
	switch (stationType_) {
	case StationType::CuttingBoard: return ingredient.GetType() == IngredientType::Vegetable;
	case StationType::Grill:        return ingredient.GetType() == IngredientType::Meat;
	case StationType::Stove:        return ingredient.GetType() == IngredientType::Shroom || ingredient.GetType() == IngredientType::Carrot;
	default:                        return true; // Generic accepts any raw ingredient
	}
}

/**
 * @brief Processes an ingredient immediately without waiting for the station timer.
 * @param ingredient Ingredient logic to mark as processed.
 * @return True when the ingredient was valid and processed successfully.
 */
bool WorkTableLogic::ProcessIngredientInstant(IngredientLogic& ingredient) {
	if (!CanProcessIngredient(ingredient))
		return false;

	CompleteProcessingForIngredient(ingredient);
	return true;
}

/**
 * @brief Applies the raw-to-processed state transition to an ingredient.
 * @param ingredient Ingredient logic to mutate.
 */
void WorkTableLogic::CompleteProcessingForIngredient(IngredientLogic& ingredient) {
	// This is the actual "logic" of processing:
	// raw -> refined, via IngredientLogic.
	ingredient.MarkProcessed();
}

/**
 * @brief Returns the workstation VFX sheet for the current station type.
 * @return Texture path used for spawned processing VFX.
 */
const char* WorkTableLogic::GetVfxTextureForStation() const {
	switch (stationType_) {
	case StationType::CuttingBoard: return "../assets/VFX/VFX_SpriteSheet.png";
	case StationType::Grill:        return "../assets/VFX/VFX_SpriteSheet.png";
	case StationType::Stove:        return "../assets/VFX/VFX_SpriteSheet.png";
	default:                        return nullptr;
	}
}

/**
 * @brief Returns the tag used to attach the correct processing VFX animation set.
 * @return Tag string for the spawned workstation VFX object.
 */
const char* WorkTableLogic::GetVfxTagForStation() const {
	switch (stationType_) {
	case StationType::CuttingBoard: return "work_vfx_cut";
	case StationType::Grill:        return "work_vfx_grill";
	case StationType::Stove:        return "work_vfx_stove";
	default:                        return nullptr;
	}
}

/**
 * @brief Returns the station-specific position and size tuning for processing VFX.
 * @return Offset/size values for the current station type.
 */
WorkTableLogic::ProcessingVfxTuning WorkTableLogic::GetProcessingVfxTuning() const {
	constexpr ProcessingVfxTuning kCuttingBoardTuning{
		glm::vec2(-1.0f, -37.0f),
		glm::vec2(150.0f, 210.0f)
	};
	constexpr ProcessingVfxTuning kGrillTuning{
		glm::vec2(-1.0f, -57.0f),
		glm::vec2(150.0f, 210.0f)
	};
	constexpr ProcessingVfxTuning kStoveTuning{
		glm::vec2(-1.0f, -57.0f),
		glm::vec2(150.0f * 0.75f, 210.0f * 0.75f)
	};

	switch (stationType_) {
	case StationType::Grill:
		return kGrillTuning;
	case StationType::Stove:
		return kStoveTuning;
	case StationType::CuttingBoard:
	case StationType::Generic:
	default:
		return kCuttingBoardTuning;
	}
}

/**
 * @brief Spawns the processing VFX object for the active workstation.
 * @param scene Active scene containing the workstation.
 */
void WorkTableLogic::SpawnProcessingVfx(Scene& scene) {
	if (vfxObjectID_ >= 0) return;

	const char* tex = GetVfxTextureForStation();
	const char* tag = GetVfxTagForStation();
	if (!tex || !tag) return;

	GameObject* table = scene.GetGameObjectByID(GetOwnerID());
	if (!table) return;

	glm::vec3 tp = table->GetPositionGLM();
	const ProcessingVfxTuning tuning = GetProcessingVfxTuning();
	glm::vec3 vfxPos{ tp.x + tuning.offset.x, tp.y + tuning.offset.y, tp.z + 0.001f };

	// Create an animated sprite so AnimationManager can drive UVs
	std::vector<glm::vec4> dummyFrames = { glm::vec4(0.f, 0.f, 1.f, 1.f) };

	// Put it on a higher layer than the table (simple version: hardcode a top-ish layer)
	std::string vfxLayer = "50";

	GameObject* vfx = scene.SpawnAnimatedSprite(tex, vfxPos, tuning.size,
		dummyFrames, 0.1f, true, vfxLayer);

	if (!vfx) return;

	vfxObjectID_ = vfx->GetID();

	// Disable gameplay interaction so the VFX behaves like a pure visual attachment.
	vfx->SetColliderSize(Math::Vector2D(0.f, 0.f));
	vfx->SetColliderOffset(Math::Vector2D(0.f, 0.f));
	vfx->SetMovableByPhysics(false);
	vfx->EnableShadow(false);

	// Tag + attach ONLY animations (via Scene::AttachLogicForTag)
	scene.SetObjectTag(vfxObjectID_, tag);
	scene.AttachLogicForTag(vfxObjectID_, tag);
}

/**
 * @brief Removes any active processing VFX owned by this workstation.
 * @param scene Active scene containing the workstation.
 */
void WorkTableLogic::DespawnProcessingVfx(Scene& scene) {
	if (vfxObjectID_ < 0) return;
	scene.RequestDespawn(vfxObjectID_);
	vfxObjectID_ = -1;
}

/**
 * @brief Keeps the processing VFX aligned to the workstation each frame.
 * @param scene Active scene containing the workstation and VFX object.
 */
void WorkTableLogic::UpdateProcessingVfxTransform(Scene& scene) {
	if (vfxObjectID_ < 0) return;

	GameObject* table = scene.GetGameObjectByID(GetOwnerID());
	GameObject* vfx = scene.GetGameObjectByID(vfxObjectID_);
	if (!table || !vfx) return;

	glm::vec3 tp = table->GetPositionGLM();
	const ProcessingVfxTuning tuning = GetProcessingVfxTuning();
	vfx->SetPosition(glm::vec3(tp.x + tuning.offset.x, tp.y + tuning.offset.y, tp.z + 0.001f));
}

/**
 * @brief Spawns the background and fill sprites for the workstation timer bar.
 * @param scene Active scene containing the workstation.
 */
void WorkTableLogic::EnsureCookingTimerBar(Scene& scene) {
	GameObject* table = scene.GetGameObjectByID(GetOwnerID());
	if (!table) return;

	glm::vec3 p = table->GetPositionGLM();

	if (timerBarBG_ID_ < 0) {
		if (GameObject* bg = scene.SpawnStaticSprite(
			timerBarBGPath_,
			{ p.x + timerBarOffset_.x, p.y + timerBarOffset_.y, p.z },
			timerBarBGSize_,
			timerBarLayerBG_)) {
			timerBarBG_ID_ = bg->GetID();
			bg->SetColliderSize(Math::Vector2D(0.f, 0.f));
			scene.SetObjectTexturePath(timerBarBG_ID_, timerBarBGPath_);
		}
	}

	if (timerBarFill_ID_ < 0) {
		if (GameObject* fill = scene.SpawnStaticSprite(
			timerBarFillPath_,
			{ p.x + timerBarOffset_.x, p.y + timerBarOffset_.y, p.z },
			timerBarFillSize_,
			timerBarLayerTop_)) {
			timerBarFill_ID_ = fill->GetID();
			fill->SetColliderSize(Math::Vector2D(0.f, 0.f));
			scene.SetObjectTexturePath(timerBarFill_ID_, timerBarFillPath_);
		}
	}
}

/**
 * @brief Removes the workstation timer-bar sprites if they are active.
 * @param scene Active scene containing the workstation.
 */
void WorkTableLogic::DestroyCookingTimerBar(Scene& scene) {
	DespawnIfAlive(scene, timerBarFill_ID_);
	DespawnIfAlive(scene, timerBarBG_ID_);
}

/**
 * @brief Keeps the timer-bar background aligned to the workstation each frame.
 * @param scene Active scene containing the workstation and timer bar.
 */
void WorkTableLogic::FollowCookingTimerBar(Scene& scene) {
	GameObject* table = scene.GetGameObjectByID(GetOwnerID());
	if (!table) return;

	glm::vec3 p = table->GetPositionGLM();

	if (timerBarBG_ID_ >= 0) {
		if (GameObject* bg = scene.GetGameObjectByID(timerBarBG_ID_)) {
			bg->SetPosition(Math::Vector3D(p.x + timerBarOffset_.x, p.y + timerBarOffset_.y, p.z));
		}
	}
}

/**
 * @brief Updates the timer-bar fill width and anchoring from a normalized ratio.
 * @param scene Active scene containing the timer-bar sprites.
 * @param ratio01 Remaining-fill ratio in the range `[0, 1]`.
 */
void WorkTableLogic::UpdateCookingTimerFill(Scene& scene, float ratio01) {
	if (timerBarBG_ID_ < 0 || timerBarFill_ID_ < 0) return;

	GameObject* bg = scene.GetGameObjectByID(timerBarBG_ID_);
	GameObject* fill = scene.GetGameObjectByID(timerBarFill_ID_);
	if (!bg || !fill) return;

	ratio01 = Clamp01Value(ratio01);
	glm::vec3 bgPos = bg->GetPositionGLM();

	const float fullW = timerBarFillSize_.x;
	const float fullH = timerBarFillSize_.y;

	float newW = fullW * ratio01;
	if (newW < 0.f) newW = 0.f;

	float leftX = bgPos.x - (fullW * 0.5f);
	float centerX = leftX + (newW * 0.5f);

	fill->SetScale({ newW, fullH, 1.0f });
	fill->SetPosition(Math::Vector3D(centerX, bgPos.y, bgPos.z));
}
