/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogic.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Implements player behaviour including WASD movement, click-to-move steering,
					collision-trimmed navigation, arrival handling, sprite direction switching, and
					simple item interaction logic. Integrates with Scene, InputManager, and physics
					step mode.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

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
	}

	moveTarget = dest;
	hasMoveTarget = true;
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

	// Desired movement for this frame
	glm::vec2 desiredDelta(dir.x * step, dir.y * step);

	// Trim against static world (outer frame + wood + gate)
	glm::vec2 allowedDelta = scene.ResolveWorldStep(player, desiredDelta);

	pos.x += allowedDelta.x;
	pos.y += allowedDelta.y;

	player->SetPosition(glm::vec3(pos.x, pos.y, pos3.z));
	scene.ClampToWalkArea(player);
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

	if (input.IsKeyPressed(GLFW_KEY_A)) inputDir.x -= 1.f;
	if (input.IsKeyPressed(GLFW_KEY_D)) inputDir.x += 1.f;
	if (input.IsKeyPressed(GLFW_KEY_W)) inputDir.y -= 1.f;
	if (input.IsKeyPressed(GLFW_KEY_S)) inputDir.y += 1.f;

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

	HandleClickInput(scene, input);

	UpdateMovement(dt, scene);

	// Debug key to prove script is running
	if (input.IsKeyJustPressed(GLFW_KEY_P)) {
		std::cout << "[PlayerLogic] P pressed\n";
	}
}
