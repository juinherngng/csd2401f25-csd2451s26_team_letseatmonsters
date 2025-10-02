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

// Physics step-by-step debug
static bool gPhysicsStepMode = false; // toggle ON/OFF
static int gStepsQueued = 0;          // how many single steps to run
static float gStepDt = 1.0f / 60.0f;  // fixed dt for each step

// Small helpers (internal)
static inline bool PointInsideCenterAABB(glm::vec2 p, glm::vec3 center, glm::vec3 scale) {
	const float hx = scale.x * 0.5f;
	const float hy = scale.y * 0.5f;
	return (p.x >= center.x - hx && p.x <= center.x + hx &&
		p.y >= center.y - hy && p.y <= center.y + hy);
}

static inline collision::AABB MakeColliderBox(GameObject* obj, const glm::vec3& pos) {
	return collision::World::makeAABBFromCenter(
		pos + glm::vec3(obj->GetColliderOffset(), 0.0f),
		glm::vec3(obj->GetColliderSize(), 1.0f));
}

static inline void ClampInsideWalk(GameObject* obj, glm::vec3& pos) {
	const glm::vec2 sz = obj->GetColliderSize();
	const glm::vec2 off = obj->GetColliderOffset();
	const glm::vec2 half = sz * 0.5f;

	pos.x = std::clamp(pos.x, kWalkL + half.x - off.x, kWalkR - half.x - off.x);
	pos.y = std::clamp(pos.y, kWalkT + half.y - off.y, kWalkB - half.y - off.y);
}

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

