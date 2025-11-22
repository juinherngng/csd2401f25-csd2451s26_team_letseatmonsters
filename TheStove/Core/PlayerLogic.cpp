// PlayerLogic.cpp
#include "PlayerLogic.hpp"
#include "../Graphics/SceneManager.hpp"
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

	pos.x += dir.x * step;
	pos.y += dir.y * step;

	player->SetPosition(glm::vec3(pos.x, pos.y, pos3.z));
	scene.ClampToWalkArea(player);

	// Update sprite based on movement direction
	UpdateSprite(scene, player, dir);
}

// Unity: OnArrived()
// For now it�s a stub; later you can branch by what we clicked (tables, spawners, etc.)
void PlayerLogic::OnArrived(Scene& scene) {
	(void)scene;
	// Example debug:
	 std::cout << "[PlayerLogic] Arrived at destination\n";
}

// Unity: PickUp(GameObject item) � here by engine ID
void PlayerLogic::PickUp(Scene& scene, int itemID) {
	GameObject* item = scene.GetGameObjectByID(itemID);
	GameObject* player = GetOwner(scene);
	if (!item || !player)
		return;

	carriedItemID = itemID;

	// For now, just snap the item near the player.
	// Later you can add proper �holdingPoint� + offsets like Unity.
	glm::vec3 p = player->GetPositionGLM();
	item->SetPosition(glm::vec3(p.x, p.y - 32.f, p.z)); // crude �front� offset
}

// Unity: Drop(Vector3 dropPos) � here: drop slightly in front of player
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
	item->SetPosition(glm::vec3(p.x + 16.f, p.y, p.z)); // simple �in front� drop
	carriedItemID = -1;
}

void PlayerLogic::Update(float dt, Scene& scene, InputManager& input) {
	GameObject* player = GetOwner(scene);
	if (!player) return;

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

		pos3.x += inputDir.x * speed * dt;
		pos3.y += inputDir.y * speed * dt;

		player->SetPosition(pos3);
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

	// Debug key to prove script is running
	if (input.IsKeyJustPressed(GLFW_KEY_P)) {
		std::cout << "[PlayerLogic] P pressed\n";
	}
}
