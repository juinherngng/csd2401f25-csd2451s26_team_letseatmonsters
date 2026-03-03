/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (80%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu	  (20%)

 DESCRIPTION:		Implements player control logic, including movement, sprite updates,
					mouse click handling, item pickup/drop, and scene clamping behavior.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#include "../Core/AudioManager.hpp"
#include "../Core/CustomerTableLogic.hpp"
#include "../Core/DebugUI.hpp"
#include "../Core/InputControls.hpp"
#include "../Core/InputManager.hpp"
#include "../Core/TableLogic.hpp"
#include "../Core/TrashCanLogic.hpp"
#include "../Core/WorkTableLogic.hpp"
#include "../Graphics/SceneManager.hpp"

#include "PlayerLogic.hpp"

#include <algorithm>
#include <iostream>
#include <limits>

// Constants and helper functions for PlayerLogic, in an anonymous namespace to limit scope to this file.
namespace {
	// Interaction and movement parameters
	constexpr float kPlayerInteractRadius = 67.0f;
	constexpr float kMoveRetargetDeadzone = 6.0f;
	constexpr float kDragRetargetDistance = 20.0f;
	constexpr float kDragRetargetInterval = 0.06f;
	constexpr float kArriveRadius = 4.0f;
	constexpr int kBlockedFramesBeforeCancel = 6;

	constexpr float kKeyboardMoveSpeed = 200.0f;
	constexpr float kTrailJitterEpsilon = 0.25f;
	constexpr float kFootstepInterval = 0.3f;
	constexpr float kClickIndicatorLifetime = 0.35f;

	// Squared distance between two points (avoids sqrt for efficiency when comparing distances)
	float DistanceSquared(const glm::vec2& a, const glm::vec2& b) {
		const glm::vec2 delta = a - b;
		return delta.x * delta.x + delta.y * delta.y;
	}

	// Convert a glm::vec3 to glm::vec2 by dropping the z component
	glm::vec2 ToVec2(const glm::vec3& value) {
		return { value.x, value.y };
	}

	// Normalize a vector, but return zero if the length is very small to avoid instability
	glm::vec2 NormalizeOrZero(const glm::vec2& v) {
		const float len = std::sqrt(v.x * v.x + v.y * v.y);
		if (len <= 0.0001f) {
			return glm::vec2(0.0f, 0.0f);
		}

		return glm::vec2(v.x / len, v.y / len);
	}

	// Result struct for FindClickedTable, containing the ID and logic pointer of the clicked table (or defaults if none)
	struct ClickedTableResult {
		int tableID = -1;
		TableLogic* tableLogic = nullptr;
	};

	// Find the closest table under the mouse cursor, if any. Returns a struct with the table ID and logic pointer, or defaults if no table was clicked.
	ClickedTableResult FindClickedTable(Scene& scene, const glm::vec2& mouseWorld) {
		ClickedTableResult result{};
		float bestDistSq = std::numeric_limits<float>::max();

		// Iterate through all game objects and check for tables under the mouse cursor
		LogicManager& logicMgr = scene.GetLogicManager();
		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) {
				continue;
			}

			const int id = obj->GetID();
			TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(id);
			if (!tableLogic) {
				continue;
			}

			auto colSize = obj->GetColliderSize();
			auto colOffset = obj->GetColliderOffset();
			glm::vec3 objPos = obj->GetPositionGLM();
			glm::vec2 center(objPos.x + colOffset.x, objPos.y + colOffset.y);

			const float halfW = colSize.x * 0.5f;
			const float halfH = colSize.y * 0.5f;

			const bool inside =
				(mouseWorld.x >= center.x - halfW && mouseWorld.x <= center.x + halfW) &&
				(mouseWorld.y >= center.y - halfH && mouseWorld.y <= center.y + halfH);

			if (!inside) {
				continue;
			}

			const float distSq = DistanceSquared(mouseWorld, center);
			if (distSq < bestDistSq) {
				bestDistSq = distSq;
				result.tableID = id;
				result.tableLogic = tableLogic;
			}
		}

		return result;
	}
}

// Initialize player state
void PlayerLogic::Start(Scene& scene) {
	(void)scene;
	hasMoveTarget = false;
	carriedItemID = -1;
	pendingTableID = -1;
	facingDir = FacingDir::Front;
	mouseDragActive_ = false;
	hasLastDragWorld_ = false;
	dragRetargetTimer_ = 0.0f;
	highlightedInteractableIDs_.clear();
	clickIndicatorID_ = -1;
	clickIndicatorTimeLeft_ = 0.0f;

	//GameObject* owner = GetOwner(scene);
	//std::cout << "[PlayerLogic] Start on object ID "
	//	<< (owner ? owner->GetID() : -1) << "\n";
}

