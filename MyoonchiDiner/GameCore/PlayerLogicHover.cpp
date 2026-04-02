/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogicHover.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements PlayerLogic hover outlines and click indicators.
					- Tracks hovered interactables under the cursor
					- Spawns and updates outline overlays for table feedback
					- Displays animated click-to-move destination indicators
					- Clears transient hover and click visuals during state transitions

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineCore/InputManager.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "GameCore/PlayerLogicShared.hpp"

namespace {
	constexpr float kHoverOutlineOffset = 2.5f;
	constexpr int kHoverOutlineSortOrder = 200;
	constexpr int kHoverOutlineMaskSortOrder = 201;
	const glm::vec4 kHoverOutlineTint(0.0f, 1.0f, 1.0f, 0.92f);
}

/**
 * @brief Refreshes hover highlights and outline overlays for interactable objects.
 * @param scene Active scene being processed.
 * @param input Centralized input snapshot for the current frame.
 * @param dt Frame delta time in seconds.
 */
void PlayerLogic::UpdateInteractableVisualCues(Scene& scene, InputManager& input, float dt) {
	// Rebuild the current hover set each frame so outlines follow both mouse movement and object motion.
	(void)dt;

	glm::vec2 mouseWorld{};
	const bool hasMouseWorld = TryGetMouseWorld(scene, input, mouseWorld);

	std::unordered_set<int> nextHighlighted;
	nextHighlighted.reserve(16);

	LogicManager& logicMgr = scene.GetLogicManager();

	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		// Only table-like interactables currently receive hover outlines.
		if (!obj) {
			continue;
		}

		const int id = obj->GetID();
		TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(id);
		if (!tableLogic) {
			continue;
		}

		const bool hovered = hasMouseWorld && IsPointInsideObjectCollider(obj, mouseWorld);
		if (!hovered) {
			continue;
		}

		nextHighlighted.insert(id);
		EnsureHoverOutline(scene, obj, id);
	}

	for (int id : highlightedInteractableIDs_) {
		// Despawn outlines for objects that are no longer under the cursor.
		if (nextHighlighted.find(id) == nextHighlighted.end()) {
			RemoveHoverOutline(scene, id);
		}
	}

	highlightedInteractableIDs_ = std::move(nextHighlighted);
}

/**
 * @brief Returns whether a world-space point lies inside an object's collider.
 * @param obj Object being tested.
 * @param worldPoint World-space point to test.
 * @return True when the point lies inside the collider bounds.
 */
bool PlayerLogic::IsPointInsideObjectCollider(const GameObject* obj, const glm::vec2& worldPoint) const {
	// Hover checks use collider bounds directly so interaction feedback matches gameplay reach.
	if (!obj) {
		return false;
	}

	auto colSize = obj->GetColliderSize();
	if (colSize.x <= 0.0f || colSize.y <= 0.0f) {
		return false;
	}

	auto colOffset = obj->GetColliderOffset();
	glm::vec3 objPos = obj->GetPositionGLM();
	glm::vec2 center(objPos.x + colOffset.x, objPos.y + colOffset.y);

	float halfW = colSize.x * 0.5f;
	float halfH = colSize.y * 0.5f;

	return (worldPoint.x >= center.x - halfW && worldPoint.x <= center.x + halfW) &&
		(worldPoint.y >= center.y - halfH && worldPoint.y <= center.y + halfH);
}

/**
 * @brief Spawns or restarts the animated click destination indicator.
 * @param scene Active scene being processed.
 * @param worldPoint World-space destination being indicated.
 */
