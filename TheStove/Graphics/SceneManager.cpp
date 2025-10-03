#include "SceneManager.h"
#include <iostream>
#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>

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

GameObject* Scene::SpawnStaticSprite(const std::string& texturePath, const glm::vec3& position, const glm::vec2& size) {
	// Load sprite texture if not already loaded
	std::string textureName = "sprite_" + texturePath; // Simple naming scheme
	Texture* spriteTexture = ResourceManager::Instance().LoadTexture(textureName, texturePath);

	if (!spriteTexture) {
		std::cerr << "Failed to load sprite texture: " << texturePath << std::endl;
		return nullptr;
	}

	// Create sprite game object with unique ID
	GameObject* obj = graphicsEngine.CreateGameObject("sprite", "staticsprite");
	if (obj) {
		obj->SetID(nextID++);
		obj->SetPosition(position);
		obj->SetScale(glm::vec3(size.x, size.y, 1.0f));
		obj->SetTexture(spriteTexture);
		sceneObjects.push_back(std::unique_ptr<GameObject>(obj));
	}

	return obj;
}

GameObject* Scene::SpawnAnimatedSprite(const std::string& texturePath, const glm::vec3& position, const glm::vec2& size, const std::vector<glm::vec4>& frames, float frameDuration, bool loop)
{
	std::string textureName = "sprite_" + texturePath;
	Texture* spriteTexture = ResourceManager::Instance().LoadTexture(textureName, texturePath);

	if (!spriteTexture) {
		std::cerr << "Failed to load animated sprite texture: " << texturePath << std::endl;
		return nullptr;
	}

	GameObject* obj = graphicsEngine.CreateGameObject("sprite", "animatedsprite");
	if (obj) {
		obj->SetID(nextID++);
		obj->SetPosition(position);
		obj->SetScale(glm::vec3(size.x, size.y, 1.0f));
		obj->SetTexture(spriteTexture);
		sceneObjects.push_back(std::unique_ptr<GameObject>(obj));

		Animator2D animator;
		animator.SetFrames(frames, frameDuration, loop);
		animator.Play();
		animators[obj->GetID()] = animator;  // Add animator for this animated sprite
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

// Utility function to generate UV frames for a sprite sheet
std::vector<glm::vec4> GenerateFrames(int startFrame, int frameCount, int totalCols, float frameWidth, float frameHeight) {
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
		std::cout << "Spawned sprite with ID: " << spriteID << std::endl;

		spriteScales[spriteID] = glm::vec3(kVisualSizePx, 1.0f);
		spriteRotations[spriteID] = 0.0f;
		spritePositions[spriteID] = kSpawnPos;

		player->SetScale(glm::vec3(kVisualSizePx, 1.0f));
		player->SetColliderSize(colliderSizePx);
		player->SetColliderOffset(colliderOffsetPx);

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

		GameObject* dinoBblue = SpawnAnimatedSprite("../assets/dino_blue.png",
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

		other1->SetColliderSize(colliderSizePx);
		other1->SetColliderOffset(colliderOffsetPx);
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

		other2->SetColliderSize(colliderSizePx);
		other2->SetColliderOffset(colliderOffsetPx);
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
	float moveSpeed = 200.0f * deltaTime;

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
	static glm::vec2 desiredMove{ 0.0f, 0.0f };
	static glm::vec2 playerVelocity{ 0.0f, 0.0f };
	static glm::vec2 other1Velocity{ 0.0f,  100.0f };
	static glm::vec2 other2Velocity{ 0.0f, -100.0f };

	desiredMove = { 0.0f, 0.0f };

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
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_sprite_back.png"));
		desiredMove.y -= movePerFrame; // up
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_S)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_sprite_front.png"));
		desiredMove.y += movePerFrame; // down
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_A)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideleft", "../assets/mc_sprite_left.png"));
		desiredMove.x -= movePerFrame; // left
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_D)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideright", "../assets/mc_sprite_right.png"));
		desiredMove.x += movePerFrame; // right
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

	// Click to move selection & target
	if (inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		const auto mp = inputManager.GetMousePosition();
		const glm::vec2 mouse{ (float)mp.x, (float)mp.y };

		if (!playerSelected) {
			const glm::vec2 csize = sprite->GetColliderSize();
			const glm::vec2 coff = sprite->GetColliderOffset();
			const glm::vec3 selCenter = position + glm::vec3(coff, 0.0f);
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
			desiredMove += step;
		}
	}

	const float kLaneX = 1000.0f;
	physics::MoveYLaneWithBounce(mCollision, other1, o1position, other1Velocity, kLaneX, physicsDt);
	physics::ClampInsideWalk(walk, other1, o1position);
	other1->SetPosition(o1position);

	physics::MoveYLaneWithBounce(mCollision, other2, o2position, other2Velocity, kLaneX, physicsDt);
	physics::ClampInsideWalk(walk, other2, o2position);
	other2->SetPosition(o2position);

	physics::ElasticBounceEqualMass(other1, other2, o1position, o2position, other1Velocity, other2Velocity);

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
		mCollision, sprite, other1, position, o1position, desiredMove, hasClickTarget, pickWeight(other1Speed));
	physics::SeparatePlayerVsOther_StopPlayerOnly(
		mCollision, sprite, other2, position, o2position, desiredMove, hasClickTarget, pickWeight(other2Speed));


	// Move player vs world + stuck guard
	const collision::AABB startBox = physics::MakeColliderBox(sprite, position);
	const glm::vec2 stepPlayer = mCollision.resolve(startBox, desiredMove);
	position += glm::vec3(stepPlayer, 0.0f);
	physics::ClampInsideWalk(walk, sprite, position);
	sprite->SetPosition(position);
	playerVelocity = (physicsDt > 0.0f) ? (stepPlayer / physicsDt) : glm::vec2{ 0.0f };

	if (playerSelected && hasClickTarget) {
		const float intended = glm::length(desiredMove);
		const float moved = glm::length(stepPlayer);

		const glm::vec2 prevPos2 = glm::vec2(position.x, position.y) - stepPlayer;
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
}
