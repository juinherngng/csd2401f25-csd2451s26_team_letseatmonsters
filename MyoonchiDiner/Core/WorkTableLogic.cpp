/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         WorkTableLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu   (90%)
 CO-AUTHOR:         Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:       Implements WorkTableLogic, the type of table that accepts raw
					ingredients, processes them into refined ingredients, and allows
					players to interact with workstations for cooking or preparation.
					This class overrides base TableLogic behavior to restrict what
					items can be placed, manage processing states, and output the
					refined ingredient when complete.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "Core/AudioManager.hpp"
#include "Graphics/GameObject.hpp"
#include "Graphics/SceneManager.hpp"

#include "WorkTableLogic.hpp"

static bool Contains(const std::string& s, const char* sub) {
	return s.find(sub) != std::string::npos;
}

static void DespawnIfAlive(Scene& scene, int& id) {
	if (id >= 0) {
		scene.DespawnByID(id);
		id = -1;
	}
}

static float Clamp01Value(float v) {
	if (v < 0.f) return 0.f;
	if (v > 1.f) return 1.f;
	return v;
}

WorkTableLogic::StationType WorkTableLogic::DetectStationTypeFromTexture(const std::string& texPath) const {
	// Detect by the workstation sprite (the table's texture)
	if (Contains(texPath, "Cutting_Board")) return StationType::CuttingBoard;
	if (Contains(texPath, "Grills"))        return StationType::Grill;
	if (Contains(texPath, "Stove"))         return StationType::Stove;
	return StationType::Generic;
}

const char* WorkTableLogic::GetProcessedTextureForRaw(IngredientType rawType) const {
	// IMPORTANT: Replace these 2 paths with your actual cooked meat/shroom assets.
	switch (rawType) {
	case IngredientType::Vegetable: return "../assets/Cabbage_CUT_Ingredient.png";
	case IngredientType::Meat:      return "../assets/Meat_CUT_Ingredient.png";
	case IngredientType::Shroom:    return "../assets/Mushroom_CUT_Ingredient.png";
	case IngredientType::Carrot:    return "../assets/CUT_Carrot_Ingredient.png";
	default:                        return "../assets/Cabbage_CUT_Ingredient.png";
	}
}

const char* WorkTableLogic::GetProcessingSoundName() const {
	switch (stationType_) {
	case StationType::CuttingBoard: return "sfx_chopping";
	case StationType::Grill:        return "sfx_grill";
	case StationType::Stove:        return "sfx_boiling_sound";
	default:                        return nullptr;
	}
}

// ------------------- Constructor -------------------

WorkTableLogic::WorkTableLogic(int ownerID) : TableLogic(ownerID) {

}

void WorkTableLogic::Start(Scene& scene) {
	TableLogic::Start(scene);

	//Figure out what kind of station THIS table is, from its texture
	Scene::Defaults def = scene.GetDefaults(GetOwnerID());
	stationType_ = DetectStationTypeFromTexture(def.texture);

	//different speeds per station
	switch (stationType_) {
	case StationType::CuttingBoard: processingTime_ = 3.0f; break;
	case StationType::Grill:        processingTime_ = 5.0f; break;
	case StationType::Stove:        processingTime_ = 7.0f; break;
	default:                        processingTime_ = 3.0f; break;
	}
}

void WorkTableLogic::OnDestroy(Scene& scene) {
	DestroyCookingTimerBar(scene);
	DespawnProcessingVfx(scene);
	TableLogic::OnDestroy(scene);
}

// ------------------- Update -------------------

void WorkTableLogic::Update(float dt, Scene& scene, InputManager&) {
	if (!scene.IsSimulationActive()) return;

	// Only do anything if we have an item and are currently processing
	if (!isProcessing_ || !HasItem()) {
		DestroyCookingTimerBar(scene);
		return;
	}

	timer_ += dt;

	if (isProcessing_) {
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
#ifndef _DEBUG
		if (AudioManager* audioMgr = scene.GetAudioManager()) {
			const char* soundName = GetProcessingSoundName();
			if (soundName && audioMgr->HasSound(soundName)) {
				audioMgr->StopSound(soundName);
			}
		}
#endif

		GameObject* item = scene.GetGameObjectByID(GetHeldItemID());
		if (item) {
			OnProcessingComplete(scene, *item);
		}
	}
}

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

float WorkTableLogic::GetProcessingProgress() const {
	if (!isProcessing_ || processingTime_ <= 0.0f)
		return 0.0f;

	float t = timer_ / processingTime_;
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	return t;
}

bool WorkTableLogic::CanAcceptItem(Scene& scene, int itemID) const {
	if (!TableLogic::CanAcceptItem(scene, itemID))
		return false;

	GameObject* item = scene.GetGameObjectByID(itemID);
	if (!item)
		return false;

	return IsItemProcessable(scene, *item);
}

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

// ------------------- Processing control -------------------

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

void WorkTableLogic::CancelProcessing(Scene& scene) {
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
	DespawnProcessingVfx(scene);
	DestroyCookingTimerBar(scene);
#ifdef _DEBUG
	(void)scene;
#endif
}

