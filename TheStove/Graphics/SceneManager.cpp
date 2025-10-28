/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Implements the Scene class, handling object spawning,
					animation, collisions, and per-frame updates.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "SceneManager.hpp"
#include <iostream>
#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>

 // Level constants
static constexpr float kWorldW = 1200.0f;
static constexpr float kWorldH = 800.0f;

// Walkable inner rectangle (match to background art)
static constexpr float kWalkL = 150.0f;  // left
static constexpr float kWalkR = 1100.0f; // right
static constexpr float kWalkT = 80.0f;   // top
static constexpr float kWalkB = 733.0f;  // bottom

// Thickness of our blocking bars (thin = precise, easy to tune)
static constexpr float kEdgeThick = 3.0f;

// Wooden divider (vertical split)
static constexpr float kWoodX0 = 562.0f;
static constexpr float kWoodX1 = 590.0f;
static constexpr float kWoodTopMinY = 100.0f;
static constexpr float kWoodTopMaxY = 300.0f;
static constexpr float kWoodGapMinY = 300.0f;
static constexpr float kWoodGapMaxY = 500.0f;
static constexpr float kWoodBotMinY = 500.0f;
static constexpr float kWoodBotMaxY = 700.0f;

// End-of-stage vertical gate
static constexpr float kEndVX0 = 1100.0f;
static constexpr float kEndVX1 = 1200.0f;
static constexpr float kEndVTopMinY = 100.0f;
static constexpr float kEndVTopMaxY = 300.0f;
static constexpr float kEndVGapMinY = 300.0f;
static constexpr float kEndVGapMaxY = 500.0f;
static constexpr float kEndVBotMinY = 500.0f;
static constexpr float kEndVBotMaxY = 700.0f;

namespace {
	inline Math::Vector2D toM(const glm::vec2& v) { return Math::Vector2D(v.x, v.y); }
	inline Math::Vector3D toM(const glm::vec3& v) { return Math::Vector3D(v.x, v.y, v.z); }
	inline glm::vec2 toG(const Math::Vector2D& v) { return glm::vec2(v.x, v.y); }
	inline glm::vec3 toG(const Math::Vector3D& v) { return glm::vec3(v.x, v.y, v.z); }
}

// Quick hit-test for a point against a center-anchored AABB.
static inline bool PointInsideCenterAABB(glm::vec2 p, glm::vec3 center, glm::vec3 scale) {
	const float hx = scale.x * 0.5f;
	const float hy = scale.y * 0.5f;
	return (p.x >= center.x - hx && p.x <= center.x + hx &&
		p.y >= center.y - hy && p.y <= center.y + hy);
}

// Scene lifecycle
Scene::Scene(GraphicsEngine& engine) : graphicsEngine(engine) {}

void Scene::LoadScene(const std::string& sceneName) {
	(void)sceneName; // Suppress unused parameter warning
	LoadTest();
}

// Spawners/background/lookup
GameObject* Scene::SpawnTriangle(const glm::vec3 position, const glm::vec3 scale, float rotation) {
	// Load resources
	Mesh* mesh = ResourceManager::Instance().GetMesh("triangle");
	Shader* shader = ResourceManager::Instance().GetShader("basic");
	if (!mesh || !shader) { std::cerr << "Missing resources for triangle\n"; return nullptr; }
	auto obj = std::make_unique<GameObject>(mesh, shader);
	obj->SetID(nextID++);
	obj->SetPosition(position);
	obj->SetScale(scale);
	obj->SetRotation(glm::radians(rotation), glm::vec3(0, 0, 1));
	GameObject* raw = obj.get();
	sceneObjects.push_back(std::move(obj));
	return raw;
}

GameObject* Scene::SpawnStaticSprite(const std::string& texturePath, const glm::vec3 position, const glm::vec2 size) {
	// Load resources
	std::string textureName = "sprite_" + texturePath;
	Texture* spriteTex = ResourceManager::Instance().LoadTexture(textureName, texturePath);
	Mesh* mesh = ResourceManager::Instance().GetMesh("sprite");
	Shader* shader = ResourceManager::Instance().GetShader("staticsprite");
	if (!spriteTex || !mesh || !shader) { std::cerr << "Missing resources for static sprite\n"; return nullptr; }

	auto obj = std::make_unique<GameObject>(mesh, shader);
	obj->SetID(nextID++);
	obj->SetPosition(position);
	obj->SetScale(glm::vec3(size.x, size.y, 1.0f));
	obj->SetTexture(spriteTex);
	GameObject* raw = obj.get();
	sceneObjects.push_back(std::move(obj));
	return raw;
}