// Handle input and movement each frame
void PlayerLogic::ResetMouseDragState() {
	mouseDragActive_ = false;
	hasLastDragWorld_ = false;
	dragRetargetTimer_ = 0.0f;
}

//Get the mouse world position if the mouse is currently over the scene viewport
bool PlayerLogic::TryGetMouseWorld(Scene& scene, glm::vec2& mouseWorld) const {
	return scene.GetGraphicsEngine().GetMouseWorldInScene(mouseWorld);
}

// Clear the current movement target and reset related state
void PlayerLogic::ClearMovementTarget(Scene& scene) {
	hasMoveTarget = false;
	blockedMoveFrames_ = 0;

	if (GameObject* player = GetOwner(scene)) {
		scene.GetMovementManager().ClearMoveTarget(player->GetID());
	}
}

// Decide and apply sprite based on movement direction
void PlayerLogic::UpdateSprite(Scene& scene, GameObject* player, const glm::vec2& moveDirRaw) {
	(void)scene;
	if (!player) {
		return;
	}

	const float moveThreshold = 0.01f;
	std::string desiredAnimation;

	float absX = std::abs(moveDirRaw.x);
	float absY = std::abs(moveDirRaw.y);

	// Detect idle/no movement
	if (glm::length(moveDirRaw) < moveThreshold) {
		switch (facingDir) {
		case FacingDir::Right: desiredAnimation = "IDLE_RIGHT"; break;
		case FacingDir::Left:  desiredAnimation = "IDLE_LEFT";  break;
		case FacingDir::Front: desiredAnimation = "IDLE_FRONT"; break;
		case FacingDir::Back:  desiredAnimation = "IDLE_BACK";  break;
		}
	}
	else {
		if (absX > absY) {
			if (moveDirRaw.x > 0.0f) {
				desiredAnimation = "WALK_RIGHT";
				facingDir = FacingDir::Right;
			}
			else {
				desiredAnimation = "WALK_LEFT";
				facingDir = FacingDir::Left;
			}
		}
		else {
			if (moveDirRaw.y > 0.0f) {
				desiredAnimation = "WALK_FRONT";
				facingDir = FacingDir::Front;
			}
			else {
				desiredAnimation = "WALK_BACK";
				facingDir = FacingDir::Back;
			}
		}
	}

	// Query the currently playing animation (to prevent animation resets due to the same input pressed)
	std::string currentAnimation = scene.GetCurrentAnimationName(player->GetID());
	if (desiredAnimation != currentAnimation) {
		scene.SetAnimation(player->GetID(), desiredAnimation);
	}
}

// Set the player's movement target to the specified world position, and update sprite direction immediately based on the click direction.
// This does NOT do any pathfinding or collision checks; it just sets the target and lets the movement system handle it. If the new target is very close to the current target, it will be ignored to prevent jitter.
void PlayerLogic::MoveTo(Scene& scene, const glm::vec2& dest) {
	if (hasMoveTarget) {
		const glm::vec2 delta = dest - moveTarget;
		if ((delta.x * delta.x + delta.y * delta.y) <=
			(kMoveRetargetDeadzone * kMoveRetargetDeadzone)) {
			return;
		}
	}

	GameObject* player = GetOwner(scene);

	//std::cout << "[PlayerLogic] MoveTo(" << dest.x << ", " << dest.y << ")\n";

	if (player) {
		glm::vec3 pos3 = player->GetPositionGLM();
		glm::vec2 pos(pos3.x, pos3.y);
		glm::vec2 delta = dest - pos;

		// Update sprite immediately based on click direction
		UpdateSprite(scene, player, delta);
	}

	moveTarget = dest;
	hasMoveTarget = true;
	blockedMoveFrames_ = 0;
}

