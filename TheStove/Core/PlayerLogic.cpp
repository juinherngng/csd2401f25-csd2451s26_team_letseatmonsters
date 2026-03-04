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
#include "../Graphics/SceneManager.hpp"
#include "../Core/AudioManager.hpp"
#include "../Core/InputManager.hpp"
#include "../Core/InputControls.hpp"
#include "../Core/TableLogic.hpp"
#include "../Core/WorkTableLogic.hpp"
#include "../Core/CustomerTableLogic.hpp"
#include "../Core/TrashCanLogic.hpp"
#include <iostream>

#include "../Core/DebugUI.hpp"
#include "../Core/PlayerLogic.hpp"
#include "../Core/TableLogic.hpp"
#include "../Core/WorkTableLogic.hpp"
#include "../Core/SimpleNpcLogic.hpp"
#include "../Core/CustomerOrderUILogic.hpp"

#include "PlayerLogic.hpp"


#include <iostream>

namespace {
	bool PointInsideObjectVisualRect(const glm::vec2& point, GameObject* obj)
	{
		if (!obj) return false;

		const Math::Vector2D colSize = obj->GetColliderSize();
		const Math::Vector2D colOffset = obj->GetColliderOffset();
		const glm::vec3 scale = obj->GetScaleGLM();

		const float width = (colSize.x > 0.0f) ? colSize.x : scale.x;
		const float height = (colSize.y > 0.0f) ? colSize.y : scale.y;

		if (width <= 0.0f || height <= 0.0f) {
			return false;
		}

		const glm::vec3 pos = obj->GetPositionGLM();
		const glm::vec2 center(pos.x + colOffset.x, pos.y + colOffset.y);

		const float halfW = width * 0.5f;
		const float halfH = height * 0.5f;

		return
			point.x >= center.x - halfW && point.x <= center.x + halfW &&
			point.y >= center.y - halfH && point.y <= center.y + halfH;
	}

	float DistanceSqToObjectCenter(const glm::vec2& point, GameObject* obj)
	{
		if (!obj) return std::numeric_limits<float>::max();

		const Math::Vector2D colOffset = obj->GetColliderOffset();
		const glm::vec3 pos = obj->GetPositionGLM();
		const glm::vec2 center(pos.x + colOffset.x, pos.y + colOffset.y);

		const glm::vec2 d = point - center;
		return d.x * d.x + d.y * d.y;
	}
}

void PlayerLogic::Start(Scene& scene)
{
	(void)scene;
	hasMoveTarget = false;
	carriedItemID = -1;
	pendingTableID = -1;
	facingDir = FacingDir::Front;
	moveMode_ = MoveMode::None;

	pathPoints_.clear();
	pathIndex_ = 0;
	finalTarget_ = glm::vec2(0.0f, 0.0f);
	directPathCheckTimer_ = 0.0f;
}

