/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogicCarry.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements PlayerLogic carrying, dropping, and carried-item layering.
					- Attaches picked-up items to the player and restores them on drop
					- Updates carried item transforms and facing-dependent offsets
					- Applies temporary carry layers for plates and held ingredients
					- Restores original collider and render-layer state when releasing items

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <cctype>

#include "EngineCore/AudioManager.hpp"
#include "GameCore/PlateLogic.hpp"
#include "GameCore/PlayerLogicShared.hpp"

 /**
  * @brief Attaches a scene item to the player as the current carried object.
  * @param scene Active scene containing the item.
  * @param itemID Object ID of the item to pick up.
  */
void PlayerLogic::PickUp(Scene& scene, int itemID) {
	// Cache the item's original scene state so carrying can be reversed cleanly later.
	GameObject* item = scene.GetGameObjectByID(itemID);
	GameObject* player = GetOwner(scene);
	if (!item || !player) {
		return;
	}

	carriedItemID = itemID;
	carriedItemOriginalLayer_ = scene.GetObjectLayer(itemID);
	hasCarriedItemOriginalLayer_ = true;

	if (scene.ShouldUseRuntimeParityMode()) {
		if (AudioManager* audioMgr = scene.GetAudioManager()) {
			glm::vec3 playerPos = player->GetPositionGLM();
			audioMgr->PlaySound3D("ui_click", playerPos.x, playerPos.y, playerPos.z,
				audioMgr->GetVfxVolume() * 0.3f);
		}
	}

	carriedItemOriginalColliderSize = item->GetColliderSize();
	hasCarriedItemOriginalColliderSize = true;
	// Disable collisions while carrying so the held item does not interfere with navigation or tables.
	item->SetColliderSize(Math::Vector2D(0.f, 0.f));
	UpdateCarriedItemTransform(scene);
}

/**
 * @brief Drops the currently carried item back into the world.
 * @param scene Active scene receiving the dropped item.
 */
void PlayerLogic::Drop(Scene& scene) {
	// Drop the carried item beside the player and restore any suppressed carry-time state.
	if (carriedItemID < 0) {
		return;
	}

	GameObject* player = GetOwner(scene);
	GameObject* item = scene.GetGameObjectByID(carriedItemID);
	if (!player || !item) {
		carriedItemID = -1;
		return;
	}

	if (hasCarriedItemOriginalColliderSize) {
		item->SetColliderSize(carriedItemOriginalColliderSize);
		hasCarriedItemOriginalColliderSize = false;
	}

	RestoreCarriedItemLayer(scene, carriedItemID);

	glm::vec3 p = player->GetPositionGLM();
	item->SetPosition(glm::vec3(p.x + 16.f, p.y, p.z));
	carriedItemID = -1;
}

/**
 * @brief Keeps the carried item aligned with the player's current carry socket.
 * @param scene Active scene containing the carried item.
 */
void PlayerLogic::UpdateCarriedItemTransform(Scene& scene) {
	// Keep the held item attached to the player's facing-dependent carry socket.
	if (carriedItemID < 0) {
		return;
	}

	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	GameObject* item = scene.GetGameObjectByID(carriedItemID);
	if (!item) {
		return;
	}

	const glm::vec2 carry = GetCarryOffsetForFacing();
	glm::vec3 p = player->GetPositionGLM();
	item->SetPosition(glm::vec3(p.x + carry.x, p.y + carry.y, p.z));
	item->SetRenderSortOrder(0);

	ApplyCarryLayer(scene, carriedItemID);

	if (auto* plate = scene.GetLogicManager().GetLogicForObject<PlateLogic>(carriedItemID)) {
		// Carry any loose first ingredient with the plate so partial dishes stay visually assembled.
		const int child = plate->GetFirstIngredientObjectID();
		if (child >= 0 && !plate->HasPreparedDish()) {
			if (GameObject* ingObj = scene.GetGameObjectByID(child)) {
				glm::vec3 platePos = item->GetPositionGLM();
				ingObj->SetPosition(platePos);

				const std::string playerLayer = scene.GetObjectLayer(player->GetID());
				const std::string childLayer = GetCarryChildLayerForFacing(playerLayer);
				if (!childLayer.empty()) {
					scene.AssignObjectToLayer(child, childLayer);
				}

				ingObj->SetRenderSortOrder(1);
				ingObj->SetColliderSize(Math::Vector2D(0.f, 0.f));
				ingObj->SetMovableByPhysics(false);
			}
		}
	}
}

/**
 * @brief Returns the facing-dependent carry offset used for the held item.
 * @return Carry offset for the current facing direction.
 */
