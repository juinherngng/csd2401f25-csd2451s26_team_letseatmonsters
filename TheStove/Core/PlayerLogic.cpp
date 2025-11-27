/*
----------------------------------------------------------------------------------------------------
FILE NAME:			PlayerLogic.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Vu Phan Hung, phanhung.vu@digipen.edu

DESCRIPTION:		Implements player control logic, including movement, sprite updates,
					mouse click handling, item pickup/drop, and scene clamping behavior.

		All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#include "PlayerLogic.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Core/InputManager.hpp"
#include "../Core/InputControls.hpp"
#include "TableLogic.hpp"
#include "WorkTableLogic.hpp"
#include "CustomerTableLogic.hpp"
#include <iostream>

#include "../Core/DebugUI.hpp"
#include "../Core/InputControls.hpp"
#include "../Core/InputManager.hpp"
#include "../Graphics/SceneManager.hpp"
#include "PlayerLogic.hpp"

void PlayerLogic::Start(Scene& scene) {
	(void)scene;
	hasMoveTarget = false;
	carriedItemID = -1;
	pendingTableID = -1;
	facingDir = FacingDir::Front;

	GameObject* owner = GetOwner(scene);
	std::cout << "[PlayerLogic] Start on object ID "
		<< (owner ? owner->GetID() : -1) << "\n";
}

// Decide and apply sprite based on movement direction
void PlayerLogic::UpdateSprite(Scene& scene, GameObject* player, const glm::vec2& moveDirRaw) {
	(void)scene;
	if (!player) return;

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

// Unity: Move(Vector3 dest)
void PlayerLogic::MoveTo(Scene& scene, const glm::vec2& dest) {
	GameObject* player = GetOwner(scene);

	std::cout << "[PlayerLogic] MoveTo(" << dest.x << ", " << dest.y << ")\n";

	if (player) {
		glm::vec3 pos3 = player->GetPositionGLM();
		glm::vec2 pos(pos3.x, pos3.y);
		glm::vec2 delta = dest - pos;

		// Update sprite immediately based on click direction
		UpdateSprite(scene, player, delta);

		scene.GetMovementManager().SetMoveTarget(player->GetID(), dest);
	}

	moveTarget = dest;
	hasMoveTarget = true;

	//auto& movement = scene.GetMovementManager(); // hypothetical accessor
	//movement.SetMoveTarget(player->GetID(), dest);
}

void PlayerLogic::HandleClickInput(Scene& scene, InputManager& input) {
	PlayerController& controller = scene.GetPlayerController();

	// Only once per click (left mouse)
	if (!controller.IsClickToMoveJustPressed())
		return;

	GameObject* player = GetOwner(scene);
	if (!player) return;

	glm::vec2 mouseWorld = controller.GetClickWorld();
	std::cout << "[PlayerLogic] Click world = (" << mouseWorld.x << ", " << mouseWorld.y << ")\n";

	// ----------------------------------------------------------
	// 1) Raycast: check if click is on any table-like object
	// ----------------------------------------------------------
	LogicManager& logicMgr = scene.GetLogicManager();

	int   clickedTableID = -1;
	float bestDistSq = std::numeric_limits<float>::max();
	TableLogic* clickedTableLogic = nullptr;

	for (GameObject* obj : scene.GetAllObjectsRaw()) {
		if (!obj) continue;

		int id = obj->GetID();

		// Debug: does this object have any TableLogic?
		TableLogic* tableLogic2 = logicMgr.GetLogicForObject<TableLogic>(id);
		std::cout << "[ClickDebug] id=" << id
			<< " hasTableLogic=" << (tableLogic2 ? "yes" : "no")
			<< "\n";

		// Any table (normal / work / customer / ingredient box) derives from TableLogic
		TableLogic* tableLogic = logicMgr.GetLogicForObject<TableLogic>(id);
		if (!tableLogic) {
			continue; // not a table-like object
		}

		// --- Use collider as click area ---
		auto colSize = obj->GetColliderSize();     // (width, height)
		auto colOffset = obj->GetColliderOffset();   // (offset x, offset y)

		glm::vec3 objPos = obj->GetPositionGLM();
		glm::vec2 center(objPos.x + colOffset.x, objPos.y + colOffset.y);

		float halfW = colSize.x * 0.5f;
		float halfH = colSize.y * 0.5f;

		bool inside =
			(mouseWorld.x >= center.x - halfW && mouseWorld.x <= center.x + halfW) &&
			(mouseWorld.y >= center.y - halfH && mouseWorld.y <= center.y + halfH);

		if (!inside)
			continue;

		float dx = mouseWorld.x - center.x;
		float dy = mouseWorld.y - center.y;
		float distSq = dx * dx + dy * dy;

		if (distSq < bestDistSq) {
			bestDistSq = distSq;
			clickedTableID = id;
			clickedTableLogic = tableLogic; // remember which table logic we hit
		}
	}

	// ----------------------------------------------------------
	// 2) If we clicked a table, move to its approach point
	// ----------------------------------------------------------
	if (clickedTableID >= 0 && clickedTableLogic) {
		std::cout << "[PlayerLogic] Click hit table id " << clickedTableID << "\n";
		pendingTableID = clickedTableID;

		// Player current world position
		glm::vec3 playerPos3 = player->GetPositionGLM();
		Math::Vector2D from(playerPos3.x, playerPos3.y);

		// Ask the table for the best approach point, in WORLD space
		Math::Vector2D approach = clickedTableLogic->GetClosestApproachPoint(scene, from);

		// Debug: where are we actually going?
		std::cout << "[PlayerLogic] Moving to approach point for table " << clickedTableID
			<< " at (" << approach.x << ", " << approach.y << ")\n";

		// Convert to glm::vec2 for MoveTo
		glm::vec2 target(approach.x, approach.y);
		MoveTo(scene, target);
	}
	else {
		// No table hit: just move to the clicked position as before
		pendingTableID = -1;
		std::cout << "[PlayerLogic] No table clicked, moving to raw mouse ("
			<< mouseWorld.x << ", " << mouseWorld.y << ")\n";
		MoveTo(scene, mouseWorld);
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

	glm::vec2 dir = moveTarget - pos;
	const float distSq = dir.x * dir.x + dir.y * dir.y;

	const float arriveRadius = 4.0f;      // pixels (tweak)
	const float arriveRadiusSq = arriveRadius * arriveRadius;

	// ReachedDestination()
	if (distSq <= arriveRadiusSq) {
		hasMoveTarget = false;

		if (GameObject* player = GetOwner(scene)) {
			scene.GetMovementManager().ClearMoveTarget(player->GetID());
		}

		OnArrived(scene);
		return;
	}

	const float dist = std::sqrt(distSq);
	if (dist > 0.0001f) {
		dir.x /= dist;
		dir.y /= dist;
	}

	float step = moveSpeed * dt;
	if (step > dist)
		step = dist;

	// --- NEW: ask CollisionWorld how much of this step is allowed ---
	const auto size = player->GetColliderSize();
	const auto offset = player->GetColliderOffset();

	// Build AABB for current position (center = pos + offset)
	Math::Vector3D center(pos3.x + offset.x, pos3.y + offset.y, pos3.z);
	Math::Vector3D scale(size.x, size.y, 1.0f);

	collision::AABB box = collision::World::makeAABBFromCenter(center, scale);
		// Apply allowed movement
		// Desired movement for this frame
		glm::vec2 desiredDelta(dir.x * step, dir.y * step);
	
		// Trim against static world (outer frame + wood + gate)
		glm::vec2 allowedDelta = scene.ResolveWorldStep(player, desiredDelta);

	// If we can't move at all (hit a wall and are stuck), cancel the target
	const float allowedLenSq = allowedDelta.x * allowedDelta.x +
		allowedDelta.y * allowedDelta.y;
	if (allowedLenSq < 0.0001f) {
		std::cout << "[PlayerLogic] MoveTo cancelled by collision, clearing target\n";
		hasMoveTarget = false;
		if (GameObject* p = GetOwner(scene)) {
			scene.GetMovementManager().ClearMoveTarget(p->GetID());
		}
		return;
	}


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
// For now it�s a stub; later you can branch by what we clicked (tables, spawners, etc.)
void PlayerLogic::OnArrived(Scene& scene) {
	(void)scene;
	// Example debug:
	std::cout << "[PlayerLogic] Arrived at destination\n";

	if (pendingTableID < 0)
		return;

	GameObject* player = GetOwner(scene);
	GameObject* tableObj = scene.GetGameObjectByID(pendingTableID);
	if (!player || !tableObj) {
		pendingTableID = -1;
		return;
	}

	glm::vec3 pPos = player->GetPositionGLM();
	glm::vec3 tPos = tableObj->GetPositionGLM();

	float dx = pPos.x - tPos.x;
	float dy = pPos.y - tPos.y;
	float distSq = dx * dx + dy * dy;

	// Interaction radius (tweak to taste)
	constexpr float kInteractRadius = 120.0f;
	if (distSq <= kInteractRadius * kInteractRadius) {
		std::cout << "[PlayerLogic] Close enough to table " << pendingTableID
			<< ", performing interaction\n";
		InteractWithTable(scene, pendingTableID);
	}
	else {
		std::cout << "[PlayerLogic] Arrived near click, but too far from table (dist="
			<< std::sqrt(distSq) << ")\n";
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

	std::cout << "[PlayerLogic] PickUp item " << itemID << "\n";

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
		std::cout << "[PlayerLogic] Drop failed, invalid item or player. Clearing carriedItemID.\n";
		carriedItemID = -1;
		return;
	}

	std::cout << "[PlayerLogic] Drop item " << carriedItemID << "\n";

	if (hasCarriedItemOriginalColliderSize) {
		item->SetColliderSize(carriedItemOriginalColliderSize);
		hasCarriedItemOriginalColliderSize = false;
	}

	glm::vec3 p = player->GetPositionGLM();
	item->SetPosition(glm::vec3(p.x + 16.f, p.y, p.z)); // simple �in front� drop
	carriedItemID = -1;
}

void PlayerLogic::HandleScaleInput(GameObject* player, InputManager& input, float dt)
{
	(void)dt;
	if (!player) return;

	glm::vec3 scale = player->GetScaleGLM();

	if (input.IsKeyPressed(GLFW_KEY_UP)) {
		scale *= 1.01f;
		scale = glm::min(scale, glm::vec3(500.0f));
		player->SetScale(scale);
	}

	if (input.IsKeyPressed(GLFW_KEY_DOWN)) {
		scale *= 0.99f;
		scale = glm::max(scale, glm::vec3(50.0f));
		player->SetScale(scale);
	}
}

void PlayerLogic::HandleRotationInput(GameObject* player, InputManager& input, float dt)
{
	if (!player) return;

	const float kRotationSpeed = 10.0f; // degrees per second

	if (input.IsKeyPressed(GLFW_KEY_RIGHT)) {
		rotation_ += kRotationSpeed * dt;
	}
	if (input.IsKeyPressed(GLFW_KEY_LEFT)) {
		rotation_ -= kRotationSpeed * dt;
	}

	// Normalize to [0, 360)
	while (rotation_ >= 360.0f) rotation_ -= 360.0f;
	while (rotation_ < 0.0f)   rotation_ += 360.0f;

	player->SetRotation(rotation_, glm::vec3(0, 0, 1));
}


void PlayerLogic::Update(float dt, Scene& scene, InputManager& input) {
	GameObject* player = GetOwner(scene);
	if (!player) return;

	const float physicsDt = scene.GetLastPhysicsDt();
	const physics::StepController& step = scene.GetStepController();
	const bool stepMode = step.enabled;

	if (stepMode && physicsDt <= 0.0f) {
		// Optional: still allow click selection while frozen
		HandleClickInput(scene, input);

		// Debug: prove we still see the key
		if (input.IsKeyJustPressed(GLFW_KEY_P)) {
			std::cout << "[PlayerLogic] P pressed (step mode, frozen)\n";
		}

		return; // skip movement while paused
	}

	glm::vec3 pos3 = player->GetPositionGLM();
	glm::vec2 inputDir(0.f, 0.f);
	float speed = 200.0f;

	// Get keyboard input
	if (input.IsKeyPressed(GLFW_KEY_A)) inputDir.x -= 1.f;
	if (input.IsKeyPressed(GLFW_KEY_D)) inputDir.x += 1.f;
	if (input.IsKeyPressed(GLFW_KEY_W)) inputDir.y -= 1.f;
	if (input.IsKeyPressed(GLFW_KEY_S)) inputDir.y += 1.f;

	// Main keyboard movement
	if (inputDir.x != 0.f || inputDir.y != 0.f) {
		hasMoveTarget = false;

		float len = std::sqrt(inputDir.x * inputDir.x + inputDir.y * inputDir.y);
		if (len > 0.0001f) {
			inputDir.x /= len;
			inputDir.y /= len;
		}

		// Desired movement this frame
		glm::vec2 desiredDelta(inputDir.x * speed * dt,
							   inputDir.y * speed * dt);

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

	HandleClickInput(scene, input);

	UpdateMovement(dt, scene);

	UpdateCarriedItemTransform(scene);

	// Debug key to prove script is running
	if (input.IsKeyJustPressed(GLFW_KEY_P)) {
		std::cout << "[PlayerLogic] P pressed\n";
	}
}

void PlayerLogic::InteractWithTable(Scene& scene, int tableObjectID)
{
	std::cout << "[PlayerLogic] InteractWithTable tableID=" << tableObjectID << "\n";

	GameObject* player = GetOwner(scene);
	if (!player)
		return;

	// Get table logic for the clicked/selected GameObject
	LogicManager& logicMgr = scene.GetLogicManager();
	TableLogic* table = logicMgr.GetLogicForObject<TableLogic>(tableObjectID);
	if (!table)
	{
		std::cout << "[PlayerLogic] InteractWithTable: no TableLogic found on that object\n";
		return;
	}

	bool playerHolding = (carriedItemID >= 0);
	bool tableHasItem = table->HasItem();

	std::cout << "  [PlayerLogic] state: playerHolding=" << playerHolding
		<< " carriedItemID=" << carriedItemID
		<< " tableHasItem=" << tableHasItem
		<< " tableHeldItemID=" << (tableHasItem ? table->GetHeldItemID() : -1)
		<< "\n";

	// --- Special case: Ingredient box ---
	if (IngredientBoxLogic* box = logicMgr.GetLogicForObject<IngredientBoxLogic>(tableObjectID))
	{
		std::cout << "  [PlayerLogic] This table is an IngredientBox\n";

		if (carriedItemID >= 0)
		{
			std::cout << "  [PlayerLogic] Already holding item " << carriedItemID
				<< ", ignoring ingredient box\n";
			return;
		}

		int newItemID = box->SpawnIngredient(scene);
		if (newItemID >= 0)
		{
			PickUp(scene, newItemID);  // auto-pickup
		}
		return; // Do not fall through to normal table logic
	}

	playerHolding = (carriedItemID >= 0);
	tableHasItem = table->HasItem();

	// -------------------------------------------------------
	// CASE 1: Player empty-handed, table has an item -> pick up
	// -------------------------------------------------------
	if (!playerHolding && tableHasItem)
	{
		std::cout << "  [PlayerLogic] CASE1: table has item, player empty -> TakeItem + PickUp\n";
		int itemID = table->TakeItem(scene);
		if (itemID >= 0)
		{
			PickUp(scene, itemID);
		}
		return;
	}

	// -------------------------------------------------------
	// CASE 2: Player holding something, table is empty -> drop onto table
	// -------------------------------------------------------
	if (playerHolding && !tableHasItem)
	{
		std::cout << "  [PlayerLogic] CASE2: player holding " << carriedItemID
			<< ", table empty -> PlaceItem\n";

		if (table->CanAcceptItem(scene, carriedItemID))
		{
			if (table->PlaceItem(scene, carriedItemID))
			{
				std::cout << "  [PlayerLogic] CASE2: PlaceItem success, clearing carriedItem\n";
				carriedItemID = -1;
			}
			else
			{
				std::cout << "  [PlayerLogic] CASE2: PlaceItem FAILED\n";
			}
		}
		else
		{
			std::cout << "  [PlayerLogic] CASE2: CanAcceptItem = false\n";
		}
		return;
	}

	// -------------------------------------------------------
	// CASE 3: Player holding something, table already has an item
	//   -> typical case: table has a Plate, player has an Ingredient
	// -------------------------------------------------------
	if (playerHolding && tableHasItem)
	{
		std::cout << "  [PlayerLogic] CASE3: both player & table have items -> try plate+ingredient combo\n";

		const int tableItemID = table->GetHeldItemID();

		PlateLogic* plate = logicMgr.GetLogicForObject<PlateLogic>(tableItemID);
		IngredientLogic* ingr = logicMgr.GetLogicForObject<IngredientLogic>(carriedItemID);

		if (plate && ingr)
		{
			bool consumedNow = false;
			if (plate->TryAddIngredient(*ingr, consumedNow))
			{
				carriedItemID = -1;
				std::cout << "  [PlayerLogic] CASE3: plate accepted ingredient; carriedItem cleared\n";
			}
			else
			{
				std::cout << "  [PlayerLogic] CASE3: plate REJECTED ingredient\n";
			}
			return;
		}

		std::cout << "  [PlayerLogic] CASE3: no (plate,ingredient) combo found\n";
		return;
	}

	std::cout << "  [PlayerLogic] No case matched, doing nothing.\n";
}


void PlayerLogic::UpdateCarriedItemTransform(Scene& scene)
{
	if (carriedItemID < 0)
		return;

	GameObject* player = GetOwner(scene);
	if (!player)
		return;

	GameObject* item = scene.GetGameObjectByID(carriedItemID);
	if (!item)
		return;

	glm::vec3 p = player->GetPositionGLM();
	item->SetPosition(glm::vec3(p.x + carryOffset.x,
		p.y + carryOffset.y,
		p.z));

	////Optional debug
	//std::cout << "[PlayerLogic] Updating carried item " << carriedItemID
	//	<< " to follow player at (" << p.x + carryOffset.x << ", "
	//	<< p.y + carryOffset.y << ")\n";
}