GameObject* Scene::SpawnAnimatedSprite(const std::string& texturePath, const glm::vec3 position, const glm::vec2 size,
	const std::vector<glm::vec4> frames, float frameDuration, bool loop) {
	// Load resources
	std::string textureName = "sprite_" + texturePath;
	Texture* spriteTex = ResourceManager::Instance().LoadTexture(textureName, texturePath);
	Mesh* mesh = ResourceManager::Instance().GetMesh("sprite");
	Shader* shader = ResourceManager::Instance().GetShader("animatedsprite");
	if (!spriteTex || !mesh || !shader) { std::cerr << "Missing resources for animated sprite\n"; return nullptr; }

	auto obj = std::make_unique<GameObject>(mesh, shader);
	obj->SetID(nextID++);
	obj->SetPosition(position);
	obj->SetScale(glm::vec3(size.x, size.y, 1.0f));
	obj->SetTexture(spriteTex);
	GameObject* raw = obj.get();
	sceneObjects.push_back(std::move(obj));

	Animator2D animator;
	animator.SetFrames(frames, frameDuration, loop);
	animator.Play();
	animators[raw->GetID()] = animator;
	return raw;
}

void Scene::SetSceneBackground(const std::string& texturePath) {
	graphicsEngine.SetBackground(texturePath);
}

GameObject* Scene::GetGameObjectByID(int targetID) {
	for (const auto& obj : sceneObjects) {
		if (obj->GetID() == targetID)
			return obj.get();
	}
	return nullptr; // Not found
}

void Scene::DespawnByID(int targetID) {
	// Remove anim state first
	animators.erase(targetID);
	objectAnimations.erase(targetID);
	currentAnimation.erase(targetID);

	sceneObjects.erase(
		std::remove_if(sceneObjects.begin(), sceneObjects.end(),
			[targetID](const std::unique_ptr<GameObject>& up) {
				return up && up->GetID() == targetID;
			}),
		sceneObjects.end()
	);
}

void Scene::CollectRenderablePointers(std::vector<GameObject*>& out) const {
	out.clear();
	out.reserve(sceneObjects.size());
	for (const auto& up : sceneObjects) {
		if (up) out.push_back(up.get());
	}
}

// World build
void Scene::BuildLevelColliders() {
	collision::WalkArea walk{
		kWalkL, kWalkR,
		kWalkT, kWalkB,
		kEdgeThick
	};

	collision::WoodVertical wood{
		kWoodX0, kWoodX1,
		kWoodTopMinY, kWoodTopMaxY,
		kWoodGapMinY, kWoodGapMaxY,
		kWoodBotMinY, kWoodBotMaxY
	};

	collision::StageEndGateVertical gate{
		kEndVX0, kEndVX1,
		kEndVTopMinY, kEndVTopMaxY,
		kEndVGapMinY, kEndVGapMaxY,
		kEndVBotMinY, kEndVBotMaxY
	};

	mCollision.build(walk, wood, gate);
}

// Utility function to generate UV frames for a sprite sheet
std::vector<glm::vec4> GenerateFrames(int startFrame, int frameCount, int totalCols, float frameWidth, float frameHeight) {
	(void)totalCols; // Suppress unused parameter warning

	std::vector<glm::vec4> frames;
	for (int i = 0; i < frameCount; ++i) {
		int col = startFrame + i;
		float offsetX = col * frameWidth;
		float offsetY = 1.0f - frameHeight; // single row, so just - frameHeight for Y offset
		frames.emplace_back(offsetX, offsetY, frameWidth, frameHeight);
	}
	return frames;
}

