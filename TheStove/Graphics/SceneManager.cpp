#include "SceneManager.h"
#include <iostream>

// ---- canvas / room ----
static constexpr float kWorldW = 1200.0f;
static constexpr float kWorldH = 800.0f;

static constexpr float kWalkL = 144.0f;  // adjust left
static constexpr float kWalkR = 1114.0f; // adjust right
static constexpr float kWalkT = 84.0f;   // adjust top
static constexpr float kWalkB = 716.0f;  // adjust bottom

// Thickness of our blocking bars (thin = precise, easy to tune)
static constexpr float kEdgeThick = 6.0f;

// ---- wooden divider (vertical) ----
// X span of the wood (covering the planks). Tune to match your sprite.
static constexpr float kWoodX0 = 580.0f;   // left of wood
static constexpr float kWoodX1 = 598.0f;   // right of wood

// TOP solid segment
static constexpr float kWoodTopMinY = 50.0f;
static constexpr float kWoodTopMaxY = 250.0f;

// GAP (walk-through) — ~160 px tall
static constexpr float kWoodGapMinY = 250.0f;
static constexpr float kWoodGapMaxY = 500.0f;

// BOTTOM solid segment
static constexpr float kWoodBotMinY = 500.0f;
static constexpr float kWoodBotMaxY = 700.0f;

// Simple point-in-AABB using center+scale (matches your sprite usage)
static inline bool PointInsideCenterAABB(glm::vec2 p, glm::vec3 center, glm::vec3 scale) {
	const float hx = scale.x * 0.5f;
	const float hy = scale.y * 0.5f;
	return (p.x >= center.x - hx && p.x <= center.x + hx &&
		p.y >= center.y - hy && p.y <= center.y + hy);
}


Scene::Scene(GraphicsEngine& engine) : graphicsEngine(engine) {}

void Scene::LoadScene(const std::string& sceneName) {

	// Test scene for now
	LoadTest();
}

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

void Scene::BuildLevelColliders() {
	coll::WalkArea walk{
		/*L*/ kWalkL, /*R*/ kWalkR,
		/*T*/ kWalkT, /*B*/ kWalkB,
		/*edgeThick*/ kEdgeThick
	};

	coll::WoodVertical wood{
		/*x0*/ kWoodX0, /*x1*/ kWoodX1,
		/*topMinY*/ kWoodTopMinY, /*topMaxY*/ kWoodTopMaxY,
		/*gapMinY*/ kWoodGapMinY, /*gapMaxY*/ kWoodGapMaxY, // no collider here
		/*botMinY*/ kWoodBotMinY, /*botMaxY*/ kWoodBotMaxY
	};

	mCollision.build(walk, wood);
}

void Scene::LoadTest() {
	// Set background image
	SetSceneBackground("../assets/Background.png");

	GameObject* player = SpawnSprite("../assets/mc_front.png",
		glm::vec3(400, 400, 0), //Position
		glm::vec2(128, 128));   //Scale

	if (player) {
		spriteID = player->GetID();
		std::cout << "Spawned sprite with ID: " << spriteID << std::endl;

		spriteScales[spriteID] = glm::vec3(128, 128, 1.0f); // initial scale to match sprite size
		spriteRotations[spriteID] = 0.0f;
		spritePositions[spriteID] = glm::vec3(400, 400, 0); // initial position

		player->SetScale({ 96.0f, 96.0f, 1.0f });     // visual size
		player->SetColliderSize({ 64.0f, 128.0f });    // tight hitbox
		player->SetColliderOffset({ -32.0f, 0.0f });    // nudge down slightly

	}
	else {
		spriteID = -1; //invalid
		std::cerr << "Failed to spawn sprite" << std::endl;
	}

	BuildLevelColliders();
}