// Handle mouse click and hold input for movement and interaction.
// On click, raycast to check if a table was clicked and move to its approach point if so.
// If holding and dragging, continuously retarget movement to the mouse position with a small deadzone and retarget interval.
void PlayerLogic::HandleClickInput(Scene& scene, InputManager& input, float dt) {
	const bool lmbJustPressed = input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);
	const bool lmbHeld = input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);

	if (!lmbHeld) {
		ResetMouseDragState();
	}

	if (!lmbJustPressed && !lmbHeld) {
		return;
	}

	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	glm::vec2 mouseWorld{};

	if (!TryGetMouseWorld(scene, mouseWorld)) {
		return;
	}

	if (lmbHeld && !lmbJustPressed) {
		if (!mouseDragActive_) {
			mouseDragActive_ = true;
			hasLastDragWorld_ = false;
			dragRetargetTimer_ = 0.0f;
		}

		dragRetargetTimer_ -= dt;

		const float minDragRetargetDistSq = kDragRetargetDistance * kDragRetargetDistance;
		const glm::vec2 delta = mouseWorld - lastDragWorld_;
		const bool movedEnough = !hasLastDragWorld_ || DistanceSquared(delta, glm::vec2(0.0f, 0.0f)) >= minDragRetargetDistSq;

		if (movedEnough && dragRetargetTimer_ <= 0.0f) {
			pendingTableID = -1;
			MoveTo(scene, mouseWorld);
			ShowClickMoveIndicator(scene, mouseWorld);
			lastDragWorld_ = mouseWorld;
			hasLastDragWorld_ = true;
			dragRetargetTimer_ = kDragRetargetInterval;
		}

		return;
	}

	mouseDragActive_ = true;
	lastDragWorld_ = mouseWorld;
	hasLastDragWorld_ = true;
	dragRetargetTimer_ = 0.0f;

	const ClickedTableResult clickedTable = FindClickedTable(scene, mouseWorld);
	if (clickedTable.tableID >= 0 && clickedTable.tableLogic) {
		pendingTableID = clickedTable.tableID;
		const glm::vec2 playerPos = ToVec2(player->GetPositionGLM());

		Math::Vector2D from(playerPos.x, playerPos.y);
		Math::Vector2D approach = clickedTable.tableLogic->GetClosestApproachPoint(scene, from);
		const glm::vec2 target(approach.x, approach.y);

		// If we're already within interaction range of the approach point, just interact immediately without moving.
		if (DistanceSquared(target, playerPos) <= (kPlayerInteractRadius * kPlayerInteractRadius)) {
			ClearMovementTarget(scene);
			pendingTableID = -1;
			InteractWithTable(scene, clickedTable.tableID);
		}
		else {
			MoveTo(scene, target);
			ShowClickMoveIndicator(scene, target);
		}

		return;
	}

	pendingTableID = -1;
	MoveTo(scene, mouseWorld);
	ShowClickMoveIndicator(scene, mouseWorld);
}

// Move owner GameObject towards moveTarget at moveSpeed
void PlayerLogic::UpdateMovement(float dt, Scene& scene) {
	if (!hasMoveTarget) {
		return;
	}

	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	glm::vec3 pos3 = player->GetPositionGLM();
	glm::vec2 pos(pos3.x, pos3.y);

	glm::vec2 dir = moveTarget - pos;
	const float distSq = dir.x * dir.x + dir.y * dir.y;

	const float arriveRadiusSq = kArriveRadius * kArriveRadius;

	// ReachedDestination()
	if (distSq <= arriveRadiusSq) {
		ClearMovementTarget(scene);
		OnArrived(scene);
		return;
	}

	const float dist = std::sqrt(distSq);
	if (dist > 0.0001f) {
		dir.x /= dist;
		dir.y /= dist;
	}

	float step = moveSpeed * dt;
	if (step > dist) {
		step = dist;
	}

	// Desired movement for this frame
	glm::vec2 desiredDelta(dir.x * step, dir.y * step);

	// Trim against static world (outer frame + wood + gate)
	glm::vec2 allowedDelta = scene.ResolveWorldStep(player, desiredDelta);

	// If we can't move at all (hit a wall and are stuck), treat it as "try to arrive"
	// and let OnArrived decide if we are close enough to interact.
	const float allowedLenSq = allowedDelta.x * allowedDelta.x +
		allowedDelta.y * allowedDelta.y;
	if (allowedLenSq < 0.0001f) {
		++blockedMoveFrames_;

		// Give collision resolution a few frames to recover from corner/edge jitter before cancelling.
		if (blockedMoveFrames_ >= kBlockedFramesBeforeCancel) {
			std::cout << "[PlayerLogic] MoveTo blocked by collision for several frames, invoking OnArrived\n";

			ClearMovementTarget(scene);

			// This will check distance to the table’s approach point using kInteractRadius
			OnArrived(scene);
		}

		return;
	}

	blockedMoveFrames_ = 0;

	pos.x += allowedDelta.x;
	pos.y += allowedDelta.y;

	player->SetPosition(glm::vec3(pos.x, pos.y, pos3.z));
	scene.ClampToWalkArea(player); // still keep outer-frame clamp

	// Use allowedDelta as movement direction for the sprite
	glm::vec2 moveDir(allowedDelta.x, allowedDelta.y);
	UpdateSprite(scene, player, moveDir);

	// Debug path line from player to target
	if (DebugRenderer::IsEnabled() && hasMoveTarget) {
		glm::vec3 from = player->GetPositionGLM();
		glm::vec3 to(moveTarget.x, moveTarget.y, from.z);
		DebugRenderer::DrawLine(from, to, glm::vec3(0.0f, 1.0f, 0.0f));
	}
}