void PlayerLogic::ShowClickMoveIndicator(Scene& scene, const glm::vec2& worldPoint) {
	// Spawn a short-lived animated marker to confirm the accepted click destination.
	const glm::vec3 markerPos(worldPoint.x, worldPoint.y, 0.0f);

	// Despawn old indicator so each click restarts the animation cleanly
	if (clickIndicatorID_ >= 0) {
		scene.DespawnByID(clickIndicatorID_);
		clickIndicatorID_ = -1;
	}

	static const std::vector<glm::vec4> kArrowFrames = []() {
		// Build the sprite-sheet frame table once and reuse it for every click marker.
		std::vector<glm::vec4> frames;
		frames.reserve(8);

		constexpr int kFrameCount = 8;
		constexpr float kFrameWidth = 1.0f / static_cast<float>(kFrameCount);

		for (int i = 0; i < kFrameCount; ++i) {
			frames.emplace_back(i * kFrameWidth, 0.0f, kFrameWidth, 1.0f);
		}

		return frames;
		}();

	std::string markerLayer = "1";
	if (GameObject* player = GetOwner(scene)) {
		// Keep the marker in the player's active layer space so it remains visible in-scene.
		const std::string playerLayer = scene.GetObjectLayer(player->GetID());
		if (!playerLayer.empty()) {
			markerLayer = playerLayer;
		}
	}

	GameObject* marker = scene.SpawnAnimatedSprite(
		"../assets/VFX/arrow-Sheet.png",
		markerPos,
		glm::vec2(PlayerLogicDetail::kClickIndicatorBaseSize, PlayerLogicDetail::kClickIndicatorBaseSize),
		kArrowFrames,
		0.06f,
		true,
		markerLayer
	);

	if (!marker) {
		clickIndicatorID_ = -1;
		clickIndicatorTimeLeft_ = 0.0f;
		return;
	}

	clickIndicatorID_ = marker->GetID();
	clickIndicatorTimeLeft_ = PlayerLogicDetail::kClickIndicatorLifetime;

	// The indicator is purely visual and should never participate in interaction queries.
	marker->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
	marker->SetRenderSortOrder(1000);
	marker->SetScale(glm::vec3(PlayerLogicDetail::kClickIndicatorBaseSize, PlayerLogicDetail::kClickIndicatorBaseSize, 1.0f));
	marker->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

/**
 * @brief Updates the lifetime and appearance of the active click indicator.
 * @param scene Active scene being processed.
 * @param dt Frame delta time in seconds.
 */
void PlayerLogic::UpdateClickMoveIndicator(Scene& scene, float dt) {
	// Count down and despawn the click marker once its short feedback window expires.
	if (clickIndicatorID_ < 0) {
		return;
	}

	GameObject* marker = scene.GetGameObjectByID(clickIndicatorID_);
	if (!marker) {
		clickIndicatorID_ = -1;
		clickIndicatorTimeLeft_ = 0.0f;
		return;
	}

	clickIndicatorTimeLeft_ -= dt;
	if (clickIndicatorTimeLeft_ <= 0.0f) {
		scene.DespawnByID(clickIndicatorID_);
		clickIndicatorID_ = -1;
		clickIndicatorTimeLeft_ = 0.0f;
		return;
	}

	// Temporarily disabled while testing whether the arrow animation is playing correctly.
	// Keep the indicator at a fixed size and fixed color.
	/*
	const float normalizedTimeLeft =
		std::clamp(clickIndicatorTimeLeft_ / kClickIndicatorLifetime, 0.0f, 1.0f);
	const float progress = 1.0f - normalizedTimeLeft;

	const float pulse = std::sin(progress * 3.14159265f);
	const float size =
		kClickIndicatorBaseSize + pulse * (kClickIndicatorPopSize - kClickIndicatorBaseSize);

	marker->SetScale(glm::vec3(size, size, 1.0f));
	marker->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, normalizedTimeLeft));
	*/

	marker->SetScale(glm::vec3(PlayerLogicDetail::kClickIndicatorBaseSize, PlayerLogicDetail::kClickIndicatorBaseSize, 1.0f));
	marker->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

/**
 * @brief Removes the active click move indicator immediately.
 * @param scene Active scene being processed.
 */
void PlayerLogic::ClearClickMoveIndicator(Scene& scene) {
	// Remove any active click marker immediately, typically when entering pause or clearing state.
	if (clickIndicatorID_ >= 0) {
		scene.DespawnByID(clickIndicatorID_);
	}

	clickIndicatorID_ = -1;
	clickIndicatorTimeLeft_ = 0.0f;
}

/**
 * @brief Clears all transient hover visuals currently owned by PlayerLogic.
 * @param scene Active scene being processed.
 */
void PlayerLogic::ClearInteractableVisualCues(Scene& scene) {
	// Reset hover state in one place so pause/scene transitions can clean up transient visuals safely.
	for (int id : highlightedInteractableIDs_) {
		if (GameObject* obj = scene.GetGameObjectByID(id)) {
			obj->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		}
	}

	highlightedInteractableIDs_.clear();
	ClearHoverOutlines(scene);
}

/**
 * @brief Ensures a hover outline exists and matches the current source object transform.
 * @param scene Active scene containing the outlined object.
 * @param sourceObj Source object being outlined.
 * @param sourceID Object ID of the source object.
 */
void PlayerLogic::EnsureHoverOutline(Scene& scene, GameObject* sourceObj, int sourceID) {
	// Lazily spawn outline sprites, then keep them glued to the source object every frame.
	if (!sourceObj || sourceID < 0) {
		return;
	}

	const std::string texturePath = scene.GetObjectTexturePath(sourceID);
	if (texturePath.empty()) {
		return;
	}

	const std::string outlineLayer = scene.GetObjectLayer(sourceID);

	auto it = hoverOutlineIDs_.find(sourceID);

	if (it == hoverOutlineIDs_.end()) {
		// First-time hover on this object: create the four offset outlines plus the center mask.
		std::array<int, 5> outlineIDs{ -1, -1, -1, -1, -1 };
		const glm::vec3 srcPos = sourceObj->GetPositionGLM();
		const glm::vec3 srcScale = sourceObj->GetScaleGLM();

		const std::array<glm::vec2, 4> offsets{
			glm::vec2(-kHoverOutlineOffset, 0.0f),
			glm::vec2(kHoverOutlineOffset, 0.0f),
			glm::vec2(0.0f, -kHoverOutlineOffset),
			glm::vec2(0.0f,  kHoverOutlineOffset)
		};

		for (std::size_t i = 0; i < offsets.size(); ++i) {
			// Each offset sprite builds one edge of the cyan silhouette.
			const glm::vec2& offset = offsets[i];
			const glm::vec3 outlinePos(srcPos.x + offset.x, srcPos.y + offset.y, srcPos.z);

			GameObject* outline = scene.SpawnStaticSprite(
				texturePath,
				outlinePos,
				glm::vec2(std::abs(srcScale.x), std::abs(srcScale.y)),
				outlineLayer
			);

			if (!outline) {
				continue;
			}

			outline->SetColorTint(kHoverOutlineTint);
			outline->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
			outline->SetMovableByPhysics(false);
			outline->EnableShadow(false);
			outline->SetRenderSortOrder(kHoverOutlineSortOrder);
			outline->SetRotation(sourceObj->GetRotation(), glm::vec3(0.0f, 0.0f, 1.0f));

			if (Shader* outlineShader = ResourceManager::Instance().GetShader("hover_outline")) {
				outline->SetShader(outlineShader);
			}

			outlineIDs[i] = outline->GetID();
		}

		GameObject* mask = scene.SpawnStaticSprite(
			texturePath,
			srcPos,
			glm::vec2(std::abs(srcScale.x), std::abs(srcScale.y)),
			outlineLayer
		);

		if (mask) {
			// The mask hides the middle of the outline stack so only the edge glow remains visible.
			mask->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			mask->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
			mask->SetMovableByPhysics(false);
			mask->EnableShadow(false);
			mask->SetRenderSortOrder(kHoverOutlineMaskSortOrder);
			mask->SetRotation(sourceObj->GetRotation(), glm::vec3(0.0f, 0.0f, 1.0f));
			outlineIDs[4] = mask->GetID();
		}

		hoverOutlineIDs_[sourceID] = outlineIDs;
		it = hoverOutlineIDs_.find(sourceID);
	}

	const glm::vec3 srcPos = sourceObj->GetPositionGLM();
	const glm::vec3 srcScale = sourceObj->GetScaleGLM();
	const float srcRot = sourceObj->GetRotation();

	const std::array<glm::vec2, 4> offsets{
		glm::vec2(-kHoverOutlineOffset, 0.0f),
		glm::vec2(kHoverOutlineOffset, 0.0f),
		glm::vec2(0.0f, -kHoverOutlineOffset),
		glm::vec2(0.0f,  kHoverOutlineOffset)
	};

	for (std::size_t i = 0; i < 4; ++i) {
		// Keep every outline segment in lockstep with the source object's transform.
		const int id = it->second[i];
		if (id < 0) {
			continue;
		}

		GameObject* outline = scene.GetGameObjectByID(id);
		if (!outline) {
			continue;
		}

		const glm::vec2& offset = offsets[i];
		outline->SetPosition(glm::vec3(srcPos.x + offset.x, srcPos.y + offset.y, srcPos.z));
		outline->SetScale(glm::vec3(std::abs(srcScale.x), std::abs(srcScale.y), 1.0f));
		outline->SetRotation(srcRot, glm::vec3(0.0f, 0.0f, 1.0f));
		outline->SetColorTint(kHoverOutlineTint);
		outline->SetRenderSortOrder(kHoverOutlineSortOrder);
		scene.AssignObjectToLayer(id, outlineLayer);
	}

	const int maskID = it->second[4];
	if (maskID >= 0) {
		// Update the center mask separately so it stays perfectly aligned with the source sprite.
		if (GameObject* mask = scene.GetGameObjectByID(maskID)) {
			mask->SetPosition(srcPos);
			mask->SetScale(glm::vec3(std::abs(srcScale.x), std::abs(srcScale.y), 1.0f));
			mask->SetRotation(srcRot, glm::vec3(0.0f, 0.0f, 1.0f));
			mask->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			mask->SetRenderSortOrder(kHoverOutlineMaskSortOrder);
			scene.AssignObjectToLayer(maskID, outlineLayer);
		}
	}
}

/**
 * @brief Removes the hover outline associated with a single source object.
 * @param scene Active scene containing the outline objects.
 * @param sourceID Object ID whose outline should be removed.
 */
void PlayerLogic::RemoveHoverOutline(Scene& scene, int sourceID) {
	// Tear down every overlay sprite associated with the requested source object.
	auto it = hoverOutlineIDs_.find(sourceID);
	if (it == hoverOutlineIDs_.end()) {
		return;
	}

	for (int id : it->second) {
		if (id >= 0 && scene.GetGameObjectByID(id)) {
			scene.DespawnByID(id);
		}
	}

	hoverOutlineIDs_.erase(it);
}

/**
 * @brief Removes every hover outline currently tracked by PlayerLogic.
 * @param scene Active scene containing the outline objects.
 */
void PlayerLogic::ClearHoverOutlines(Scene& scene) {
	// Bulk cleanup used when the player leaves gameplay or hover tracking is reset wholesale.
	for (const auto& [sourceID, ids] : hoverOutlineIDs_) {
		(void)sourceID;
		for (int id : ids) {
			if (id >= 0 && scene.GetGameObjectByID(id)) {
				scene.DespawnByID(id);
			}
		}
	}

	hoverOutlineIDs_.clear();
}