void Scene::Update(float deltaTime, GLFWwindow* window) {

	inputManager.Update(window);

	const float rotationSpeed = 1.0f * deltaTime; // degrees per second
	float moveSpeed = 200.0f * deltaTime;

	if (spriteID < 0) return;

	GameObject* sprite = GetGameObjectByID(spriteID);

	if (!sprite) {
		std::cerr << "Sprite with ID " << spriteID << " not found" << std::endl;
		return;
	}

	glm::vec3& position = spritePositions[spriteID];
	glm::vec3& scale = spriteScales[spriteID];
	float& rotation = spriteRotations[spriteID];

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

	glm::vec2 desiredMove{ 0.0f, 0.0f };

	if (inputManager.IsKeyPressed(GLFW_KEY_W)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_back.png"));
		desiredMove.y -= moveSpeed; // up
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_S)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_front.png"));
		desiredMove.y += moveSpeed; // down
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_A)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideleft", "../assets/mc_sideleft.png"));
		desiredMove.x -= moveSpeed; // left
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_D)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideright", "../assets/mc_sideright.png"));
		desiredMove.x += moveSpeed; // right
	}

	// --- CLICK HANDLING ---
// 1) Select the player only if you click on them
// 2) If already selected, the next left click sets the move target
	if (inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		auto mp = inputManager.GetMousePosition();
		glm::vec2 mouse{ (float)mp.x, (float)mp.y };


		// Build the player's current selection box from center+scale
		// (You can swap 'scale' for a dedicated hitbox if you prefer tighter selection.)
		if (!playerSelected) {
			glm::vec2 csize = sprite->GetColliderSize();
			glm::vec2 coff = sprite->GetColliderOffset();
			glm::vec3 selCenter = position + glm::vec3(coff, 0.0f);
			glm::vec3 selScale = glm::vec3(csize, 1.0f);

			if (PointInsideCenterAABB(mouse, selCenter, selScale)) {
				playerSelected = true;     // selected; wait for destination click
				hasClickTarget = false;    // clear any old target
				stuckFrames = 0;
				// (Optional) visual feedback: change sprite tint or cursor here
			}
			// else: clicked somewhere else; ignore
		}
		else {
			// Player is selected → this click sets the destination
			clickTarget = mouse;
			hasClickTarget = true;
			stuckFrames = 0;
			// (Optional) drop a waypoint marker VFX here
		}
	}

	// --- CLICK-TO-MOVE: only if we have a target (and are selected) ---
	if (playerSelected && hasClickTarget) {
		glm::vec2 pos2(position.x, position.y);
		glm::vec2 toTarget = clickTarget - pos2;
		float dist = glm::length(toTarget);

		if (dist > 1.0f) {
			glm::vec2 dir = toTarget / dist;
			// use playerSpeed here so click movement is consistent regardless of frame dt scaling above
			desiredMove += dir * (playerSpeed * deltaTime);
		}
		else {
			hasClickTarget = false;    // reached target
			stuckFrames = 0;
		}
	}

	// (Optional) Right-click anywhere to deselect
	if (inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
		playerSelected = false;
		hasClickTarget = false;
		stuckFrames = 0;
	}


	coll::AABB startBox = coll::World::makeAABBFromCenter(position, scale);
	glm::vec2 allowed = mCollision.resolve(startBox, desiredMove);

	position.x += allowed.x;
	position.y += allowed.y;

	// --- Cancel pathing if we're stuck against a wall ---
	if (playerSelected && hasClickTarget) {
		// If we intended to move but barely moved, we might be blocked.
		const float intended = glm::length(desiredMove);
		const float moved = glm::length(allowed);

		// Also check that we didn't make progress toward the target
		glm::vec2 newPos2(position.x, position.y);
		float prevDist = glm::length(clickTarget - (newPos2 - glm::vec2(allowed.x, allowed.y)));
		float newDist = glm::length(clickTarget - newPos2);

		bool noProgress = (newDist >= prevDist - 0.25f); // didn’t get meaningfully closer
		bool barelyMoved = (moved <= 0.05f && intended > 0.0f);

		if (noProgress || barelyMoved) ++stuckFrames; else stuckFrames = 0;

		if (stuckFrames >= kStuckFramesToCancel) {
			hasClickTarget = false; // stop trying to walk through walls
			stuckFrames = 0;
		}
	}


	// Optional: clamp to overall screen (kept if you still want hard bounds)
	position.x = glm::clamp(position.x, 0.0f, 1200.0f);
	position.y = glm::clamp(position.y, 0.0f, 800.0f);

	// Push transforms back to the GameObject for rendering
	sprite->SetScale(scale);
	sprite->SetRotation(rotation, glm::vec3(0, 0, 1));
	sprite->SetPosition(position);
}