// Unity: OnArrived()
// For now it's a stub; later you can branch by what we clicked (tables, spawners, etc.)
void PlayerLogic::OnArrived(Scene& scene) {
	//std::cout << "[PlayerLogic] Arrived at destination\n";

	if (pendingTableID < 0) {
		return;
	}

	GameObject* player = GetOwner(scene);
	GameObject* tableObj = scene.GetGameObjectByID(pendingTableID);
	if (!player || !tableObj) {
		pendingTableID = -1;
		return;
	}

	glm::vec3 pPos3 = player->GetPositionGLM();
	const glm::vec2 pPos = ToVec2(pPos3);

	// Get the table logic so we can ask for its approach point
	LogicManager& logicMgr = scene.GetLogicManager();
	TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(pendingTableID);

	float distSq = 0.0f;

	if (tableLogic) {
		// Use the SAME approach-point logic that we used when clicking.
		Math::Vector2D from(pPos.x, pPos.y);
		Math::Vector2D approach = tableLogic->GetClosestApproachPoint(scene, from);

		distSq = DistanceSquared(pPos, glm::vec2(approach.x, approach.y));

		//std::cout << "[PlayerLogic] Dist to table APPROACH point: "
		//	<< std::sqrt(distSq)
		//	<< " (approach=(" << approach.x << ", " << approach.y << "))\n";
	}
	else {
		// Fallback: no TableLogic (shouldn’t really happen for tables)
		glm::vec3 tPos = tableObj->GetPositionGLM();
		distSq = DistanceSquared(pPos, ToVec2(tPos));

		//std::cout << "[PlayerLogic] Dist to table ORIGIN (fallback): "
		//	<< std::sqrt(distSq) << "\n";
	}

	// Interaction radius around the approach point
	if (distSq <= kPlayerInteractRadius * kPlayerInteractRadius) {
		//std::cout << "[PlayerLogic] Close enough to table " << pendingTableID
		//	<< " (approach), performing interaction\n";
		InteractWithTable(scene, pendingTableID);
	}
	else {
		//std::cout << "[PlayerLogic] Arrived near click, but too far from table approach (dist="
		//	<< std::sqrt(distSq) << ")\n";
	}

	pendingTableID = -1;
}

// Unity: PickUp(GameObject item)
void PlayerLogic::PickUp(Scene& scene, int itemID) {
	GameObject* item = scene.GetGameObjectByID(itemID);
	GameObject* player = GetOwner(scene);
	if (!item || !player) {
		return;
	}

	carriedItemID = itemID;

	// Play UI click sound for pickup feedback (release mode only)
#ifndef _DEBUG
	if (AudioManager* audioMgr = scene.GetAudioManager()) {
		audioMgr->PlaySound("ui_click", audioMgr->GetVfxVolume() * 0.05f, false);
	}
#endif

	//std::cout << "[PlayerLogic] PickUp item " << itemID << "\n";

	// Save original collider size
	carriedItemOriginalColliderSize = item->GetColliderSize();
	hasCarriedItemOriginalColliderSize = true;

	// Shrink collider so physics stops pushing the player around
	// (Adjust to your GameObject API if needed)
	item->SetColliderSize(Math::Vector2D(0.f, 0.f));

	// Snap once, then every frame we keep it following in UpdateCarriedItemTransform
	UpdateCarriedItemTransform(scene);
}

// Unity: Drop(Vector3 dropPos) � here: drop slightly in front of player
void PlayerLogic::Drop(Scene& scene) {
	if (carriedItemID < 0) {
		return;
	}

	GameObject* player = GetOwner(scene);
	GameObject* item = scene.GetGameObjectByID(carriedItemID);
	if (!player || !item) {
		//std::cout << "[PlayerLogic] Drop failed, invalid item or player. Clearing carriedItemID.\n";
		carriedItemID = -1;
		return;
	}

	//std::cout << "[PlayerLogic] Drop item " << carriedItemID << "\n";

	if (hasCarriedItemOriginalColliderSize) {
		item->SetColliderSize(carriedItemOriginalColliderSize);
		hasCarriedItemOriginalColliderSize = false;
	}

	glm::vec3 p = player->GetPositionGLM();
	item->SetPosition(glm::vec3(p.x + 16.f, p.y, p.z)); // simple �in front� drop
	carriedItemID = -1;
}