// Test scene setup
void Scene::LoadTest() {
	// Set background image
	SetSceneBackground("../assets/Background.png");

	// Choose visual size once as variables
	const glm::vec2 kVisualSizePx = { 128.0f, 128.0f };
	const glm::vec3 kSpawnPos = { 400.0f, 400.0f, 0.0f };

	// Define collider = 64x128 with no offset (relative to visual)
	const glm::vec2 colliderSizePx = { 64.0f, 128.0f };
	const glm::vec2 colliderOffsetPx{ 0.0f, 0.0f };
	const float kLaneX = 1000.0f;

	// Player
	if (GameObject* player = SpawnStaticSprite("../assets/mc_sprite_front.png", kSpawnPos, kVisualSizePx)) {
		spriteID = player->GetID();
		std::cout << "Spawned player sprite with ID: " << spriteID << std::endl;

		spriteScales[spriteID] = glm::vec3(kVisualSizePx, 1.0f);
		spriteRotations[spriteID] = 0.0f;
		spritePositions[spriteID] = kSpawnPos;

		player->SetScale(glm::vec3(kVisualSizePx, 1.0f));
		player->SetColliderSize(Math::Vector2D(colliderSizePx.x, colliderSizePx.y));
		player->SetColliderOffset(Math::Vector2D(colliderOffsetPx.x, colliderOffsetPx.y));

		// Compute UV frames for a 4x4 grid sprite sheet
		std::vector<glm::vec4> frames;
		const int cols = 24;
		const int rows = 1;
		float frameWidth = 1.0f / (float)cols;
		float frameHeight = 1.0f / (float)rows;

		std::vector<glm::vec4> idleFrames = GenerateFrames(0, 4, cols, frameWidth, frameHeight);
		std::vector<glm::vec4> walkFrames = GenerateFrames(4, 6, cols, frameWidth, frameHeight);
		std::vector<glm::vec4> attackFrames = GenerateFrames(6, 7, cols, frameWidth, frameHeight);

		for (int row = 0; row < rows; ++row) {
			for (int col = 0; col < cols; ++col) {
				float offsetX = col * frameWidth;
				float offsetY = 1.0f - (row + 1) * frameHeight; // Flip Y
				frames.emplace_back(offsetX, offsetY, frameWidth, frameHeight);
			}
		}

		GameObject* dinoRed = SpawnAnimatedSprite("../assets/dino_red.png",
			glm::vec3(700, 650, 0),
			glm::vec2(128, 128),
			frames, 0.25f, true);

		if (dinoRed) {
			dinoID = dinoRed->GetID();
			std::cout << "Spawned dinoRed sprite with ID: " << dinoID << std::endl;

			// Store all animations
			Animator2D idleAnimator;
			idleAnimator.SetFrames(idleFrames, 0.25f, true);
			idleAnimator.Play();

			Animator2D walkAnimator;
			walkAnimator.SetFrames(walkFrames, 0.15f, true);
			walkAnimator.Play();

			Animator2D attackAnimator;
			attackAnimator.SetFrames(attackFrames, 0.15f, true);
			attackAnimator.Play();

			objectAnimations[dinoID]["IDLE"] = idleAnimator;
			objectAnimations[dinoID]["WALK"] = walkAnimator;
			objectAnimations[dinoID]["ATTACK"] = attackAnimator;

			currentAnimation[dinoID] = "IDLE";
		}

		GameObject* dinoBlue = SpawnAnimatedSprite("../assets/dino_blue.png",
			glm::vec3(700, 500, 0),
			glm::vec2(128, 128),
			frames, 0.25f, true);

		GameObject* dinoGreen = SpawnAnimatedSprite("../assets/dino_green.png",
			glm::vec3(700, 350, 0),
			glm::vec2(128, 128),
			frames, 0.25f, true);

		GameObject* dinoYellow = SpawnAnimatedSprite("../assets/dino_yellow.png",
			glm::vec3(700, 200, 0),
			glm::vec2(128, 128),
			frames, 0.25f, true);

		// Suppress unused variable warning
		(void)dinoBlue;
		(void)dinoGreen;
		(void)dinoYellow;
	}
	else {
		spriteID = -1; //invalid
		std::cerr << "Failed to spawn sprite" << std::endl;
	}

	// Other1
	const glm::vec3 kOtherSpawn1 = { kLaneX, 200.0f, 0.0f };

	if (GameObject* other1 = SpawnStaticSprite("../assets/goat_sprite_front.png", kOtherSpawn1, kVisualSizePx)) {
		otherID = other1->GetID();

		spritePositions[otherID] = kOtherSpawn1;
		spriteScales[otherID] = glm::vec3(kVisualSizePx, 1.0f);
		spriteRotations[otherID] = 0.0f;

		other1->SetColliderSize(Math::Vector2D(colliderSizePx.x, colliderSizePx.y));
		other1->SetColliderOffset(Math::Vector2D(colliderOffsetPx.x, colliderOffsetPx.y));
	}
	else {
		otherID = -1; //invalid
		std::cerr << "Failed to spawn sprite" << std::endl;
	}

	// Other2
	const glm::vec3 kOtherSpawn2 = { kLaneX, 800.0f, 0.0f };

	if (GameObject* other2 = SpawnStaticSprite("../assets/goat_sprite_front.png", kOtherSpawn2, kVisualSizePx)) {
		otherID2 = other2->GetID();

		spritePositions[otherID2] = kOtherSpawn2;
		spriteScales[otherID2] = glm::vec3(kVisualSizePx, 1.0f);
		spriteRotations[otherID2] = 0.0f;

		other2->SetColliderSize(Math::Vector2D(colliderSizePx.x, colliderSizePx.y));
		other2->SetColliderOffset(Math::Vector2D(colliderOffsetPx.x, colliderOffsetPx.y));
	}
	else {
		otherID2 = -1; //invalid
		std::cerr << "Failed to spawn sprite" << std::endl;
	}

	BuildLevelColliders();
}