// ------------------- TableLogic hooks -------------------

void WorkTableLogic::OnItemPlaced(Scene& scene, GameObject& item) {
	CancelProcessing(scene); // always reset
	if (IsItemProcessable(scene, item)) {
		if (IngredientLogic* ing = scene.GetLogicManager().GetLogicForObject<IngredientLogic>(item.GetID())) {
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
#ifndef _DEBUG
		if (AudioManager* audioMgr = scene.GetAudioManager()) {
			const char* soundName = GetProcessingSoundName();
			if (soundName && audioMgr->HasSound(soundName)) {
				audioMgr->PlaySound(soundName, audioMgr->GetVfxVolume(), false);
				// Lower volume specifically for cutting board sound
				if (stationType_ == StationType::CuttingBoard) {
					audioMgr->SetVolume(soundName, audioMgr->GetVfxVolume() * 0.2f);
				}
			}
		}
#endif
	}
}

void WorkTableLogic::OnItemTaken(Scene& scene, GameObject& item) {
	(void)item;
	// If the player removes the item mid-process, cancel.
	if (isProcessing_) {
		CancelProcessing(scene);
	}
}

// ------------------- Processing complete hook -------------------

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

bool WorkTableLogic::ProcessIngredientInstant(IngredientLogic& ingredient) {
	if (!CanProcessIngredient(ingredient))
		return false;

	CompleteProcessingForIngredient(ingredient);
	return true;
}

void WorkTableLogic::CompleteProcessingForIngredient(IngredientLogic& ingredient) {
	// This is the actual "logic" of processing:
	// raw -> refined, via IngredientLogic.
	ingredient.MarkProcessed();
}

const char* WorkTableLogic::GetVfxTextureForStation() const {
	switch (stationType_) {
	case StationType::CuttingBoard: return "../assets/VFX SpriteSheet.png";
	case StationType::Grill:        return "../assets/VFX SpriteSheet.png";
	case StationType::Stove:        return "../assets/VFX SpriteSheet.png";
	default:                        return nullptr;
	}
}

const char* WorkTableLogic::GetVfxTagForStation() const {
	switch (stationType_) {
	case StationType::CuttingBoard: return "work_vfx_cut";
	case StationType::Grill:        return "work_vfx_grill";
	case StationType::Stove:        return "work_vfx_stove";
	default:                        return nullptr;
	}
}

void WorkTableLogic::SpawnProcessingVfx(Scene& scene) {
	if (vfxObjectID_ >= 0) return;

	const char* tex = GetVfxTextureForStation();
	const char* tag = GetVfxTagForStation();
	if (!tex || !tag) return;

	GameObject* table = scene.GetGameObjectByID(GetOwnerID());
	if (!table) return;

	glm::vec3 tp = table->GetPositionGLM();
	glm::vec3 vfxPos{ tp.x + vfxOffset_.x, tp.y + vfxOffset_.y, tp.z + 0.001f };

	// Create an animated sprite so AnimationManager can drive UVs
	std::vector<glm::vec4> dummyFrames = { glm::vec4(0.f, 0.f, 1.f, 1.f) };

	// Put it on a higher layer than the table (simple version: hardcode a top-ish layer)
	std::string vfxLayer = "50";

	GameObject* vfx = scene.SpawnAnimatedSprite(tex, vfxPos, glm::vec2(170, 230),
		dummyFrames, 0.1f, true, vfxLayer);

	if (!vfx) return;

	vfxObjectID_ = vfx->GetID();

	// no collisions / no physics / no shadow
	vfx->SetColliderSize(Math::Vector2D(0.f, 0.f));
	vfx->SetColliderOffset(Math::Vector2D(0.f, 0.f));
	vfx->SetMovableByPhysics(false);
	vfx->EnableShadow(false);

	// Tag + attach ONLY animations (via Scene::AttachLogicForTag)
	scene.SetObjectTag(vfxObjectID_, tag);
	scene.AttachLogicForTag(vfxObjectID_, tag);
}

void WorkTableLogic::DespawnProcessingVfx(Scene& scene) {
	if (vfxObjectID_ < 0) return;
	scene.RequestDespawn(vfxObjectID_);
	vfxObjectID_ = -1;
}

void WorkTableLogic::UpdateProcessingVfxTransform(Scene& scene) {
	if (vfxObjectID_ < 0) return;

	GameObject* table = scene.GetGameObjectByID(GetOwnerID());
	GameObject* vfx = scene.GetGameObjectByID(vfxObjectID_);
	if (!table || !vfx) return;

	glm::vec3 tp = table->GetPositionGLM();
	vfx->SetPosition(glm::vec3(tp.x + vfxOffset_.x, tp.y + vfxOffset_.y, tp.z + 0.001f));
}

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

void WorkTableLogic::DestroyCookingTimerBar(Scene& scene) {
	DespawnIfAlive(scene, timerBarFill_ID_);
	DespawnIfAlive(scene, timerBarBG_ID_);
}

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