// Main update loop for player logic: handle input, movement, sprite updates, interactions, and footstep effects.
void PlayerLogic::HandleKeyboardMovement(float dt, Scene& scene, InputManager& input, GameObject* player, const glm::vec3& playerPos) {
	glm::vec2 inputDir(0.0f, 0.0f);
	if (input.IsKeyPressed(GLFW_KEY_A)) inputDir.x -= 1.0f;
	if (input.IsKeyPressed(GLFW_KEY_D)) inputDir.x += 1.0f;
	if (input.IsKeyPressed(GLFW_KEY_W)) inputDir.y -= 1.0f;
	if (input.IsKeyPressed(GLFW_KEY_S)) inputDir.y += 1.0f;

	if (inputDir.x != 0.0f || inputDir.y != 0.0f) {
		hasMoveTarget = false;
		const glm::vec2 normalizedInput = NormalizeOrZero(inputDir);
		const glm::vec2 desiredDelta = normalizedInput * kKeyboardMoveSpeed * dt;
		const glm::vec2 allowedDelta = scene.ResolveWorldStep(player, desiredDelta);

		glm::vec3 nextPos = playerPos;
		nextPos.x += allowedDelta.x;
		nextPos.y += allowedDelta.y;
		player->SetPosition(nextPos);
		scene.ClampToWalkArea(player);
		UpdateSprite(scene, player, normalizedInput);
		return;
	}

	if (hasMoveTarget) {
		UpdateSprite(scene, player, moveTarget - ToVec2(playerPos));
	}
	else {
		UpdateSprite(scene, player, glm::vec2(0.0f, 0.0f));
	}
}

// Emit footstep particles and play sounds based on movement. Particles are emitted along the path traveled, with some jitter for visual interest. Sounds are played at regular intervals while moving.
void PlayerLogic::UpdateFootstepTrailAndAudio(float dt, Scene& scene, InputManager& input, GameObject* player, const glm::vec3& beforePos, const glm::vec3& afterPos) {
	(void)dt;
	const bool hasIntent =
		input.IsKeyPressed(GLFW_KEY_A) || input.IsKeyPressed(GLFW_KEY_D) ||
		input.IsKeyPressed(GLFW_KEY_W) || input.IsKeyPressed(GLFW_KEY_S) ||
		hasMoveTarget;

	const glm::vec2 moveDelta(afterPos.x - beforePos.x, afterPos.y - beforePos.y);
	const bool actuallyMoved = std::sqrt(moveDelta.x * moveDelta.x + moveDelta.y * moveDelta.y) > kTrailJitterEpsilon;
	if (!hasIntent || !actuallyMoved) {
		hasLastTrailPos_ = false;
		trailCarry_ = 0.0f;
		footstepEmitTimer_ = 0.0f;
		return;
	}

#ifndef _DEBUG
	footstepEmitTimer_ += dt;
	if (footstepEmitTimer_ >= kFootstepInterval) {
		footstepEmitTimer_ = 0.0f;
		if (AudioManager* audioMgr = scene.GetAudioManager()) {
			audioMgr->PlaySound("sfx_step_1", audioMgr->GetVfxVolume() * 0.04f, false);
		}
	}
#endif

	glm::vec3 feet = afterPos;
	const auto co = player->GetColliderOffset();
	const auto scale = player->GetScaleGLM();
	feet.x += co.x;
	feet.y += co.y + (scale.y * 0.5f) - 6.0f;

	const glm::vec2 dir = NormalizeOrZero(moveDelta);
	if (!hasLastTrailPos_) {
		lastTrailPos_ = feet;
		hasLastTrailPos_ = true;
		trailCarry_ = 0.0f;
	}

	const glm::vec2 a(lastTrailPos_.x, lastTrailPos_.y);
	const glm::vec2 b(feet.x, feet.y);
	const glm::vec2 d = b - a;
	const float segmentDist = std::sqrt(d.x * d.x + d.y * d.y);
	if (segmentDist > 0.0001f) {
		const glm::vec2 segDir = d / segmentDist;
		const float spacing = 15.0f;
		const float total = segmentDist + trailCarry_;
		const int count = static_cast<int>(std::floor(total / spacing));

		for (int i = 0; i < count; ++i) {
			const float along = spacing * (i + 1) - trailCarry_;
			const glm::vec2 p2 = a + segDir * along;
			glm::vec3 trailPos(p2.x, p2.y, afterPos.z);

			const float behind = 20.0f;
			trailPos.x -= dir.x * behind;
			trailPos.y -= dir.y * behind;

			const glm::vec2 perp(-dir.y, dir.x);
			const float jitter = ((std::rand() % 1000) / 1000.0f - 0.5f) * 3.0f;
			trailPos.x += perp.x * jitter;
			trailPos.y += perp.y * jitter;

			scene.GetParticleSystem().EmitTrail(scene.GetEntityManager(), trailPos, afterPos.z, dir);
		}

		trailCarry_ = total - count * spacing;
	}

	lastTrailPos_ = feet;
}