GameObject* Scene::SpawnSprite(const std::string& texturePath, const glm::vec3& position, const glm::vec2& size) {
	// Load sprite texture if not already loaded
	std::string textureName = "sprite_" + texturePath; // Simple naming scheme
	Texture* spriteTexture = ResourceManager::Instance().LoadTexture(textureName, texturePath);

	if (!spriteTexture) {
		std::cerr << "Failed to load sprite texture: " << texturePath << std::endl;
		return nullptr;
	}

	// Create sprite game object with unique ID
	GameObject* obj = graphicsEngine.CreateGameObject("sprite", "sprite");
	if (obj) {
		obj->SetID(nextID++);
		obj->SetPosition(position);
		obj->SetScale(glm::vec3(size.x, size.y, 1.0f));
		obj->SetTexture(spriteTexture);
		sceneObjects.push_back(std::unique_ptr<GameObject>(obj));
	}

	return obj;
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

	// Physics step-by-step controls 
	static bool prevToggle = false;											// P key
	static bool prevW = false, prevA = false, prevS = false, prevD = false; // WASD

	// Read current inputs (this frame)
	bool toggleNow = inputManager.IsKeyPressed(GLFW_KEY_P);
	bool wNow = inputManager.IsKeyPressed(GLFW_KEY_W);
	bool aNow = inputManager.IsKeyPressed(GLFW_KEY_A);
	bool sNow = inputManager.IsKeyPressed(GLFW_KEY_S);
	bool dNow = inputManager.IsKeyPressed(GLFW_KEY_D);

	// Toggle step mode on P
	if (toggleNow && !prevToggle) {
		gPhysicsStepMode = !gPhysicsStepMode;
		std::cout << "[Physics] Step mode " << (gPhysicsStepMode ? "ON" : "OFF") << "\n";
	}

	// Queue one step per left click when step mode is ON (edge from your input manager)
	if (gPhysicsStepMode && inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		++gStepsQueued;
		std::cout << "[Physics] Step from mouse click\n";
	}

	// Queue one step on WASD key-down edge (any of them) when step mode is ON
	if (gPhysicsStepMode) {
		bool anyDownEdge =
			(wNow && !prevW) ||
			(aNow && !prevA) ||
			(sNow && !prevS) ||
			(dNow && !prevD);

		if (anyDownEdge) {
			++gStepsQueued;
			std::cout << "[Physics] Step from WASD press\n";
		}
	}

	// Update edge-state after queuing so we detect edges correctly next frame
	prevToggle = toggleNow;
	prevW = wNow; prevA = aNow; prevS = sNow; prevD = dNow;

	// Resolve the dt to use for physics this frame, then consume one queued step
	const float physicsDt =
		gPhysicsStepMode
		? (gStepsQueued > 0 ? gStepDt : 0.0f)
		: deltaTime;

	// Run exactly one physics slice this frame
	if (gPhysicsStepMode && physicsDt > 0.0f) {
		--gStepsQueued;
	}

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

			if (PointInsideCenterAABB(mouse, selCenter, selScale)) {
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
	auto moveOtherWithBounce = [&](GameObject* obj, glm::vec3& pos, glm::vec2& vel) {
		// Lock to vertical lane
		pos.x = kLaneX;

		const glm::vec2 desired = vel * physicsDt;
		const collision::AABB start = MakeColliderBox(obj, pos);
		const glm::vec2 allowed = mCollision.resolve(start, desired);

		// Apply
		pos += glm::vec3(allowed, 0.0f);
		obj->SetPosition(pos);

		// Bounce off walls: if a component was blocked this frame, flip that velocity component
		if (physicsDt > 0.0f) {
			const float eps = 1e-4f;
			if (std::abs(allowed.x - desired.x) > eps) vel.x = -vel.x;
			if (std::abs(allowed.y - desired.y) > eps) vel.y = -vel.y;
		}

		// Keep inferred velocity consistent (optional, useful for displays)
		if (physicsDt > 0.0f) {
			// If we bounced, allowed ≠ desired; keep magnitude from vel we just updated
			// (No need to recompute from allowed.)
		}

		// Clamp to walk area using collider
		ClampInsideWalk(obj, pos);
		pos.x = kLaneX;
		obj->SetPosition(pos);
		};

	// Move both
	moveOtherWithBounce(other1, o1Pos, other1Velocity);
	moveOtherWithBounce(other2, o2Pos, other2Velocity);

	// ---- Other1/Other2 elastic bounce (equal mass, robust normal) ----
	{
		collision::AABB a = MakeColliderBox(other1, o1Pos);
		collision::AABB b = MakeColliderBox(other2, o2Pos);
		glm::vec2 mtv;
		if (collision::overlapMTV(a, b, mtv)) {
			// Separate them evenly by MTV (even if very small)
			glm::vec2 half = 0.5f * mtv;
			o1Pos += glm::vec3(+half, 0.0f);
			o2Pos += glm::vec3(-half, 0.0f);
			other1->SetPosition(o1Pos);
			other2->SetPosition(o2Pos);

			// --- Robust normal ---
			glm::vec2 n;
			float len = glm::length(mtv);
			if (len > 1e-6f) {
				n = mtv / len; // safe to normalize
			}
			else {
				// Fallback: choose axis by relative centers or current motion.
				glm::vec2 rel = glm::vec2(o1Pos.x - o2Pos.x, o1Pos.y - o2Pos.y);
				if (std::abs(rel.y) >= std::abs(rel.x)) {
					n = { 0.0f, (rel.y >= 0.0f ? 1.0f : -1.0f) };  // vertical
				}
				else {
					n = { (rel.x >= 0.0f ? 1.0f : -1.0f), 0.0f };  // horizontal
				}
			}

			// Swap normal components of velocity (perfectly elastic, equal mass)
			float v1n = glm::dot(other1Velocity, n);
			float v2n = glm::dot(other2Velocity, n);

			glm::vec2 v1t = other1Velocity - v1n * n;
			glm::vec2 v2t = other2Velocity - v2n * n;

			// Exchange the normal components
			other1Velocity = v1t + v2n * n;
			other2Velocity = v2t + v1n * n;
		}
	}

	auto playerVsOtherStop = [&](GameObject* otherObj, glm::vec3& otherPos, glm::vec2& otherVel) {
		const float otherSpeed = glm::length(otherVel);
		const float intentSpeed = (physicsDt > 0.0f)
			? (glm::length(desiredMove) / physicsDt)
			: 0.0f;

		constexpr float kIdle = 5.0f;
		constexpr float kPushBiasIdle = 0.50f;
		constexpr float kPushBiasMoving = 0.50f;

		float weightPlayer = (otherSpeed < kIdle && intentSpeed > 0.0f)
			? kPushBiasIdle
			: kPushBiasMoving;

		const collision::AABB a = MakeColliderBox(player, pPos);
		const collision::AABB b = MakeColliderBox(otherObj, otherPos);

		glm::vec2 pushA, pushB;
		if (collision::separateWeighted(a, b, weightPlayer, pushA, pushB)) {
			// Other: world-safe correction
			const collision::AABB otherBoxForSeparation = MakeColliderBox(otherObj, otherPos);
			const glm::vec2 otherSeparationAllowed = mCollision.resolve(otherBoxForSeparation, pushB);
			const glm::vec2 otherSeparationBlocked = pushB - otherSeparationAllowed;
			if (otherSeparationBlocked.x != 0.0f || otherSeparationBlocked.y != 0.0f) {
				pushA += otherSeparationBlocked;   // blocked portion → player
				pushB = otherSeparationAllowed;
				desiredMove = { 0.0f, 0.0f };
				hasClickTarget = false;
			}

			// Player: world-safe correction
			const collision::AABB playerBoxForSeparation = MakeColliderBox(player, pPos);
			const glm::vec2 playerSeparationAllowed = mCollision.resolve(playerBoxForSeparation, pushA);
			pushA = playerSeparationAllowed;

			// Apply separation
			pPos += glm::vec3(pushA, 0.0f);
			otherPos += glm::vec3(pushB, 0.0f);
			player->SetPosition(pPos);
			otherObj->SetPosition(otherPos);

			// STOP both
			playerVelocity = { 0.0f, 0.0f };
			// otherVel = { 0.0f, 0.0f };

			// Epsilon safety
			glm::vec2 mtv;
			if (collision::overlapMTV(MakeColliderBox(player, pPos),
				MakeColliderBox(otherObj, otherPos), mtv)) {
				pPos += glm::vec3(mtv * 1.001f, 0.0f);
				player->SetPosition(pPos);
			}

			// Clamp other
			ClampInsideWalk(otherObj, otherPos);
			otherObj->SetPosition(otherPos);
		}
		};

	// Run STOP vs both others
	playerVsOtherStop(other1, o1Pos, other1Velocity);
	playerVsOtherStop(other2, o2Pos, other2Velocity);

	// Move player vs world + stuck guard
	const collision::AABB startBox = MakeColliderBox(player, pPos);
	const glm::vec2 stepPlayer = mCollision.resolve(startBox, desiredMove);

	const glm::vec2 prevAllowed = (deltaTime > 0.0f) ? (playerVelocity * deltaTime) : glm::vec2{ 0.0f };
	pPos.x += stepPlayer.x; pPos.y += stepPlayer.y;
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