// Decide and apply sprite based on movement direction
void PlayerLogic::UpdateSprite(Scene& scene, GameObject* player, const glm::vec2& moveDirRaw) {
	(void)scene;
	if (!player) return;

	const float moveThreshold = 0.01f;
	std::string desiredAnimation;
	const bool isHolding = carriedItemID >= 0;

	float absX = std::abs(moveDirRaw.x);
	float absY = std::abs(moveDirRaw.y);

	// Detect idle/no movement
	if (glm::length(moveDirRaw) < moveThreshold) {
		switch (facingDir) {
		case FacingDir::Right: desiredAnimation = isHolding ? "IDLE_RIGHT_CARRY" : "IDLE_RIGHT"; break;
		case FacingDir::Left:  desiredAnimation = isHolding ? "IDLE_LEFT_CARRY" : "IDLE_LEFT";  break;
		case FacingDir::Front: desiredAnimation = isHolding ? "IDLE_FRONT" : "IDLE_FRONT"; break;			// Using normal idle for front since we dont have idle front carry animations yet
		case FacingDir::Back:  desiredAnimation = isHolding ? "IDLE_BACK" : "IDLE_BACK";  break;			// Using normal idle for back since we dont have idle back carry animations yet
		}
	}
	else {
		if (absX > absY) {
			if (moveDirRaw.x > 0.0f) {
				desiredAnimation = isHolding ? "CARRY_RIGHT" : "WALK_RIGHT";
				facingDir = FacingDir::Right;
			}
			else {
				desiredAnimation = isHolding ? "CARRY_LEFT" : "WALK_LEFT";
				facingDir = FacingDir::Left;
			}
		}
		else {
			if (moveDirRaw.y > 0.0f) {
				desiredAnimation = isHolding ? "CARRY_FRONT" : "WALK_FRONT";
				facingDir = FacingDir::Front;
			}
			else {
				desiredAnimation = isHolding ? "CARRY_BACK" : "WALK_BACK";
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

void PlayerLogic::MoveDirect(const glm::vec2& dest)
{
	const float kRetargetEpsSq = 16.0f * 16.0f;

	if (hasMoveTarget && moveMode_ == MoveMode::Direct) {
		glm::vec2 d = dest - finalTarget_;
		if ((d.x * d.x + d.y * d.y) <= kRetargetEpsSq) {
			return;
		}
	}

	finalTarget_ = dest;
	moveTarget = dest;
	pathPoints_.clear();
	pathIndex_ = 0;
	hasMoveTarget = true;
	moveMode_ = MoveMode::Direct;
}

void PlayerLogic::MoveTo(Scene& scene, const glm::vec2& dest)
{
	GameObject* player = GetOwner(scene);
	if (!player)
		return;

	glm::vec2 snappedDest = dest;

	// Snap interactable destinations to the nearest walkable cell center
	scene.GetNearestNavigationCellCenterForObject(player->GetID(), dest, snappedDest);

	const float kRetargetEpsSq = 16.0f * 16.0f;

	if (hasMoveTarget && moveMode_ == MoveMode::Pathfinding) {
		glm::vec2 d = snappedDest - finalTarget_;
		if ((d.x * d.x + d.y * d.y) <= kRetargetEpsSq) {
			return;
		}
	}

	finalTarget_ = snappedDest;
	pathPoints_.clear();
	pathIndex_ = 0;
	hasMoveTarget = false;
	moveMode_ = MoveMode::Pathfinding;

	const glm::vec3 pos3 = player->GetPositionGLM();
	const glm::vec2 startPos(pos3.x, pos3.y);

	if (!scene.FindPathForObject(player->GetID(), startPos, finalTarget_, pathPoints_)) {
		std::cout << "[PlayerLogic] No path found.\n";
		pendingTableID = -1;
		moveMode_ = MoveMode::None;
		return;
	}

	while (!pathPoints_.empty()) {
		glm::vec2 d = pathPoints_.front() - startPos;
		const float kSkipWaypointRadius = 18.0f;
		if ((d.x * d.x + d.y * d.y) <= kSkipWaypointRadius * kSkipWaypointRadius) {
			pathPoints_.erase(pathPoints_.begin());
		}
		else {
			break;
		}
	}

	if (pathPoints_.empty()) {
		moveMode_ = MoveMode::None;
		OnArrived(scene);
		return;
	}

	hasMoveTarget = true;
	moveTarget = pathPoints_[0];
}

void PlayerLogic::HandleClickInput(Scene& scene, InputManager& input)
{
	// Only once per click (left mouse)
	if (!input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		return;
	}

	GameObject* player = GetOwner(scene);
	if (!player) return;

	glm::vec2 mouseWorld{};
	if (!scene.GetGraphicsEngine().GetMouseWorldInScene(mouseWorld)) {
		return;
	}

	LogicManager& logicMgr = scene.GetLogicManager();

	int clickedTableID = -1;
	TableLogic* clickedTableLogic = nullptr;
	float bestDistSq = std::numeric_limits<float>::max();

	auto considerTableTarget = [&](int tableID, GameObject* hitObject)
		{
			if (tableID < 0 || !hitObject) {
				return;
			}

			TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(tableID);
			if (!tableLogic) {
				return;
			}

			const float distSq = DistanceSqToObjectCenter(mouseWorld, hitObject);
			if (distSq < bestDistSq) {
				bestDistSq = distSq;
				clickedTableID = tableID;
				clickedTableLogic = tableLogic;
			}
		};

	// ----------------------------------------------------------
	// 1) Direct table clicks
	// ----------------------------------------------------------
	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		if (!obj) continue;

		const int id = obj->GetID();
		TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(id);
		if (!tableLogic) continue;

		if (!PointInsideObjectVisualRect(mouseWorld, obj)) {
			continue;
		}

		considerTableTarget(id, obj);
	}

	// ----------------------------------------------------------
	// 2) Customer sprite clicks OR real spawned bubble clicks
	//    -> redirect to that customer's table
	// ----------------------------------------------------------
	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		if (!obj) continue;

		const int id = obj->GetID();

		SimpleNpcLogic* npcLogic = logicMgr.GetLogicForObject<SimpleNpcLogic>(id);
		if (!npcLogic) continue;

		const int customerTableID = npcLogic->GetCustomerTableID();
		if (customerTableID < 0) continue;

		const bool hitCustomer = PointInsideObjectVisualRect(mouseWorld, obj);

		bool hitBubble = false;
		if (CustomerOrderUILogic* uiLogic = logicMgr.GetLogicForObject<CustomerOrderUILogic>(id)) {
			hitBubble = uiLogic->HitTestBubble(scene, mouseWorld);
		}

		if (!hitCustomer && !hitBubble) {
			continue;
		}

		considerTableTarget(customerTableID, obj);
	}

	// ----------------------------------------------------------
	// 3) If we resolved to a table, move to its approach point
	// ----------------------------------------------------------
	if (clickedTableID >= 0 && clickedTableLogic) {
		pendingTableID = clickedTableID;

		glm::vec3 playerPos3 = player->GetPositionGLM();
		glm::vec2 start(playerPos3.x, playerPos3.y);

		Math::Vector2D from(playerPos3.x, playerPos3.y);
		Math::Vector2D approach = clickedTableLogic->GetClosestApproachPoint(scene, from);

		glm::vec2 target(approach.x, approach.y);

		glm::vec2 snappedTarget = target;
		scene.GetNearestNavigationCellCenterForObject(player->GetID(), target, snappedTarget);

		if (scene.HasDirectPathForObject(player->GetID(), start, snappedTarget)) {
			MoveDirect(snappedTarget);
		}
		else {
			MoveTo(scene, target);
		}
	}
	else {
		pendingTableID = -1;

		glm::vec3 playerPos3 = player->GetPositionGLM();
		glm::vec2 start(playerPos3.x, playerPos3.y);

		if (scene.HasDirectPathForObject(player->GetID(), start, mouseWorld)) {
			MoveDirect(mouseWorld);
		}
		else {
			MoveTo(scene, mouseWorld);
		}
	}
}


// Move owner GameObject towards moveTarget at moveSpeed
void PlayerLogic::UpdateMovement(float dt, Scene& scene) {
	if (!hasMoveTarget)
		return;

	GameObject* player = GetOwner(scene);
	if (!player)
		return;

	glm::vec3 pos3 = player->GetPositionGLM();
	glm::vec2 pos(pos3.x, pos3.y);

	// -------------------------------------------------
// LIVE SHORTCUT CHECK:
// while following an A* path, if final target is now
// directly reachable, switch immediately to direct mode
// -------------------------------------------------
	if (moveMode_ == MoveMode::Pathfinding) {
		directPathCheckTimer_ -= dt;

		if (directPathCheckTimer_ <= 0.0f) {
			directPathCheckTimer_ = kDirectPathCheckInterval;

			if (scene.HasDirectPathForObject(player->GetID(), pos, finalTarget_)) {
				moveMode_ = MoveMode::Direct;
				moveTarget = finalTarget_;
				pathPoints_.clear();
				pathIndex_ = 0;
				hasMoveTarget = true;
			}
		}
	}

	const float arriveRadius = 6.0f;
	const float arriveRadiusSq = arriveRadius * arriveRadius;

	// ---------------------------
	// DIRECT FREE MOVEMENT MODE
	// ---------------------------
	if (moveMode_ == MoveMode::Direct)
	{
		moveTarget = finalTarget_;

		glm::vec2 dir = moveTarget - pos;
		float distSq = dir.x * dir.x + dir.y * dir.y;

		if (distSq <= arriveRadiusSq) {
			hasMoveTarget = false;
			moveMode_ = MoveMode::None;
			pathPoints_.clear();
			pathIndex_ = 0;

			if (GameObject* p = GetOwner(scene)) {
				scene.GetMovementManager().ClearMoveTarget(p->GetID());
			}

			OnArrived(scene);
			return;
		}

		float dist = std::sqrt(distSq);
		if (dist > 0.0001f) {
			dir.x /= dist;
			dir.y /= dist;
		}

		float step = moveSpeed * dt;
		if (step > dist)
			step = dist;

		glm::vec2 desiredDelta(dir.x * step, dir.y * step);
		glm::vec2 allowedDelta = scene.ResolveWorldStep(player, desiredDelta);

		const float allowedLenSq =
			allowedDelta.x * allowedDelta.x +
			allowedDelta.y * allowedDelta.y;

		if (allowedLenSq < 0.0001f) {
			std::vector<glm::vec2> newPath;

			if (scene.FindPathForObject(player->GetID(), pos, finalTarget_, newPath)) {
				pathPoints_ = newPath;
				pathIndex_ = 0;
				moveMode_ = MoveMode::Pathfinding;
				hasMoveTarget = !pathPoints_.empty();

				while (!pathPoints_.empty()) {
					glm::vec2 d = pathPoints_.front() - pos;
					if ((d.x * d.x + d.y * d.y) <= arriveRadiusSq) {
						pathPoints_.erase(pathPoints_.begin());
					}
					else {
						break;
					}
				}

				if (!pathPoints_.empty()) {
					moveTarget = pathPoints_.front();
					return;
				}
			}

			hasMoveTarget = false;
			moveMode_ = MoveMode::None;
			pathPoints_.clear();
			pathIndex_ = 0;
			pendingTableID = -1;

			if (GameObject* p = GetOwner(scene)) {
				scene.GetMovementManager().ClearMoveTarget(p->GetID());
			}
			return;
		}

		pos.x += allowedDelta.x;
		pos.y += allowedDelta.y;

		player->SetPosition(glm::vec3(pos.x, pos.y, pos3.z));
		scene.ClampToWalkArea(player);

		UpdateSprite(scene, player, allowedDelta);
		return;
	}

	// ---------------------------
	// PATHFINDING MODE
	// ---------------------------
	if (pathPoints_.empty()) {
		hasMoveTarget = false;
		moveMode_ = MoveMode::None;
		return;
	}

	// Advance waypoint(s) if already reached
	while (pathIndex_ < pathPoints_.size()) {
		glm::vec2 toWaypoint = pathPoints_[pathIndex_] - pos;
		float distSq = toWaypoint.x * toWaypoint.x + toWaypoint.y * toWaypoint.y;

		if (distSq <= arriveRadiusSq) {
			++pathIndex_;
		}
		else {
			break;
		}
	}

	if (pathIndex_ >= pathPoints_.size()) {
		hasMoveTarget = false;
		moveMode_ = MoveMode::None;
		pathPoints_.clear();
		pathIndex_ = 0;

		if (GameObject* p = GetOwner(scene)) {
			scene.GetMovementManager().ClearMoveTarget(p->GetID());
		}

		OnArrived(scene);
		return;
	}

	moveTarget = pathPoints_[pathIndex_];

	glm::vec2 dir = moveTarget - pos;
	float distSq = dir.x * dir.x + dir.y * dir.y;
	float dist = std::sqrt(distSq);

	if (dist > 0.0001f) {
		dir.x /= dist;
		dir.y /= dist;
	}

	float step = moveSpeed * dt;
	if (step > dist)
		step = dist;

	glm::vec2 desiredDelta(dir.x * step, dir.y * step);
	glm::vec2 allowedDelta = scene.ResolveWorldStep(player, desiredDelta);

	const float allowedLenSq =
		allowedDelta.x * allowedDelta.x +
		allowedDelta.y * allowedDelta.y;

	if (allowedLenSq < 0.0001f) {
		std::vector<glm::vec2> newPath;

		if (scene.FindPathForObject(player->GetID(), pos, finalTarget_, newPath)) {
			pathPoints_ = newPath;
			pathIndex_ = 0;

			while (!pathPoints_.empty()) {
				glm::vec2 d = pathPoints_.front() - pos;
				if ((d.x * d.x + d.y * d.y) <= arriveRadiusSq) {
					pathPoints_.erase(pathPoints_.begin());
				}
				else {
					break;
				}
			}

			if (!pathPoints_.empty()) {
				moveTarget = pathPoints_.front();
				return;
			}
		}

		std::cout << "[PlayerLogic] Path blocked and repath failed.\n";

		hasMoveTarget = false;
		moveMode_ = MoveMode::None;
		pathPoints_.clear();
		pathIndex_ = 0;
		pendingTableID = -1;

		if (GameObject* p = GetOwner(scene)) {
			scene.GetMovementManager().ClearMoveTarget(p->GetID());
		}
		return;
	}

	pos.x += allowedDelta.x;
	pos.y += allowedDelta.y;

	player->SetPosition(glm::vec3(pos.x, pos.y, pos3.z));
	scene.ClampToWalkArea(player);

	UpdateSprite(scene, player, allowedDelta);

	if (DebugRenderer::IsEnabled() && hasMoveTarget) {
		glm::vec2 prev(pos.x, pos.y);

		for (std::size_t i = pathIndex_; i < pathPoints_.size(); ++i) {
			glm::vec2 next = pathPoints_[i];
			DebugRenderer::DrawLine(
				glm::vec3(prev.x, prev.y, pos3.z),
				glm::vec3(next.x, next.y, pos3.z),
				glm::vec3(0.0f, 1.0f, 0.0f)
			);
			prev = next;
		}
	}
}

// Unity: OnArrived()
// For now it's a stub; later you can branch by what we clicked (tables, spawners, etc.)
void PlayerLogic::OnArrived(Scene& scene) {
	//std::cout << "[PlayerLogic] Arrived at destination\n";

	if (pendingTableID < 0)
		return;

	GameObject* player = GetOwner(scene);
	GameObject* tableObj = scene.GetGameObjectByID(pendingTableID);
	if (!player || !tableObj) {
		pendingTableID = -1;
		return;
	}

	glm::vec3 pPos = player->GetPositionGLM();

	// Get the table logic so we can ask for its approach point
	LogicManager& logicMgr = scene.GetLogicManager();
	TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(pendingTableID);

	float distSq = 0.0f;

	if (tableLogic) {
		// Use the SAME approach-point logic that we used when clicking.
		Math::Vector2D from(pPos.x, pPos.y);
		Math::Vector2D approach = tableLogic->GetClosestApproachPoint(scene, from);

		float dx = pPos.x - approach.x;
		float dy = pPos.y - approach.y;
		distSq = dx * dx + dy * dy;

		//std::cout << "[PlayerLogic] Dist to table APPROACH point: "
		//	<< std::sqrt(distSq)
		//	<< " (approach=(" << approach.x << ", " << approach.y << "))\n";
	}
	else {
		// Fallback: no TableLogic (shouldn’t really happen for tables)
		glm::vec3 tPos = tableObj->GetPositionGLM();
		float dx = pPos.x - tPos.x;
		float dy = pPos.y - tPos.y;
		distSq = dx * dx + dy * dy;

		//std::cout << "[PlayerLogic] Dist to table ORIGIN (fallback): "
		//	<< std::sqrt(distSq) << "\n";
	}

	// Interaction radius around the approach point
	constexpr float kInteractRadius = 67.0f;  // tweak to taste

	if (distSq <= kInteractRadius * kInteractRadius) {
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


// Unity: PickUp(GameObject item) � here by engine ID
void PlayerLogic::PickUp(Scene& scene, int itemID) {
	GameObject* item = scene.GetGameObjectByID(itemID);
	GameObject* player = GetOwner(scene);
	if (!item || !player)
		return;

	carriedItemID = itemID;

	// Store original layer so we can restore on drop
	carriedItemOriginalLayer_ = scene.GetObjectLayer(itemID);
	hasCarriedItemOriginalLayer_ = true;

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
	if (carriedItemID < 0)
		return;

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

	// Restore original layer so it goes back to normal rendering order
	RestoreCarriedItemLayer(scene, carriedItemID);

	glm::vec3 p = player->GetPositionGLM();
	item->SetPosition(glm::vec3(p.x + 16.f, p.y, p.z)); // simple �in front� drop
	carriedItemID = -1;
}

//void PlayerLogic::HandleScaleInput(GameObject* player, InputManager& input, float dt)
//{
//	(void)dt;
//	if (!player) return;
//
//	glm::vec3 scale = player->GetScaleGLM();
//
//	if (input.IsKeyPressed(GLFW_KEY_UP)) {
//		scale *= 1.01f;
//		scale = glm::min(scale, glm::vec3(500.0f));
//		player->SetScale(scale);
//	}
//
//	if (input.IsKeyPressed(GLFW_KEY_DOWN)) {
//		scale *= 0.99f;
//		scale = glm::max(scale, glm::vec3(50.0f));
//		player->SetScale(scale);
//	}
//}

//void PlayerLogic::HandleRotationInput(GameObject* player, InputManager& input, float dt)
//{
//	if (!player) return;
//
//	const float kRotationSpeed = 10.0f; // degrees per second
//
//	if (input.IsKeyPressed(GLFW_KEY_RIGHT)) {
//		rotation_ += kRotationSpeed * dt;
//	}
//	if (input.IsKeyPressed(GLFW_KEY_LEFT)) {
//		rotation_ -= kRotationSpeed * dt;
//	}
//
//	// Normalize to [0, 360)
//	while (rotation_ >= 360.0f) rotation_ -= 360.0f;
//	while (rotation_ < 0.0f)   rotation_ += 360.0f;
//
//	player->SetRotation(rotation_, glm::vec3(0, 0, 1));
//}


void PlayerLogic::Update(float dt, Scene& scene, InputManager& input) {
	// Stop player logic when paused/overlay is active
	if (!scene.IsSimulationActive() || scene.IsPauseOverlayActive()) {
		return;
	}

	GameObject* player = GetOwner(scene);
	if (!player) return;

	if (!scene.IsObjectLayerEnabled(player->GetID())) {
		return;
	}

	glm::vec3 beforePos = player->GetPositionGLM();
	const float physicsDt = scene.GetLastPhysicsDt();
	const physics::StepController& step = scene.GetStepController();
	const bool stepMode = step.enabled;

	// --- Lock movement if we're using cutting board ---
	UpdateStationLock(scene);

	if (movementLocked_) {
		// Ensure we don't keep any stale move target
		hasMoveTarget = false;
		moveMode_ = MoveMode::None;
		scene.GetMovementManager().ClearMoveTarget(player->GetID());

		// Check if we should be playing the chopping animation (only if we're locked to a work table and it's currently processing)
		if (ShouldPlayChopAnimation(scene)) {
			EnsureChopAnimation(scene, player);
		}
		else {
			// Stay idle (keeps facing direction from last movement)
			UpdateSprite(scene, player, glm::vec2(0.f, 0.f));
		}

		// Still keep held item visually attached
		UpdateCarriedItemTransform(scene);
		return;
	}

	if (stepMode && physicsDt <= 0.0f) {
		// Optional: still allow click selection while frozen
		//std::cout << "HANDLE CLICK INPUT FREONZE\n";
		HandleClickInput(scene, input);

		// Debug: prove we still see the key
		if (input.IsKeyJustPressed(GLFW_KEY_P)) {
			//std::cout << "[PlayerLogic] P pressed (step mode, frozen)\n";
		}

		return; // skip movement while paused
	}

	glm::vec3 pos3 = player->GetPositionGLM();
	glm::vec2 inputDir(0.f, 0.f);

	// Get keyboard input
	if (input.IsKeyPressed(GLFW_KEY_A)) inputDir.x -= 1.f;
	if (input.IsKeyPressed(GLFW_KEY_D)) inputDir.x += 1.f;
	if (input.IsKeyPressed(GLFW_KEY_W)) inputDir.y -= 1.f;
	if (input.IsKeyPressed(GLFW_KEY_S)) inputDir.y += 1.f;

	// Main keyboard movement
	if (inputDir.x != 0.f || inputDir.y != 0.f) {
		hasMoveTarget = false;
		moveMode_ = MoveMode::None;
		pathPoints_.clear();
		pathIndex_ = 0;
		pendingTableID = -1;

		float len = std::sqrt(inputDir.x * inputDir.x + inputDir.y * inputDir.y);
		if (len > 0.0001f) {
			inputDir.x /= len;
			inputDir.y /= len;
		}

		// Desired movement this frame
		glm::vec2 desiredDelta(inputDir.x * moveSpeed * dt,
			inputDir.y * moveSpeed * dt);

		// Trim against static world (outer frame + wood + gate)
		glm::vec2 allowedDelta = scene.ResolveWorldStep(player, desiredDelta);

		pos3.x += allowedDelta.x;
		pos3.y += allowedDelta.y;

		player->SetPosition(pos3);

		// Optional: still clamp to overall walk rectangle if you want a hard outer bound
		scene.ClampToWalkArea(player);

		// Update sprite based on keyboard movement
		UpdateSprite(scene, player, inputDir);
	}
	// If has click-to-move target, follow that
	else if (hasMoveTarget) {
		glm::vec2 pos(pos3.x, pos3.y);
		glm::vec2 moveDir = moveTarget - pos;
		// Move player toward target
		// Set animation based on moveDir
		UpdateSprite(scene, player, moveDir); // Pass click-move vector 
	}
	else {
		// Idle: pass zero movement vector
		UpdateSprite(scene, player, glm::vec2(0.f, 0.f));
	}

	//std::cout << "HANDLE CLICK INPUT\n";

	HandleClickInput(scene, input);

	UpdateMovement(dt, scene);

	// Footstep trail: path-interpolated emission (prevents gaps at high speed)
	glm::vec3 afterPos = player->GetPositionGLM();

	// movement intent avoids spam from clamp jitter
	bool hasIntent =
		input.IsKeyPressed(GLFW_KEY_A) || input.IsKeyPressed(GLFW_KEY_D) ||
		input.IsKeyPressed(GLFW_KEY_W) || input.IsKeyPressed(GLFW_KEY_S) ||
		hasMoveTarget;

	glm::vec2 moveDelta(afterPos.x - beforePos.x, afterPos.y - beforePos.y);
	float dist = std::sqrt(moveDelta.x * moveDelta.x + moveDelta.y * moveDelta.y);

	// ignore micro jitter
	const float jitterEps = 0.25f;
	bool actuallyMoved = dist > jitterEps;

	if (hasIntent && actuallyMoved) {
		// Play footstep sound at regular intervals (release mode only)
#ifndef _DEBUG
		footstepEmitTimer_ += dt;
		const float footstepInterval = 0.3f; // seconds between footstep sounds
		if (footstepEmitTimer_ >= footstepInterval) {
			footstepEmitTimer_ = 0.0f;
			if (AudioManager* audioMgr = scene.GetAudioManager()) {
				audioMgr->PlaySound("sfx_step_1", audioMgr->GetVfxVolume() * 0.04f, false);
			}
		}
#endif

		// Feet position from collider size
		glm::vec3 feet = afterPos;
		auto co = player->GetColliderOffset();
		auto scale = player->GetScaleGLM();
		feet.x += co.x;
		feet.y += co.y + (scale.y * 0.5f) - 6.0f;

		// Move direction (normalized) used for particle velocity shaping
		glm::vec2 dir = moveDelta;
		float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
		if (len > 0.0001f) {
			dir.x /= len;
			dir.y /= len;
		}

		// Initialize last point on first valid movement frame
		if (!hasLastTrailPos_) {
			lastTrailPos_ = feet;
			hasLastTrailPos_ = true;
			trailCarry_ = 0.0f;
		}

		// Interpolate from lastTrailPos_ to feet, spawn evenly spaced particles
		glm::vec2 a(lastTrailPos_.x, lastTrailPos_.y);
		glm::vec2 b(feet.x, feet.y);
		glm::vec2 d = b - a;

		float segmentDist = std::sqrt(d.x * d.x + d.y * d.y);
		if (segmentDist > 0.0001f) {
			glm::vec2 segDir = d / segmentDist;

			const float spacing = 15.0f; // tune: smaller = denser trail
			float total = segmentDist + trailCarry_;
			int count = (int)std::floor(total / spacing);

			// emit along the path
			for (int i = 0; i < count; ++i) {
				float along = spacing * (i + 1) - trailCarry_;
				glm::vec2 p2 = a + segDir * along;

				// spawn behind movement direction
				glm::vec3 trailPos(p2.x, p2.y, afterPos.z);
				const float behind = 20.0f;
				trailPos.x -= dir.x * behind;
				trailPos.y -= dir.y * behind;

				// slight sideways jitter
				glm::vec2 perp(-dir.y, dir.x);
				float jitter = ((std::rand() % 1000) / 1000.0f - 0.5f) * 3.0f;
				trailPos.x += perp.x * jitter;
				trailPos.y += perp.y * jitter;

				scene.GetParticleSystem().EmitTrail(scene.GetEntityManager(), trailPos, afterPos.z, dir);
			}

			trailCarry_ = total - count * spacing;
		}

		lastTrailPos_ = feet;
	}
	else {
		// reset when not moving (prevents burst when resuming)
		hasLastTrailPos_ = false;
		trailCarry_ = 0.0f;
		footstepEmitTimer_ = 0.0f; // reset footstep timer when stopped
	}

	UpdateCarriedItemTransform(scene);

	// Debug key to prove script is running
	if (input.IsKeyJustPressed(GLFW_KEY_P)) {
		//std::cout << "[PlayerLogic] P pressed\n";
	}
}

void PlayerLogic::InteractWithTable(Scene& scene, int tableObjectID)
{
	//std::cout << "[PlayerLogic] InteractWithTable tableID=" << tableObjectID << "\n";

	GameObject* player = GetOwner(scene);
	if (!player)
		return;

	// Play interact audio for the table being interacted with
	scene.PlayInteractAudio(tableObjectID);

	// Get table logic for the clicked/selected GameObject
	LogicManager& logicMgr = scene.GetLogicManager();
	TableLogic* table = logicMgr.GetLogicForObject<TableLogic>(tableObjectID);
	if (!table)
	{
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
	if (IngredientBoxLogic* box = logicMgr.GetLogicForObject<IngredientBoxLogic>(tableObjectID))
	{
		//std::cout << "  [PlayerLogic] This table is an IngredientBox\n";

		if (carriedItemID >= 0)
		{
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

	// --- Special case: Customer table payment ---
	if (CustomerTableLogic* ctable = logicMgr.GetLogicForObject<CustomerTableLogic>(tableObjectID))
	{
		// If the customer is in Paying state, consume the click and stop here.
		if (ctable->TryTakePayment(scene)) {
			return;
		}
	}

	// --- Special case: Trash can ---
// If this object is a trash can, placing an item should delete it immediately.
	if (TrashCanLogic* trash = logicMgr.GetLogicForObject<TrashCanLogic>(tableObjectID))
	{
		// Only meaningful if player is holding something
		if (carriedItemID >= 0)
		{
			const int itemToTrash = carriedItemID;

			// Try to "place" it on the trash can (TrashCanLogic will despawn it)
			if (trash->PlaceItem(scene, itemToTrash))
			{
				// Restore collider size if the object still exists this frame
				// (depending on when despawns are processed)
				if (hasCarriedItemOriginalColliderSize)
				{
					if (GameObject* item = scene.GetGameObjectByID(itemToTrash))
					{
						item->SetColliderSize(carriedItemOriginalColliderSize);
					}
					hasCarriedItemOriginalColliderSize = false;
				}

				RestoreCarriedItemLayer(scene, itemToTrash);

				// Clear carried item
				carriedItemID = -1;
			}
		}

		// Either way, stop here — trash can shouldn't behave like normal tables.
		return;
	}

	playerHolding = (carriedItemID >= 0);
	tableHasItem = table->HasItem();

	// -------------------------------------------------------
	// CASE 1: Player empty-handed, table has an item -> pick up
	// -------------------------------------------------------
	if (!playerHolding && tableHasItem)
	{
		//std::cout << "  [PlayerLogic] CASE1: table has item, player empty -> TakeItem + PickUp\n";
		int itemID = table->TakeItem(scene);
		if (itemID >= 0) {
			PickUp(scene, itemID);
		}
		return;
	}

	// -------------------------------------------------------
	// CASE 2: Player holding something, table is empty -> drop onto table
	// -------------------------------------------------------
	if (playerHolding && !tableHasItem)
	{
		//std::cout << "  [PlayerLogic] CASE2: player holding " << carriedItemID
		//	<< ", table empty -> PlaceItem\n";

		if (table->CanAcceptItem(scene, carriedItemID))
		{
			if (table->PlaceItem(scene, carriedItemID))
			{
				//std::cout << "  [PlayerLogic] CASE2: PlaceItem success, clearing carriedItem\n";
				if (hasCarriedItemOriginalColliderSize) {
					if (GameObject* item = scene.GetGameObjectByID(carriedItemID))
						item->SetColliderSize(carriedItemOriginalColliderSize);
					hasCarriedItemOriginalColliderSize = false;
				}
				// R	estore original layer so it goes back to normal rendering order on the table
				RestoreCarriedItemLayer(scene, carriedItemID);
				carriedItemID = -1;

				// Play put down sound effect (release mode only)
#ifndef _DEBUG
				if (AudioManager* audioMgr = scene.GetAudioManager()) {
					audioMgr->PlaySound("sfx_put_down", audioMgr->GetVfxVolume() * 0.4f, false);
				}
#endif
			}
			else
			{
				//std::cout << "  [PlayerLogic] CASE2: PlaceItem FAILED\n";
			}
		}
		else
		{
			//std::cout << "  [PlayerLogic] CASE2: CanAcceptItem = false\n";
		}
		// --- NEW: if this is a cutting board and it started processing, lock player movement ---
		if (WorkTableLogic* wt = logicMgr.GetLogicForObject<WorkTableLogic>(tableObjectID))
		{
			if (wt->LocksPlayerMovementWhileProcessing() && wt->IsProcessing())
			{
				BeginStationLock(scene, tableObjectID);
			}
		}
		return;
}

	// -------------------------------------------------------
	// CASE 3: Player holding something, table already has an item
	//   -> typical case: table has a Plate, player has an Ingredient
	// -------------------------------------------------------
	if (playerHolding && tableHasItem)
	{
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
			if (plate->TryAddIngredient(*ingr, consumedNow))
			{
				//std::cout << "  [PlayerLogic] CASE3: plate accepted ingredient type\n";

				// --- VISUAL: first ingredient goes onto the plate visually ---
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
						ingredientObj->SetColliderSize(Math::Vector2D(0.f, 0.f));
						ingredientObj->SetMovableByPhysics(false);
					}
				}

				RestoreCarriedItemLayer(scene, carriedItemID);

				//// If at some point TryAddIngredient decides to consume immediately,
				//// we still support that (currently outConsumedNow is always false).
				//bool carriedDespawned = false;
				//if (consumedNow)
				//{
				//	scene.DespawnByID(ingredientObjID);
				//	hasCarriedItemOriginalColliderSize = false;
				//	carriedDespawned = true;
				//}

				// --- Try to assemble a dish once we have enough ingredients ---
				DishType dishType;
				std::vector<IngredientType> consumedTypes;
				if (plate->TryAssembleDish(dishType, consumedTypes))
				{
					//std::cout << "  [PlayerLogic] CASE3: Dish assembled on plate\n";
						// Update visuals based on computed dish type
					plate->ApplyDishVisual(scene);

					// 1) Destroy the first ingredient that was sitting on the plate (if any)
					int firstObjID = plate->GetFirstIngredientObjectID();
					if (firstObjID >= 0) {
						scene.DespawnByID(firstObjID);
						plate->SetFirstIngredientObjectID(-1);
				}

					// 2) Destroy the ingredient we just added (the one we were carrying)
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
			else
			{
				//std::cout << "  [PlayerLogic] CASE3: plate REJECTED ingredient\n";
			}
			return;
	}


		//std::cout << "  [PlayerLogic] CASE3: no (plate,ingredient) combo found\n";
		return;
}

	//std::cout << "  [PlayerLogic] No case matched, doing nothing.\n";
}


void PlayerLogic::UpdateCarriedItemTransform(Scene& scene) {
	if (carriedItemID < 0)
		return;

	GameObject* player = GetOwner(scene);
	if (!player)
		return;

	GameObject* item = scene.GetGameObjectByID(carriedItemID);
	if (!item)
		return;

	const glm::vec2 carry = GetCarryOffsetForFacing();
	glm::vec3 p = player->GetPositionGLM();
	item->SetPosition(glm::vec3(p.x + carry.x,
		p.y + carry.y,
		p.z));

	// Base sort order for carried item
	item->SetRenderSortOrder(0);

	// Layer adjustment so the carried item renders above the player sprite
	ApplyCarryLayer(scene, carriedItemID);

	// If the carried item is a plate with 1 ingredient attached visually, move that ingredient too.
	if (auto* plate = scene.GetLogicManager().GetLogicForObject<PlateLogic>(carriedItemID))
	{
		const int child = plate->GetFirstIngredientObjectID();
		if (child >= 0 && !plate->HasPreparedDish())
		{
			if (GameObject* ingObj = scene.GetGameObjectByID(child))
			{
				// Follow the plate exactly (same position as plate)
				glm::vec3 platePos = item->GetPositionGLM();
				ingObj->SetPosition(platePos);

				// Also adjust layer to match the plate's layer offset (so it renders above the plate)
				const std::string playerLayer = scene.GetObjectLayer(player->GetID());
				const std::string childLayer = GetCarryChildLayerForFacing(playerLayer);
				if (!childLayer.empty()) {
					scene.AssignObjectToLayer(child, childLayer);
				}

				// Ensure stable ordering above the plate
				ingObj->SetRenderSortOrder(1);

				// Make sure it doesn't collide / get pushed
				ingObj->SetColliderSize(Math::Vector2D(0.f, 0.f));
				ingObj->SetMovableByPhysics(false);
			}
		}
	}

	// Debug
	//std::cout << "[PlayerLogic] Updating carried item " << carriedItemID
	//	<< " to follow player at (" << p.x + carryOffset.x << ", "
	//	<< p.y + carryOffset.y << ")\n";
}

void PlayerLogic::BeginStationLock(Scene& scene, int tableID)
{
	movementLocked_ = true;
	lockedTableID_ = tableID;

	// Stop any click-to-move immediately
	hasMoveTarget = false;
	moveMode_ = MoveMode::None;

	// Also clear any existing move target from the movement manager to be safe
	if (GameObject* p = GetOwner(scene)) {
		scene.GetMovementManager().ClearMoveTarget(p->GetID());
		// Check if we should be playing the chopping animation right away (in case the table is already processing)	
		if (ShouldPlayChopAnimation(scene)) {
			EnsureChopAnimation(scene, p);
		}
	}
}

void PlayerLogic::EndStationLock()
{
	movementLocked_ = false;
	lockedTableID_ = -1;
}

void PlayerLogic::UpdateStationLock(Scene& scene)
{
	if (!movementLocked_)
		return;

	// If the table vanished, unlock
	if (lockedTableID_ < 0) {
		EndStationLock();
		return;
	}

	// Only remain locked while the cutting board is actively processing
	WorkTableLogic* wt = scene.GetLogicManager().GetLogicForObject<WorkTableLogic>(lockedTableID_);
	if (!wt || !wt->LocksPlayerMovementWhileProcessing() || !wt->IsProcessing()) {
		EndStationLock();
	}
}

bool PlayerLogic::ShouldPlayChopAnimation(Scene& scene) const
{
	if (lockedTableID_ < 0) {
		return false;
	}

	WorkTableLogic* wt = scene.GetLogicManager().GetLogicForObject<WorkTableLogic>(lockedTableID_);
	if (!wt) {
		return false;
	}

	return wt->LocksPlayerMovementWhileProcessing() && wt->IsProcessing();
}

void PlayerLogic::EnsureChopAnimation(Scene& scene, GameObject* player)
{
	if (!player) {
		return;
	}

	const std::string currentAnimation = scene.GetCurrentAnimationName(player->GetID());
	if (currentAnimation != "CHOP") {
		scene.SetAnimation(player->GetID(), "CHOP");
	}
}

glm::vec2 PlayerLogic::GetCarryOffsetForFacing() const
{
	switch (facingDir) {
	case FacingDir::Front: return carryOffsetFront_;
	case FacingDir::Back:  return carryOffsetBack_;
	case FacingDir::Left:  return carryOffsetLeft_;
	case FacingDir::Right: return carryOffsetRight_;
	default:               return carryOffset;
	}
}

void PlayerLogic::ApplyCarryLayer(Scene& scene, int itemID)
{
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

void PlayerLogic::RestoreCarriedItemLayer(Scene& scene, int itemID)
{
	if (!hasCarriedItemOriginalLayer_) {
		return;
	}

	if (scene.GetGameObjectByID(itemID)) {
		scene.AssignObjectToLayer(itemID, carriedItemOriginalLayer_);
		if (GameObject* item = scene.GetGameObjectByID(itemID)) {
			item->SetRenderSortOrder(0);
		}
	}

	// If this is a plate with an ingredient visually attached, restore the ingredient's layer and order too.
	if (auto* plate = scene.GetLogicManager().GetLogicForObject<PlateLogic>(itemID)) {
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
	bool TryParseLayerNumber(const std::string& layer, int& outValue) {
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

std::string PlayerLogic::GetCarryLayerForFacing(const std::string& baseLayer) const
{
	int value = 0;
	if (!TryParseLayerNumber(baseLayer, value)) {
		return baseLayer;
	}

	const int offset = (facingDir == FacingDir::Front) ? 1 : -2;
	int target = value + offset;
	if (target < 0) {
		target = 0;
	}

	return std::to_string(target);
}

std::string PlayerLogic::GetCarryChildLayerForFacing(const std::string& baseLayer) const
{
	int value = 0;
	if (!TryParseLayerNumber(baseLayer, value)) {
		return baseLayer;
	}

	const int offset = (facingDir == FacingDir::Front) ? 2 : -1;
	int target = value + offset;
	if (target < 0) {
		target = 0;
	}

	return std::to_string(target);
}

std::string PlayerLogic::GetChildLayerAbove(const std::string& baseLayer) const
{
	int value = 0;
	if (!TryParseLayerNumber(baseLayer, value)) {
		return baseLayer;
	}

	return std::to_string(value + 1);
}