// Main update loop for player logic: handle input, movement, sprite updates, interactions, and footstep effects.
void PlayerLogic::Update(float dt, Scene& scene, InputManager& input) {
	if (!scene.IsSimulationActive() || scene.IsPauseOverlayActive()) {
		ClearInteractableVisualCues(scene);
		return;
	}

	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	UpdateInteractableVisualCues(scene, input, dt);
	const glm::vec3 beforePos = player->GetPositionGLM();

	const float physicsDt = scene.GetLastPhysicsDt();
	const bool stepMode = scene.GetStepController().enabled;
	if (stepMode && physicsDt <= 0.0f) {
		HandleClickInput(scene, input, dt);
		return;
	}

	HandleKeyboardMovement(dt, scene, input, player, beforePos);
	HandleClickInput(scene, input, dt);
	UpdateClickMoveIndicator(scene, dt);
	UpdateMovement(dt, scene);

	const glm::vec3 afterPos = player->GetPositionGLM();
	UpdateFootstepTrailAndAudio(dt, scene, input, player, beforePos, afterPos);
	UpdateCarriedItemTransform(scene);
}

// Optional: visual cues for interactable objects under mouse cursor
void PlayerLogic::UpdateInteractableVisualCues(Scene& scene, InputManager& input, float dt) {
	(void)input;
	(void)dt;

	glm::vec2 mouseWorld{};
	const bool hasMouseWorld = scene.GetGraphicsEngine().GetMouseWorldInScene(mouseWorld);

	std::unordered_set<int> nextHighlighted;
	nextHighlighted.reserve(16);

	LogicManager& logicMgr = scene.GetLogicManager();

	for (GameObject* obj : scene.GetAllObjectsRaw()) {
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
		obj->SetColorTint(glm::vec4(1.22f, 1.22f, 0.74f, 1.0f));
	}

	for (int id : highlightedInteractableIDs_) {
		if (nextHighlighted.find(id) != nextHighlighted.end()) {
			continue;
		}
		if (GameObject* obj = scene.GetGameObjectByID(id)) {
			obj->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		}
	}

	highlightedInteractableIDs_ = std::move(nextHighlighted);
}

// Check if a world point is inside the object's collider (used for mouse hover)
bool PlayerLogic::IsPointInsideObjectCollider(const GameObject* obj, const glm::vec2& worldPoint) const {
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

// Show a temporary indicator (e.g. a circle) at the clicked position for click-to-move feedback
void PlayerLogic::ShowClickMoveIndicator(Scene& scene, const glm::vec2& worldPoint) {
	const glm::vec3 markerPos(worldPoint.x, worldPoint.y, 0.0f);

	GameObject* marker = scene.GetGameObjectByID(clickIndicatorID_);
	if (!marker) {
		marker = scene.SpawnStaticSprite("../assets/Coin.png", markerPos, glm::vec2(26.0f, 26.0f));
		if (!marker) {
			clickIndicatorID_ = -1;
			clickIndicatorTimeLeft_ = 0.0f;
			return;
		}

		clickIndicatorID_ = marker->GetID();
		marker->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
	}

	marker->SetPosition(markerPos);
	marker->SetScale(glm::vec3(26.0f, 26.0f, 1.0f));
	marker->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 0.95f));
	clickIndicatorTimeLeft_ = kClickIndicatorLifetime;
}

// Update the click move indicator (scaling and fading) and despawn when time is up
void PlayerLogic::UpdateClickMoveIndicator(Scene& scene, float dt) {
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

	const float t = std::clamp(clickIndicatorTimeLeft_ / kClickIndicatorLifetime, 0.0f, 1.0f);
	const float alpha = 0.25f + 0.70f * t;
	const float size = 18.0f + (1.0f - t) * 28.0f;
	marker->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, alpha));
	marker->SetScale(glm::vec3(size, size, 1.0f));
}

