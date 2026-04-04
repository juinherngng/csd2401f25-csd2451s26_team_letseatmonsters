/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         IngredientBoxLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Defines IngredientBoxLogic, a table-like object that spawns
					ingredient items for the player. The box can be configured to
					spawn vegetables, meat, mushrooms, or plates depending on the
					setup. Provides logic for spawning, clearing, and controlling
					which ingredient type appears.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <random>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/EngineRng.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineCore/LogicManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/IngredientBoxLogic.hpp"

IngredientBoxLogic::IngredientBoxLogic(int ownerID) : TableLogic(ownerID) {}

/**
 * @brief Configures this box to spawn vegetable ingredients.
 */
void IngredientBoxLogic::ConfigureAsVegetableBox() {
	// Store the authored vegetable spawn profile used when the player interacts with this box.
	spawnMode_ = BoxSpawnMode::Ingredient;
	spawnType_ = IngredientType::Vegetable;

	// Point the runtime spawn settings at the vegetable art and layer configuration.
	ingredientTexture_ = "../assets/Food/Cabbage_Ingredient.png";
	ingredientWidth_ = 64.0f;
	ingredientHeight_ = 64.0f;
	ingredientLayer_ = "3";
}

/**
 * @brief Configures this box to spawn meat ingredients.
 */
void IngredientBoxLogic::ConfigureAsMeatBox() {
	// Store the authored meat spawn profile used when this box is activated.
	spawnMode_ = BoxSpawnMode::Ingredient;
	spawnType_ = IngredientType::Meat;

	// Point the runtime spawn settings at the meat art and layer configuration.
	ingredientTexture_ = "../assets/Food/Meat_Ingredient.png";
	ingredientWidth_ = 64.0f;
	ingredientHeight_ = 64.0f;
	ingredientLayer_ = "3";
}

/**
 * @brief Configures this box to spawn mushroom ingredients.
 */
void IngredientBoxLogic::ConfigureAsShroomBox() {
	// Store the authored mushroom spawn profile used when this box is activated.
	spawnMode_ = BoxSpawnMode::Ingredient;
	spawnType_ = IngredientType::Shroom;

	// Point the runtime spawn settings at the mushroom art and layer configuration.
	ingredientTexture_ = "../assets/Food/Mushroom_Ingredient.png";
	ingredientWidth_ = 64.0f;
	ingredientHeight_ = 64.0f;
	ingredientLayer_ = "3";
}

/**
 * @brief Configures this box to spawn carrot ingredients.
 */
void IngredientBoxLogic::ConfigureAsCarrotBox() {
	// Store the authored carrot spawn profile used when this box is activated.
	spawnMode_ = BoxSpawnMode::Ingredient;
	spawnType_ = IngredientType::Carrot;

	// Point the runtime spawn settings at the carrot art and layer configuration.
	ingredientTexture_ = "../assets/Food/Ingredient_Carrot.png";
	// The raw carrot sprite sheet is authored at a 4:3 aspect instead of square.
	ingredientWidth_ = 64.0f;
	ingredientHeight_ = 48.0f;
	ingredientLayer_ = "3";
}

/**
 * @brief Configures this box to spawn empty plates instead of ingredients.
 */
void IngredientBoxLogic::ConfigureAsPlateBox() {
	// Switch the box into plate-spawn mode for plate source stations.
	spawnMode_ = BoxSpawnMode::Plate;

	// Store the authored plate spawn profile used when this station is activated.
	plateTexture_ = "../assets/Food/Plate.png";
	plateWidth_ = 64.0f;
	plateHeight_ = 64.0f;
	plateLayer_ = "2";
}

/**
 * @brief Initializes the box and auto-configures its spawn mode from scene metadata.
 * @param scene Active scene containing the ingredient box object.
 */
void IngredientBoxLogic::Start(Scene& scene) {
	// Let the base table logic initialize shared table state first.
	TableLogic::Start(scene);

	// Log the resolved approach points so authored interaction offsets can be verified easily.
	auto worldPoints = GetApproachPointsWorld(scene);
	for (std::size_t i = 0; i < worldPoints.size(); ++i) {
		TS_LOG_DEBUG("[IngredientBoxLogic] approach[" << i << "] world=("
			<< worldPoints[i].x << ", " << worldPoints[i].y << ")");
	}

	// Auto-configure the station based on the authored object tag and texture.
	Scene::Defaults def = scene.GetDefaults(GetOwnerID());
	const std::string& tag = def.tag;
	const std::string& tex = def.texture;

	if (tag == "plate_box") {
		// Plate boxes bypass ingredient detection and always become plate sources.
		ConfigureAsPlateBox();
		return;
	}

	if (tag == "ingredient_box") {
		// Infer the ingredient family from the box art when the station is tagged as an ingredient box.
		if (tex.find("VegIngredientBox") != std::string::npos) {
			ConfigureAsVegetableBox();
		}
		else if (tex.find("MeatIngredientBox") != std::string::npos) {
			ConfigureAsMeatBox();
		}
		else if (tex.find("ShroomIngredientBox") != std::string::npos) {
			ConfigureAsShroomBox();
		}
		else if (tex.find("CarrotIngredientBox") != std::string::npos) {
			ConfigureAsCarrotBox();
		}
		else {
			// Fall back to vegetables when the texture does not match a known ingredient-box variant.
			ConfigureAsVegetableBox();
			TS_LOG_WARN("[IngredientBoxLogic] owner "
				<< GetOwnerID()
				<< " has unknown tag '" << tag
				<< "', defaulting to vegetable box");
		}
	}
}

