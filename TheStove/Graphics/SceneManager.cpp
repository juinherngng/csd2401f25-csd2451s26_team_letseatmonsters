#include "SceneManager.h"
#include <iostream>
#include <algorithm>

// Level constants
static constexpr float kWorldW = 1200.0f;
static constexpr float kWorldH = 800.0f;

// Walkable inner rectangle (match to background art)
static constexpr float kWalkL = 148.0f;  // left
static constexpr float kWalkR = 1078.0f; // right
static constexpr float kWalkT = 84.0f;   // top
static constexpr float kWalkB = 733.0f;  // bottom

// Thickness of our blocking bars (thin = precise, easy to tune)
static constexpr float kEdgeThick = 3.0f;

// Wooden divider (vertical split)
static constexpr float kWoodX0 = 562.0f;
static constexpr float kWoodX1 = 590.0f;
static constexpr float kWoodTopMinY = 50.0f;
static constexpr float kWoodTopMaxY = 250.0f;
static constexpr float kWoodGapMinY = 250.0f;
static constexpr float kWoodGapMaxY = 500.0f;
static constexpr float kWoodBotMinY = 500.0f;
static constexpr float kWoodBotMaxY = 700.0f;

// End-of-stage vertical gate
static constexpr float kEndVX0 = 1100.0f;
static constexpr float kEndVX1 = 1132.0f;
static constexpr float kEndVTopMinY = 50.0f;
static constexpr float kEndVTopMaxY = 250.0f;
static constexpr float kEndVGapMinY = 250.0f;
static constexpr float kEndVGapMaxY = 500.0f;
static constexpr float kEndVBotMinY = 500.0f;
static constexpr float kEndVBotMaxY = 700.0f;

// Scene lifecycle
Scene::Scene(GraphicsEngine& engine) : graphicsEngine(engine) {}

void Scene::LoadScene(const std::string& sceneName) {
	(void)sceneName; // Suppress unused parameter warning
	LoadTest();
}