void Scene::SetAnimation(int objID, const std::string& newAnim) {
	if (objectAnimations.count(objID) &&
		objectAnimations[objID].count(newAnim) &&
		currentAnimation[objID] != newAnim)
	{
		currentAnimation[objID] = newAnim;
		objectAnimations[objID][newAnim].Play();  // restart animation
	}
}

// Per-frame update
void Scene::Update(float deltaTime, GLFWwindow* window) {
	inputManager.Update(window);

	const float physicsDt = physicsStep_.resolveDt(inputManager, deltaTime);
	const collision::WalkArea walk{ kWalkL, kWalkR, kWalkT, kWalkB, kEdgeThick };

	for (auto& [id, animMap] : objectAnimations) {
		std::string& animName = currentAnimation[id];
		Animator2D& animator = animMap[animName];
		animator.Update(deltaTime);

		GameObject* obj = GetGameObjectByID(id);
		if (!obj) continue;
		Shader* shader = obj->GetShader();
		if (!shader) continue;

		glm::vec4 uvFrame = animator.GetCurrentFrameUV();
		shader->Use();
		shader->SetUVOffset(glm::vec2(uvFrame.x, uvFrame.y));
		shader->SetUVScale(glm::vec2(uvFrame.z, uvFrame.w));
	}

	const float rotationSpeed = 1.0f * deltaTime; // degrees per second
	float moveSpeed = 200.0f * physicsDt;

	if (spriteID < 0) return;
	GameObject* sprite = GetGameObjectByID(spriteID);
	if (!sprite) {
		std::cerr << "Sprite with ID " << spriteID << " not found" << std::endl;
		return;
	}

	GameObject* other1 = GetGameObjectByID(otherID);
	if (!other1) {
		std::cerr << "Sprite with ID " << otherID << " not found" << std::endl;
		return;
	}
	GameObject* other2 = GetGameObjectByID(otherID2);
	if (!other2)
	{
		std::cerr << "Sprite with ID " << otherID2 << " not found" << std::endl;
		return;
	}

	glm::vec3& position = spritePositions[spriteID];
	glm::vec3& scale = spriteScales[spriteID];
	float& rotation = spriteRotations[spriteID];

	glm::vec3& o1position = spritePositions[otherID];
	glm::vec3& o2position = spritePositions[otherID2];

	// Per-frame state
	static Math::Vector2D desiredMoveM{ 0.f, 0.f };
	static Math::Vector2D playerVelM{ 0.f, 0.f };
	static Math::Vector2D other1VelM{ 0.f, 100.f };
	static Math::Vector2D other2VelM{ 0.f, -100.f };

	desiredMoveM = Math::Vector2D(0.f, 0.f);

	if (inputManager.IsKeyPressed(GLFW_KEY_UP)) {
		std::cout << "Up key pressed: scale = " << scale.x << "," << scale.y << "," << scale.z << std::endl;
		scale *= 1.01f;

		// Clamp max scale
		scale = glm::min(scale, glm::vec3(500.0f));
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_DOWN)) {
		std::cout << "Down key pressed: scale = " << scale.x << "," << scale.y << "," << scale.z << std::endl;
		scale *= 0.99f;

		// Clamp min scale
		scale = glm::max(scale, glm::vec3(50.0f));
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_RIGHT)) {
		rotation += rotationSpeed;
		if (rotation > 360.0f) rotation -= 360.0f;

		std::cout << "Right key pressed: rotation = " << rotation << std::endl;
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_LEFT)) {
		rotation -= rotationSpeed;
		if (rotation < 0.0f) rotation += 360.0f;

		std::cout << "Left key pressed: rotation = " << rotation << std::endl;
	}

	// Keyboard movement + facing textures
	const float movePerFrame = 200.0f * physicsDt; // displacement this frame
	if (inputManager.IsKeyPressed(GLFW_KEY_W)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_sprite_back.png"));
		desiredMoveM.y -= moveSpeed; // up
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_S)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_sprite_front.png"));
		desiredMoveM.y += moveSpeed; // down
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_A)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideleft", "../assets/mc_sprite_left.png"));
		desiredMoveM.x -= moveSpeed; // left
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_D)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideright", "../assets/mc_sprite_right.png"));
		desiredMoveM.x += moveSpeed; // right
	}

	if (inputManager.IsKeyPressed(GLFW_KEY_1)) {
		SetAnimation(dinoID, "WALK");
		std::cout << "Set to Walk Animation" << std::endl;
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_2)) {
		SetAnimation(dinoID, "ATTACK");
		std::cout << "Set to Attack Animation" << std::endl;
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_3)) {
		SetAnimation(dinoID, "IDLE");
		std::cout << "Set to Idle Animation" << std::endl;
	}
	if (inputManager.IsKeyJustPressed(GLFW_KEY_R)) {
		DebugRenderer::SetEnabled(!DebugRenderer::IsEnabled());
		std::cout << "[DebugRenderer] Collider box visibility: "
			<< (DebugRenderer::IsEnabled() ? "ON" : "OFF") << std::endl;
	}
	if (inputManager.IsKeyJustPressed(GLFW_KEY_T)) {
		showAuxDebug_ = !showAuxDebug_;
		std::cout << "[Debug] Points/Lines: " << (showAuxDebug_ ? "ON" : "OFF") << std::endl;
	}

	// Click-to-Move behaviour
	// If player is NOT selected: click must hit the player's collider to select.
	// If already selected: the next left click sets the destination target.
	if (inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		auto mp = inputManager.GetMousePosition();
		glm::vec2 mouse{ (float)mp.x, (float)mp.y };

		if (!playerSelected) {
			const Math::Vector2D csize = sprite->GetColliderSize();
			const Math::Vector2D coff = sprite->GetColliderOffset();

			const Math::Vector3D selCenterM(position.x + coff.x, position.y + coff.y, position.z);
			const Math::Vector3D selScaleM(csize.x, csize.y, 1.0f);

			if (collision::pointInsideCenterAABB(toM(mouse), selCenterM, selScaleM)) {
				playerSelected = true;
				hasClickTarget = false;
				stuckFrames = 0;
			}
		}
		else {
			clickTarget = mouse;
			hasClickTarget = true;
			stuckFrames = 0;

			// Face toward the new target (dominant axis)
			glm::vec2 toTarget = clickTarget - glm::vec2(position.x, position.y);
			if (glm::length(toTarget) > 0.001f) {
				float ax = std::abs(toTarget.x);
				float ay = std::abs(toTarget.y);
				if (ax >= ay) {
					if (toTarget.x >= 0.0f) {
						sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideright", "../assets/mc_sprite_right.png"));
					}
					else {
						sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideleft", "../assets/mc_sprite_left.png"));
					}
				}
				else {
					if (toTarget.y >= 0.0f) {
						sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_sprite_front.png"));
					}
					else {
						sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_sprite_back.png"));
					}
				}
			}
		}

	}

	if (inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
		playerSelected = false;
		hasClickTarget = false;
		stuckFrames = 0;
	}

	// Build current AABB from collider size/offset for collision resolution
	const Math::Vector3D centerM(position.x + sprite->GetColliderOffset().x, position.y + sprite->GetColliderOffset().y, position.z);
	const Math::Vector3D scaleM(sprite->GetColliderSize().x, sprite->GetColliderSize().y, 1.0f);
	collision::AABB startBox = collision::World::makeAABBFromCenter(centerM, scaleM);

	// Only apply click-to-move if we have an active target and the player is selected
	if (playerSelected && hasClickTarget) {
		const glm::vec2 pos2(position.x, position.y);
		glm::vec2 toTarget = clickTarget - pos2;
		const float dist = glm::length(toTarget);

		// Update facing each frame while pathing (dominant axis)
		if (dist > 0.001f) {
			float ax = std::abs(toTarget.x);
			float ay = std::abs(toTarget.y);
			if (ax >= ay) {
				if (toTarget.x >= 0.0f) {
					sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideright", "../assets/mc_sprite_right.png"));
				}
				else {
					sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideleft", "../assets/mc_sprite_left.png"));
				}
			}
			else {
				if (toTarget.y >= 0.0f) {
					sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_sprite_front.png"));
				}
				else {
					sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_sprite_back.png"));
				}
			}
		}

		if (dist > 0.0f) {
			const float maxStep = playerSpeed * physicsDt;
			const glm::vec2 step = (dist <= maxStep) ? toTarget : (toTarget / dist) * maxStep;
			desiredMoveM = Math::Vector2D(step.x + desiredMoveM.x, step.y + desiredMoveM.y);
		}
	}

	const float kLaneX = 1000.0f;
	{
		Math::Vector3D posM = toM(o1position);
		physics::MoveYLaneWithBounce(mCollision, other1, posM, other1VelM, kLaneX, physicsDt);
		physics::ClampInsideWalk(walk, other1, posM);
		o1position = toG(posM);
		other1->SetPosition(o1position);
	}

	{
		Math::Vector3D posM = toM(o2position);
		physics::MoveYLaneWithBounce(mCollision, other2, posM, other2VelM, kLaneX, physicsDt);
		physics::ClampInsideWalk(walk, other2, posM);
		o2position = toG(posM);
		other2->SetPosition(o2position);
	}

	{
		Math::Vector3D p1M = toM(o1position);
		Math::Vector3D p2M = toM(o2position);
		physics::ElasticBounceEqualMass(other1, other2, p1M, p2M, other1VelM, other2VelM);
		o1position = toG(p1M);
		o2position = toG(p2M);
		other1->SetPosition(o1position);
		other2->SetPosition(o2position);
	}

	// Split weight logic (your old heuristic)
	const float intentSpeed = (physicsDt > 0.0f)
		? (Math::Vector2D(desiredMoveM.x / physicsDt, desiredMoveM.y / physicsDt).Length())
		: 0.0f;
	const float other1Speed = other1VelM.Length();
	const float other2Speed = other2VelM.Length();

	auto pickWeight = [&](float otherSpeed) {
		constexpr float kIdle = 5.0f;
		constexpr float kPushBiasIdle = 0.50f;
		constexpr float kPushBiasMoving = 0.50f;
		return (otherSpeed < kIdle && intentSpeed > 0.0f) ? kPushBiasIdle : kPushBiasMoving;
		};

	{
		Math::Vector3D playerPosM = toM(position);
		Math::Vector3D o1PosM = toM(o1position);
		physics::SeparatePlayerVsOther_StopPlayerOnly(
			mCollision, sprite, other1, playerPosM, o1PosM, desiredMoveM, hasClickTarget, pickWeight(other1VelM.Length()));
		position = toG(playerPosM);
		o1position = toG(o1PosM);
		sprite->SetPosition(position);
		other1->SetPosition(o1position);
	}

	{
		Math::Vector3D playerPosM = toM(position);
		Math::Vector3D o2PosM = toM(o2position);
		physics::SeparatePlayerVsOther_StopPlayerOnly(
			mCollision, sprite, other2, playerPosM, o2PosM, desiredMoveM, hasClickTarget, pickWeight(other2VelM.Length()));
		position = toG(playerPosM);
		o2position = toG(o2PosM);
		sprite->SetPosition(position);
		other2->SetPosition(o2position);
	}

	// Resolve desired movement against world walls (X then Y sweep)
	Math::Vector2D allowedM = mCollision.resolve(startBox, desiredMoveM);

	// If that fails, try Y then X sweep (corner case for thin walls)
	if (allowedM.x == 0.f && allowedM.y == 0.f && (desiredMoveM.x != 0.f || desiredMoveM.y != 0.f)) {
		const Math::Vector2D tryX = mCollision.resolve(startBox, Math::Vector2D(desiredMoveM.x, 0.f));
		const Math::Vector2D tryY = mCollision.resolve(startBox, Math::Vector2D(0.f, desiredMoveM.y));

		// Pick the longer of the axis-only moves (same heuristic as before)
		if (std::abs(tryX.x) > std::abs(tryY.y)) {
			allowedM = tryX;
		}
		else {
			allowedM = tryY;
		}
	}

	// Apply allowed move
	position.x += allowedM.x;
	position.y += allowedM.y;

	const collision::StageEndGateVertical gate{
	kEndVX0, kEndVX1,
	kEndVTopMinY, kEndVTopMaxY,
	kEndVGapMinY, kEndVGapMaxY,
	kEndVBotMinY, kEndVBotMaxY
	};

	{
		Math::Vector3D posM = toM(position);
		physics::ClampInsideWalkWithGate(walk, gate, sprite, posM);
		position = toG(posM);
	}
	sprite->SetPosition(position);

	if (playerSelected && hasClickTarget) {
		const float intended = Math::Vector2D(desiredMoveM.x, desiredMoveM.y).Length();
		const float moved = Math::Vector2D(allowedM.x, allowedM.y).Length();

		const glm::vec2 prevPos2 = glm::vec2(position.x, position.y) - glm::vec2(allowedM.x, allowedM.y);
		const float prevDist = glm::length(clickTarget - prevPos2);
		const float newDist = glm::length(clickTarget - glm::vec2(position.x, position.y));

		const bool noProgress = (newDist >= prevDist - 0.25f);
		const bool barelyMoved = (moved <= 0.05f && intended > 0.0f);

		if (noProgress || barelyMoved) ++stuckFrames;
		else stuckFrames = 0;

		if (stuckFrames >= kStuckFramesToCancel) {
			hasClickTarget = false;
			stuckFrames = 0;
		}
	}

	// Final clamps + transforms
	position.x = glm::clamp(position.x, 0.0f, kWorldW);
	position.y = glm::clamp(position.y, 0.0f, kWorldH);
	sprite->SetScale(scale);
	sprite->SetRotation(rotation, glm::vec3(0, 0, 1));
	sprite->SetPosition(position);

	if (DebugRenderer::IsEnabled() && showAuxDebug_) {
		// Line: player to click target (only when a target exists)
		if (playerSelected && hasClickTarget) {
			DebugRenderer::DrawLine(
				{ position.x,   position.y,   0.0f },
				{ clickTarget.x, clickTarget.y, 0.0f },
				{ 0.0f, 1.0f, 0.0f }
			);
		}

		// Points: collider corners (like your AABB, but as GL_POINTS)
		const Math::Vector2D cs = sprite->GetColliderSize();
		const Math::Vector2D co = sprite->GetColliderOffset();

		if (cs.x > 0.0f && cs.y > 0.0f) {
			const glm::vec3 c = { position.x + co.x, position.y + co.y, 0.0f };
			const glm::vec3 mn = { c.x - cs.x * 0.5f, c.y - cs.y * 0.5f, 0.0f };
			const glm::vec3 mx = { c.x + cs.x * 0.5f, c.y + cs.y * 0.5f, 0.0f };

			DebugRenderer::DrawPoint({ mn.x, mn.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f); // BT
			DebugRenderer::DrawPoint({ mx.x, mn.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f); // BR
			DebugRenderer::DrawPoint({ mx.x, mx.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f); // TR
			DebugRenderer::DrawPoint({ mn.x, mx.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f); // TL
			DebugRenderer::DrawPoint(c, { 1.0f, 0.2f, 0.2f }, 7.0f); // center
		}
	}
}