// Reset color tints on previously highlighted interactables
void PlayerLogic::ClearInteractableVisualCues(Scene& scene) {
	for (int id : highlightedInteractableIDs_) {
		if (GameObject* obj = scene.GetGameObjectByID(id)) {
			obj->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		}
	}

	highlightedInteractableIDs_.clear();
}

// Handle interaction logic when clicking on a table-like object
void PlayerLogic::InteractWithTable(Scene& scene, int tableObjectID) {
	//std::cout << "[PlayerLogic] InteractWithTable tableID=" << tableObjectID << "\n";

	GameObject* player = GetOwner(scene);
	if (!player) {
		return;
	}

	// Play interact audio for the table being interacted with
	scene.PlayInteractAudio(tableObjectID);

	// Get table logic for the clicked/selected GameObject
	LogicManager& logicMgr = scene.GetLogicManager();
	TableLogic* table = logicMgr.GetLogicForObject<TableLogic>(tableObjectID);
	if (!table) {
		//std::cout << "[PlayerLogic] InteractWithTable: no TableLogic found on that object\n";
		return;
	}

	bool playerHolding = (carriedItemID >= 0);
	bool tableHasItem = table->HasItem();

	//std::cout << "  [PlayerLogic] state: playerHolding=" << playerHolding
	//	<< " carriedItemID=" << carriedItemID
	//	<< " tableHasItem=" << tableHasItem
	//	<< " tableHeldItemID=" << (tableHasItem ? table->GetHeldItemID() : -1)
	//	<< "\n";

	// --- Special case: Ingredient box ---
	if (IngredientBoxLogic* box = logicMgr.GetLogicForObject<IngredientBoxLogic>(tableObjectID)) {
		//std::cout << "  [PlayerLogic] This table is an IngredientBox\n";

		if (carriedItemID >= 0) {
			//std::cout << "  [PlayerLogic] Already holding item " << carriedItemID
			//	<< ", ignoring ingredient box\n";
			return;
		}

		int newItemID = box->SpawnIngredient(scene);
		if (newItemID >= 0) {
			PickUp(scene, newItemID);  // auto-pickup
		}
		return; // Do not fall through to normal table logic
	}

	// Special case: Customer table payment
	if (CustomerTableLogic* ctable = logicMgr.GetLogicForObject<CustomerTableLogic>(tableObjectID)) {
		// If the customer is in Paying state, consume the click and stop here.
		if (ctable->TryTakePayment(scene)) {
			return;
		}
	}

	// Special case: Trash can
	// If this object is a trash can, placing an item should delete it immediately.
	if (TrashCanLogic* trash = logicMgr.GetLogicForObject<TrashCanLogic>(tableObjectID)) {
		// Only meaningful if player is holding something
		if (carriedItemID >= 0) {
			const int itemToTrash = carriedItemID;

			// Try to "place" it on the trash can (TrashCanLogic will despawn it)
			if (trash->PlaceItem(scene, itemToTrash)) {
				// Restore collider size if the object still exists this frame
				// (depending on when despawns are processed)
				if (hasCarriedItemOriginalColliderSize) {
					if (GameObject* item = scene.GetGameObjectByID(itemToTrash)) {
						item->SetColliderSize(carriedItemOriginalColliderSize);
					}
					hasCarriedItemOriginalColliderSize = false;
				}

				// Clear carried item
				carriedItemID = -1;
			}
		}

		// Either way, stop here — trash can shouldn't behave like normal tables.
		return;
	}

	playerHolding = (carriedItemID >= 0);
	tableHasItem = table->HasItem();

	// CASE 1: Player empty-handed, table has an item -> pick up
	if (!playerHolding && tableHasItem) {
		//std::cout << "  [PlayerLogic] CASE1: table has item, player empty -> TakeItem + PickUp\n";
		int itemID = table->TakeItem(scene);
		if (itemID >= 0) {
			PickUp(scene, itemID);
		}
		return;
	}

	// CASE 2: Player holding something, table is empty -> drop onto table
	if (playerHolding && !tableHasItem) {
		//std::cout << "  [PlayerLogic] CASE2: player holding " << carriedItemID
		//	<< ", table empty -> PlaceItem\n";

		if (table->CanAcceptItem(scene, carriedItemID)) {
			if (table->PlaceItem(scene, carriedItemID)) {
				//std::cout << "  [PlayerLogic] CASE2: PlaceItem success, clearing carriedItem\n";
				if (hasCarriedItemOriginalColliderSize) {
					if (GameObject* item = scene.GetGameObjectByID(carriedItemID))
						item->SetColliderSize(carriedItemOriginalColliderSize);
					hasCarriedItemOriginalColliderSize = false;
				}
				carriedItemID = -1;

				// Play put down sound effect (release mode only)
#ifndef _DEBUG
				if (AudioManager* audioMgr = scene.GetAudioManager()) {
					audioMgr->PlaySound("sfx_put_down", audioMgr->GetVfxVolume() * 0.4f, false);
				}
#endif
			}
			else {
				//std::cout << "  [PlayerLogic] CASE2: PlaceItem FAILED\n";
			}
		}
		else {
			//std::cout << "  [PlayerLogic] CASE2: CanAcceptItem = false\n";
		}
		return;
	}

	// CASE 3: Player holding something, table already has an item
	//   -> typical case: table has a Plate, player has an Ingredient
	if (playerHolding && tableHasItem) {
		//std::cout << "  [PlayerLogic] CASE3: both player & table have items -> try plate+ingredient combo\n";

		const int tableItemID = table->GetHeldItemID();

		PlateLogic* plate = logicMgr.GetLogicForObject<PlateLogic>(tableItemID);
		IngredientLogic* ingr = logicMgr.GetLogicForObject<IngredientLogic>(carriedItemID);

		if (plate && ingr) {
			// GameObject ID of the ingredient we are currently carrying
			const int ingredientObjID = ingr->GetOwnerID();

			// How many logical ingredients were on the plate BEFORE we add this one?
			const int ingredientCountBefore = plate->GetIngredientCount();

			bool consumedNow = false;
			if (plate->TryAddIngredient(*ingr, consumedNow)) {
				//std::cout << "  [PlayerLogic] CASE3: plate accepted ingredient type\n";

				// VISUAL: first ingredient goes onto the plate visually
				if (ingredientCountBefore == 0) // this is the first ingredient on this plate
				{
					GameObject* plateObj = scene.GetGameObjectByID(tableItemID);
					GameObject* ingredientObj = scene.GetGameObjectByID(ingredientObjID);

					if (plateObj && ingredientObj) {
						glm::vec3 platePos = plateObj->GetPositionGLM();

						// Snap the ingredient sprite onto the plate.
						// If you want a small offset, tweak this, e.g. platePos.y + 8.f.
						ingredientObj->SetPosition(platePos);

						// Remember which GameObject is now visually sitting on this plate
						plate->SetFirstIngredientObjectID(ingredientObjID);
					}
				}

				// Try to assemble a dish once we have enough ingredients
				DishType dishType;
				std::vector<IngredientType> consumedTypes;
				if (plate->TryAssembleDish(dishType, consumedTypes)) {
					//std::cout << "  [PlayerLogic] CASE3: Dish assembled on plate\n";
						// Update visuals based on computed dish type
					plate->ApplyDishVisual(scene);

					// Destroy the first ingredient that was sitting on the plate (if any)
					int firstObjID = plate->GetFirstIngredientObjectID();
					if (firstObjID >= 0) {
						scene.DespawnByID(firstObjID);
					}

					// Destroy the ingredient we just added (the one we were carrying)
					if (ingredientObjID >= 0 && ingredientObjID != firstObjID) {
						scene.DespawnByID(ingredientObjID);
					}

					// We won't restore its collider size because the object is gone.
					hasCarriedItemOriginalColliderSize = false;
				}


				// Either way, we are no longer carrying this item
				carriedItemID = -1;
				//std::cout << "  [PlayerLogic] CASE3: plate accepted ingredient; carriedItem cleared\n";

				// Play put down sound effect (release mode only)
#ifndef _DEBUG
				if (AudioManager* audioMgr = scene.GetAudioManager()) {
					audioMgr->PlaySound("sfx_put_down", audioMgr->GetVfxVolume() * 0.4f, false);
				}
#endif
			}
			else {
				//std::cout << "  [PlayerLogic] CASE3: plate REJECTED ingredient\n";
			}
			return;
		}

		//std::cout << "  [PlayerLogic] CASE3: no (plate,ingredient) combo found\n";
		return;
	}

	//std::cout << "  [PlayerLogic] No case matched, doing nothing.\n";
}

// Update the position of the carried item to follow the player with an offset
void PlayerLogic::UpdateCarriedItemTransform(Scene& scene) {
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

	glm::vec3 p = player->GetPositionGLM();
	item->SetPosition(glm::vec3(p.x + carryOffset.x, p.y + carryOffset.y, p.z));

	////Optional debug
	//std::cout << "[PlayerLogic] Updating carried item " << carriedItemID
	//	<< " to follow player at (" << p.x + carryOffset.x << ", "
	//	<< p.y + carryOffset.y << ")\n";
}