glm::vec2 PlayerLogic::GetCarryOffsetForFacing() const {
	// Each facing uses a slightly different anchor to keep the held item readable in 2D.
	switch (facingDir) {
	case FacingDir::Front: return carryOffsetFront_;
	case FacingDir::Back:  return carryOffsetBack_;
	case FacingDir::Left:  return carryOffsetLeft_;
	case FacingDir::Right: return carryOffsetRight_;
	default:               return carryOffset;
	}
}

/**
 * @brief Applies the correct presentation layer to a carried item.
 * @param scene Active scene containing the carried item.
 * @param itemID Object ID of the carried item.
 */
void PlayerLogic::ApplyCarryLayer(Scene& scene, int itemID) {
	// Move the held item onto the carry presentation layer for the current facing direction.
	if (!hasCarriedItemOriginalLayer_) {
		return;
	}

	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	const std::string playerLayer = scene.GetObjectLayer(player->GetID());
	const std::string desiredLayer = GetCarryLayerForFacing(playerLayer);
	if (desiredLayer.empty()) {
		return;
	}

	const std::string currentLayer = scene.GetObjectLayer(itemID);
	if (currentLayer != desiredLayer) {
		scene.AssignObjectToLayer(itemID, desiredLayer);
	}
}

/**
 * @brief Restores the original layer state for a previously carried item.
 * @param scene Active scene containing the released item.
 * @param itemID Object ID of the item being restored.
 */
void PlayerLogic::RestoreCarriedItemLayer(Scene& scene, int itemID) {
	// Restore the original authored layer once the item is no longer attached to the player.
	if (!hasCarriedItemOriginalLayer_) {
		return;
	}

	if (scene.GetGameObjectByID(itemID)) {
		scene.AssignObjectToLayer(itemID, carriedItemOriginalLayer_);
		if (GameObject* item = scene.GetGameObjectByID(itemID)) {
			item->SetRenderSortOrder(0);
		}
	}

	if (auto* plate = scene.GetLogicManager().GetLogicForObject<PlateLogic>(itemID)) {
		// Child ingredients sit one layer above their parent plate after the item is restored.
		const int child = plate->GetFirstIngredientObjectID();
		if (child >= 0) {
			const std::string childLayer = GetChildLayerAbove(carriedItemOriginalLayer_);
			if (!childLayer.empty()) {
				scene.AssignObjectToLayer(child, childLayer);
			}
			if (GameObject* ingObj = scene.GetGameObjectByID(child)) {
				ingObj->SetRenderSortOrder(1);
			}
		}
	}

	carriedItemOriginalLayer_.clear();
	hasCarriedItemOriginalLayer_ = false;
}

namespace {
	/**
	 * @brief Parses a numeric layer name into an integer value.
	 * @param layer Layer name to parse.
	 * @param outValue Receives the parsed integer value on success.
	 * @return True when the layer name is purely numeric.
	 */
	bool TryParseLayerNumber(const std::string& layer, int& outValue) {
		// Numeric layers can be offset safely; named layers should fall back unchanged.
		if (layer.empty()) {
			return false;
		}

		int value = 0;
		for (char c : layer) {
			if (!std::isdigit(static_cast<unsigned char>(c))) {
				return false;
			}
			value = value * 10 + (c - '0');
		}

		outValue = value;
		return true;
	}
}

/**
 * @brief Returns the carry layer used for the current facing direction.
 * @param baseLayer Original object layer before carrying.
 * @return Layer name used while the item is carried.
 */
std::string PlayerLogic::GetCarryLayerForFacing(const std::string& /*baseLayer*/) const {
	// Carry visuals currently use a fixed foreground layer for every facing so held items stay readable.
	switch (facingDir) {
	case FacingDir::Front: return "6";
	case FacingDir::Back:  return "6";
	case FacingDir::Left:  return "6";
	case FacingDir::Right: return "6";
	default:               return "6";
	}
}

/**
 * @brief Returns the child carry layer used for plate contents while carried.
 * @param baseLayer Original object layer before carrying.
 * @return Layer name used for carried child visuals.
 */
std::string PlayerLogic::GetCarryChildLayerForFacing(const std::string& /*baseLayer*/) const {
	// Child visuals follow the same carry-layer policy as the parent plate for now.
	switch (facingDir) {
	case FacingDir::Front: return "6";
	case FacingDir::Back:  return "6";
	case FacingDir::Left:  return "6";
	case FacingDir::Right: return "6";
	default:               return "6";
	}
}

/**
 * @brief Returns the layer directly above a numeric base layer.
 * @param baseLayer Original layer name.
 * @return Incremented numeric layer name, or the original string when parsing fails.
 */
std::string PlayerLogic::GetChildLayerAbove(const std::string& baseLayer) const {
	// Numeric layers are offset upward so attached child sprites render above their parent.
	int value = 0;
	if (!TryParseLayerNumber(baseLayer, value)) {
		return baseLayer;
	}

	return std::to_string(value + 1);
}