// Spawners/background/lookup
GameObject* Scene::SpawnTriangle(const glm::vec3& position, const glm::vec3& scale, float rotation) {
	GameObject* obj = graphicsEngine.CreateGameObject("triangle", "basic");
	if (obj) {
		obj->SetPosition(position);
		obj->SetScale(scale);
		obj->SetRotation(glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
		sceneObjects.push_back(std::unique_ptr<GameObject>(obj));
	}
	return obj;
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

	collision::StageEndGateVertical end{
		kEndVX0, kEndVX1,
		kEndVTopMinY, kEndVTopMaxY,
		kEndVGapMinY, kEndVGapMaxY,
		kEndVBotMinY, kEndVBotMaxY
	};

	mCollision.build(walk, wood, end);
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
	if (GameObject* player = SpawnSprite("../assets/mc_sprite_front.png", kSpawnPos, kVisualSizePx)) {
		spriteID = player->GetID();
		std::cout << "Spawned sprite with ID: " << spriteID << std::endl;

		spriteScales[spriteID] = glm::vec3(kVisualSizePx, 1.0f);
		spriteRotations[spriteID] = 0.0f;
		spritePositions[spriteID] = kSpawnPos;

		player->SetScale(glm::vec3(kVisualSizePx, 1.0f));
		player->SetColliderSize(colliderSizePx);
		player->SetColliderOffset(colliderOffsetPx);
	}
	else {
		spriteID = -1; //invalid
		std::cerr << "Failed to spawn sprite" << std::endl;
	}

	// Other1
	const glm::vec3 kOtherSpawn1 = { kLaneX, 200.0f, 0.0f };

	if (GameObject* other1 = SpawnSprite("../assets/goat_sprite_front.png", kOtherSpawn1, kVisualSizePx)) {
		otherID = other1->GetID();

		spritePositions[otherID] = kOtherSpawn1;
		spriteScales[otherID] = glm::vec3(kVisualSizePx, 1.0f);
		spriteRotations[otherID] = 0.0f;

		other1->SetColliderSize(colliderSizePx);
		other1->SetColliderOffset(colliderOffsetPx);
	}
	else {
		otherID = -1; //invalid
		std::cerr << "Failed to spawn sprite" << std::endl;
	}

	// Other2
	const glm::vec3 kOtherSpawn2 = { kLaneX, 800.0f, 0.0f };

	if (GameObject* other2 = SpawnSprite("../assets/goat_sprite_front.png", kOtherSpawn2, kVisualSizePx)) {
		otherID2 = other2->GetID();

		spritePositions[otherID2] = kOtherSpawn2;
		spriteScales[otherID2] = glm::vec3(kVisualSizePx, 1.0f);
		spriteRotations[otherID2] = 0.0f;

		other2->SetColliderSize(colliderSizePx);
		other2->SetColliderOffset(colliderOffsetPx);
	}
	else {
		otherID2 = -1; //invalid
		std::cerr << "Failed to spawn sprite" << std::endl;
	}

	BuildLevelColliders();
}

// Per-frame update
void Scene::Update(float deltaTime, GLFWwindow* window) {
	inputManager.Update(window);

	const float physicsDt = physicsStep_.resolveDt(inputManager, deltaTime);
	const collision::WalkArea walk{ kWalkL, kWalkR, kWalkT, kWalkB, kEdgeThick };

	if (spriteID < 0) return;
	GameObject* player = GetGameObjectByID(spriteID);
	GameObject* other1 = GetGameObjectByID(otherID);
	GameObject* other2 = GetGameObjectByID(otherID2);
	if (!player || !other1 || !other2) {
		std::cerr << "Player/Other not found\n";
		return;
	}

	glm::vec3& pPos = spritePositions[spriteID];
	glm::vec3& o1Pos = spritePositions[otherID];
	glm::vec3& o2Pos = spritePositions[otherID2];
	glm::vec3& scale = spriteScales[spriteID];
	float& rotation = spriteRotations[spriteID];

	// Per-frame state
	static glm::vec2 desiredMove{ 0.0f, 0.0f };
	static glm::vec2 playerVelocity{ 0.0f, 0.0f };
	static glm::vec2 other1Velocity{ 0.0f,  100.0f };
	static glm::vec2 other2Velocity{ 0.0f, -100.0f };

	desiredMove = { 0.0f, 0.0f };

	// UI: scale/rotation
	const float rotationSpeed = 1.0f * deltaTime; // degrees per second

	if (inputManager.IsKeyPressed(GLFW_KEY_UP)) {
		std::cout << "Up key pressed: scale = " << scale.x << "," << scale.y << "," << scale.z << std::endl;
		scale *= 1.01f;

		//Clamp max scale
		scale = glm::min(scale, glm::vec3(500.0f));
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_DOWN)) {
		std::cout << "Down key pressed: scale = " << scale.x << "," << scale.y << "," << scale.z << std::endl;
		scale *= 0.99f;

		//Clamp min scale
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
		player->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_sprite_back.png"));
		desiredMove.y -= movePerFrame; // up
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_S)) {
		player->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_sprite_front.png"));
		desiredMove.y += movePerFrame; // down
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_A)) {
		player->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideleft", "../assets/mc_sprite_left.png"));
		desiredMove.x -= movePerFrame; // left
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_D)) {
		player->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideright", "../assets/mc_sprite_right.png"));
		desiredMove.x += movePerFrame; // right
	}

	// Click to move selection & target
	if (inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		const auto mp = inputManager.GetMousePosition();
		const glm::vec2 mouse{ (float)mp.x, (float)mp.y };

		if (!playerSelected) {
			const glm::vec2 csize = player->GetColliderSize();
			const glm::vec2 coff = player->GetColliderOffset();
			const glm::vec3 selCenter = pPos + glm::vec3(coff, 0.0f);
			const glm::vec3 selScale = glm::vec3(csize, 1.0f);

			if (collision::pointInsideCenterAABB(mouse, selCenter, selScale)) {
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
			glm::vec2 toTarget = clickTarget - glm::vec2(pPos.x, pPos.y);
			if (glm::length(toTarget) > 0.001f) {
				float ax = std::abs(toTarget.x);
				float ay = std::abs(toTarget.y);
				if (ax >= ay) {
					if (toTarget.x >= 0.0f) {
						player->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideright", "../assets/mc_sprite_right.png"));
					}
					else {
						player->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideleft", "../assets/mc_sprite_left.png"));
					}
				}
				else {
					if (toTarget.y >= 0.0f) {
						player->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_sprite_front.png"));
					}
					else {
						player->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_sprite_back.png"));
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

	// Only apply click-to-move if we have an active target and the player is selected
	if (playerSelected && hasClickTarget) {
		const glm::vec2 pos2(pPos.x, pPos.y);
		glm::vec2 toTarget = clickTarget - pos2;
		const float dist = glm::length(toTarget);

		// Update facing each frame while pathing (dominant axis)
		if (dist > 0.001f) {
			float ax = std::abs(toTarget.x);
			float ay = std::abs(toTarget.y);
			if (ax >= ay) {
				if (toTarget.x >= 0.0f) {
					player->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideright", "../assets/mc_sprite_right.png"));
				}
				else {
					player->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideleft", "../assets/mc_sprite_left.png"));
				}
			}
			else {
				if (toTarget.y >= 0.0f) {
					player->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_sprite_front.png"));
				}
				else {
					player->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_sprite_back.png"));
				}
			}
		}

		if (dist > 0.0f) {
			const float maxStep = playerSpeed * physicsDt;
			const glm::vec2 step = (dist <= maxStep) ? toTarget : (toTarget / dist) * maxStep;
			desiredMove += step;
		}
	}

	const float kLaneX = 1000.0f;
	physics::MoveYLaneWithBounce(mCollision, other1, o1Pos, other1Velocity, kLaneX, physicsDt);
	physics::ClampInsideWalk(walk, other1, o1Pos);
	other1->SetPosition(o1Pos);

	physics::MoveYLaneWithBounce(mCollision, other2, o2Pos, other2Velocity, kLaneX, physicsDt);
	physics::ClampInsideWalk(walk, other2, o2Pos);
	other2->SetPosition(o2Pos);

	physics::ElasticBounceEqualMass(other1, other2, o1Pos, o2Pos, other1Velocity, other2Velocity);

	// Split weight logic (your old heuristic)
	const float intentSpeed = (physicsDt > 0.0f) ? (glm::length(desiredMove) / physicsDt) : 0.0f;
	const float other1Speed = glm::length(other1Velocity);
	const float other2Speed = glm::length(other2Velocity);

	auto pickWeight = [&](float otherSpeed) {
		constexpr float kIdle = 5.0f;
		constexpr float kPushBiasIdle = 0.50f;
		constexpr float kPushBiasMoving = 0.50f;
		return (otherSpeed < kIdle && intentSpeed > 0.0f) ? kPushBiasIdle : kPushBiasMoving;
		};

	physics::SeparatePlayerVsOther_StopPlayerOnly(
		mCollision, player, other1, pPos, o1Pos, desiredMove, hasClickTarget, pickWeight(other1Speed));
	physics::SeparatePlayerVsOther_StopPlayerOnly(
		mCollision, player, other2, pPos, o2Pos, desiredMove, hasClickTarget, pickWeight(other2Speed));


	// Move player vs world + stuck guard
	const collision::AABB startBox = physics::MakeColliderBox(player, pPos);
	const glm::vec2 stepPlayer = mCollision.resolve(startBox, desiredMove);
	pPos += glm::vec3(stepPlayer, 0.0f);
	physics::ClampInsideWalk(walk, player, pPos);
	player->SetPosition(pPos);
	playerVelocity = (physicsDt > 0.0f) ? (stepPlayer / physicsDt) : glm::vec2{ 0.0f };

	if (playerSelected && hasClickTarget) {
		const float intended = glm::length(desiredMove);
		const float moved = glm::length(stepPlayer);

		const glm::vec2 prevPos2 = glm::vec2(pPos.x, pPos.y) - stepPlayer;
		const float prevDist = glm::length(clickTarget - prevPos2);
		const float newDist = glm::length(clickTarget - glm::vec2(pPos.x, pPos.y));

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
	pPos.x = std::clamp(pPos.x, 0.0f, kWorldW);
	pPos.y = std::clamp(pPos.y, 0.0f, kWorldH);
	player->SetScale(scale);
	player->SetRotation(rotation, glm::vec3(0, 0, 1));
	player->SetPosition(pPos);
}
