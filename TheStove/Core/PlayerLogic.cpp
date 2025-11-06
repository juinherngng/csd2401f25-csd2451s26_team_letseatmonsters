/*
----------------------------------------------------------------------------------------------------
FILE NAME:			PlayerLogic.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Vu Phan Hung, phanhung.vu@digipen.edu

DESCRIPTION:		Implements player control logic, including movement, sprite updates,
					mouse click handling, item pickup/drop, and scene clamping behavior.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#include "PlayerLogic.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Graphics/DebugRenderer.hpp"
#include "../Core/InputManager.hpp"
#include "../Core/InputControls.hpp"
#include "../Core/DebugUI.hpp"        // for DebuggerApp
#include <iostream>

void PlayerLogic::Start(Scene& scene)
{
	(void)scene;
	hasMoveTarget = false;
	carriedItemID = -1;
	facingDir = FacingDir::Front;
}

// Decide and apply sprite based on movement direction
void PlayerLogic::UpdateSprite(Scene& scene, GameObject* player, const glm::vec2& moveDirRaw) {
	(void)scene;
	if (!player) return;

	glm::vec2 direction = moveDirRaw;

	// Small dead zone to avoid jitter when very close / tiny input
	if (glm::length(direction) <= 0.001f) {
		return;
	}

	float absX = std::abs(direction.x);
	float absY = std::abs(direction.y);

	if (absX > absY) {
		// Horizontal dominant
		if (direction.x > 0.0f) {
			player->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_right.png", "../assets/mc_sprite_right.png"));
		}
		else {
			player->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_left.png", "../assets/mc_sprite_left.png"));
		}
	}
	else {
		// Vertical dominant
		if (direction.y > 0.0f) {
			player->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_front.png", "../assets/mc_sprite_front.png"));
		}
		else {
			player->SetTexture(ResourceManager::Instance().LoadTexture(
				"../assets/mc_sprite_back.png", "../assets/mc_sprite_back.png"));
		}
	}
}

// Unity: Move(Vector3 dest)
void PlayerLogic::MoveTo(Scene& scene, const glm::vec2& dest) {
	GameObject* player = GetOwner(scene);

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
	// Only once per click
	if (!input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT))
		return;

	GameObject* player = GetOwner(scene);
	if (!player) return;

	glm::vec2 mouseWorld{};
	// Use the same helper the old PlayerController used
	if (!scene.GetGraphicsEngine().GetMouseWorldInScene(mouseWorld)) {
		// mouse not over scene viewport, do nothing
		std::cout << "not over scene";
		return;
	}

	std::cout << "[PlayerLogic] Click world = (" << mouseWorld.x << ", " << mouseWorld.y << ")\n";

	MoveTo(scene, mouseWorld);   // our own kinematic MoveTo
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

	Math::Vector2D desiredDelta(dir.x * step, dir.y * step);
	Math::Vector2D allowedDelta =
		scene.GetCollisionManager().GetCollisionWorld().resolve(box, desiredDelta);

	// If we can't move at all (hit a wall and are stuck), cancel the target
	const float allowedLenSq = allowedDelta.x * allowedDelta.x +
		allowedDelta.y * allowedDelta.y;
	if (allowedLenSq < 0.0001f) {
		hasMoveTarget = false;
		if (GameObject* p = GetOwner(scene)) {
			scene.GetMovementManager().ClearMoveTarget(p->GetID());
		}
		return;
	}

	// Apply allowed movement
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
// For now it’s a stub; later you can branch by what we clicked (tables, spawners, etc.)
void PlayerLogic::OnArrived(Scene& scene) {
	(void)scene;
	// Example debug:
	 std::cout << "[PlayerLogic] Arrived at destination\n";
}

// Unity: PickUp(GameObject item) – here by engine ID
void PlayerLogic::PickUp(Scene& scene, int itemID) {
	GameObject* item = scene.GetGameObjectByID(itemID);
	GameObject* player = GetOwner(scene);
	if (!item || !player)
		return;

	carriedItemID = itemID;

	// For now, just snap the item near the player.
	// Later you can add proper “holdingPoint” + offsets like Unity.
	glm::vec3 p = player->GetPositionGLM();
	item->SetPosition(glm::vec3(p.x, p.y - 32.f, p.z)); // crude “front” offset
}

// Unity: Drop(Vector3 dropPos) – here: drop slightly in front of player
void PlayerLogic::Drop(Scene& scene) {
	if (carriedItemID < 0)
		return;

	GameObject* player = GetOwner(scene);
	GameObject* item = scene.GetGameObjectByID(carriedItemID);
	if (!player || !item) {
		carriedItemID = -1;
		return;
	}

	glm::vec3 p = player->GetPositionGLM();
	item->SetPosition(glm::vec3(p.x + 16.f, p.y, p.z)); // simple “in front” drop
	carriedItemID = -1;
}

void PlayerLogic::Update(float dt, Scene& scene, InputManager& input) {
	GameObject* player = GetOwner(scene);
	if (!player) return;

	glm::vec3 pos3 = player->GetPositionGLM();
	glm::vec2 inputDir(0.f, 0.f);
	float speed = 200.0f;

	if (input.IsKeyPressed(GLFW_KEY_A)) inputDir.x -= 1.f;
	if (input.IsKeyPressed(GLFW_KEY_D)) inputDir.x += 1.f;
	if (input.IsKeyPressed(GLFW_KEY_W)) inputDir.y -= 1.f;
	if (input.IsKeyPressed(GLFW_KEY_S)) inputDir.y += 1.f;

	if (inputDir.x != 0.f || inputDir.y != 0.f) {
		hasMoveTarget = false;

		scene.GetMovementManager().ClearMoveTarget(player->GetID());

		float len = std::sqrt(inputDir.x * inputDir.x + inputDir.y * inputDir.y);
		if (len > 0.0001f) {
			inputDir.x /= len;
			inputDir.y /= len;
		}

		glm::vec2 desiredStep = inputDir * moveSpeed * dt;

		// Build collider at current position in M-space
		Math::Vector3D posM(pos3.x, pos3.y, pos3.z);
		const collision::AABB box = physics::MakeColliderBox(player, posM);

		// Ask collision world to resolve movement against all static walls
		collision::World& world = scene.GetCollisionWorld();
		Math::Vector2D desiredMove(desiredStep.x, desiredStep.y);
		Math::Vector2D allowedMove = world.resolve(box, desiredMove);

		// Apply allowed movement
		posM.x += allowedMove.x;
		posM.y += allowedMove.y;

		glm::vec3 newPos(posM.x, posM.y, posM.z);
		player->SetPosition(newPos);

		// Still clamp to walk area / gate as a final safeguard
		scene.ClampToWalkArea(player);

		// Update sprite based on keyboard movement
		UpdateSprite(scene, player, inputDir);
	}


	HandleClickInput(scene, input);


	UpdateMovement(dt, scene);

	// Debug key to prove script is running
	if (input.IsKeyJustPressed(GLFW_KEY_P)) {
		std::cout << "[PlayerLogic] P pressed\n";
	}
}