/**
 * @brief Updates the ingredient box for one frame.
 * @param dt Delta time for the frame.
 * @param scene Active scene containing the ingredient box object.
 * @param input Input manager forwarded by the logic system.
 */
void IngredientBoxLogic::Update(float dt, Scene& scene, InputManager& input) {
	// Preserve the base table update flow so shared interaction state stays current.
	TableLogic::Update(dt, scene, input);
}

/**
 * @brief Returns whether this box can accept a dropped item.
 * @param scene Unused active scene reference.
 * @param itemID Unused candidate item ID.
 * @return Always false because ingredient boxes are sources, not containers.
 */
bool IngredientBoxLogic::CanAcceptItem(Scene& scene, int itemID) const {
	// Explicitly mark both parameters unused because source boxes never accept items.
	(void)scene;
	(void)itemID;
	return false;
}

/**
 * @brief Spawns the box's configured output item at the box position.
 * @param scene Active scene containing the ingredient box object.
 * @return Runtime ID of the spawned item, or `-1` on failure.
 */
int IngredientBoxLogic::SpawnIngredient(Scene& scene) {
	// Cache common scene helpers used by both ingredient and plate spawn flows.
	const int ownerID_ = GetOwnerID();
	LogicManager& logicMgr = scene.GetLogicManager();

	GameObject* spawnedObj = nullptr;
	int itemID = -1;

	if (spawnMode_ == BoxSpawnMode::Ingredient) {
		// Spawn a raw ingredient sprite directly on top of the source box.
		spawnedObj = scene.SpawnStaticSpriteAtSamePos(
			ownerID_,
			ingredientTexture_,
			ingredientWidth_,
			ingredientHeight_,
			ingredientLayer_
		);

		if (!spawnedObj) {
			TS_LOG_ERROR("[IngredientBoxLogic] Failed to spawn INGREDIENT from box "
				<< ownerID_);
			return -1;
		}

		itemID = spawnedObj->GetID();

		{
			// Tag the spawned object so other gameplay systems recognize it as an ingredient pickup.
			Scene::Defaults def = scene.GetDefaults(itemID);
			def.tag = "ingredient";
			scene.SetDefaults(itemID, def);
		}

		// Attach ingredient logic initialized with the configured raw ingredient type.
		if (auto* ingLogic = logicMgr.AddLogic<IngredientLogic>(itemID, spawnType_)) {
			ingLogic->Start(scene);
		}

		TS_LOG_DEBUG("[IngredientBoxLogic] Spawned INGREDIENT " << itemID
			<< " of type=" << static_cast<int>(spawnType_)
			<< " from box " << ownerID_);

		// Play a family-specific pickup sound so ingredients feel distinct when spawned.
		if (spawnType_ == IngredientType::Vegetable) {
			if (AudioManager* audioMgr = scene.GetAudioManager()) {
				std::uniform_int_distribution<int> dist(1, 4);
				int variant = dist(EngineRng::Get());
				std::string sfxName = "sfx_pickup_cabbage_" + std::to_string(variant);
				if (audioMgr->HasSound(sfxName)) {
					audioMgr->PlaySound(sfxName, audioMgr->GetVfxVolume() * 0.5f);
				}
			}
		}
		else {
			// Use the generic pickup variants for non-vegetable ingredients.
			if (AudioManager* audioMgr = scene.GetAudioManager()) {
				const char* variants[] = { "sfx_standard_pickup_01", "sfx_standard_pickup_02" };
				std::uniform_int_distribution<int> dist(0, 1);
				const char* sfxName = variants[dist(EngineRng::Get())];
				if (audioMgr->HasSound(sfxName)) {
					audioMgr->PlaySound(sfxName, audioMgr->GetVfxVolume() * 0.5f);
				}
			}
		}
	}
	else {
		// Spawn an empty plate sprite directly on top of the source box.
		spawnedObj = scene.SpawnStaticSpriteAtSamePos(
			ownerID_,
			plateTexture_,
			plateWidth_,
			plateHeight_,
			plateLayer_
		);

		if (!spawnedObj) {
			TS_LOG_ERROR("[IngredientBoxLogic] Failed to spawn PLATE from box "
				<< ownerID_);
			return -1;
		}

		itemID = spawnedObj->GetID();

		{
			// Tag the spawned object so gameplay systems recognize it as a plate.
			Scene::Defaults def = scene.GetDefaults(itemID);
			def.tag = "plate";
			scene.SetDefaults(itemID, def);
		}

		// Attach plate logic so the new plate can accept ingredients immediately.
		if (auto* plateLogic = logicMgr.AddLogic<PlateLogic>(itemID)) {
			plateLogic->Start(scene);
		}

		TS_LOG_DEBUG("[IngredientBoxLogic] Spawned PLATE " << itemID
			<< " from box " << ownerID_);
	}

	return itemID;
}
