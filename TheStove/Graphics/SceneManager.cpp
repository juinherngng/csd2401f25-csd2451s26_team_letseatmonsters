#include "SceneManager.h"
#include <iostream>

// Room (overall render / collision reference size; used only for clamping at the end)
static constexpr float kWorldW = 1200.0f;
static constexpr float kWorldH = 800.0f;

// Walkable inner rectangle, tune these four values to match your background art precisely.
static constexpr float kWalkL = 148.0f;  // adjust left
static constexpr float kWalkR = 1078.0f; // adjust right
static constexpr float kWalkT = 84.0f;   // adjust top
static constexpr float kWalkB = 733.0f;  // adjust bottom

// Thickness of our blocking bars (thin = precise, easy to tune)
static constexpr float kEdgeThick = 3.0f;

// Wooden divider (vertical split)
// X-span of the divider—should cover the visible planks of the sprite/art.
static constexpr float kWoodX0 = 562.0f; // left of wood
static constexpr float kWoodX1 = 590.0f; // right of wood

// TOP solid segment (blocked)
static constexpr float kWoodTopMinY = 50.0f;
static constexpr float kWoodTopMaxY = 250.0f;

// Middle GAP (pass-through)
static constexpr float kWoodGapMinY = 250.0f;
static constexpr float kWoodGapMaxY = 500.0f;

// BOTTOM solid segment (blocked)
static constexpr float kWoodBotMinY = 500.0f;
static constexpr float kWoodBotMaxY = 700.0f;

// End-of-stage vertical gate (right side)
// Place two vertical bars just inside the right walk boundary.
static constexpr float kEndVX0 = 1100.0f; // left edge of the gate
static constexpr float kEndVX1 = 1132.0f; // right edge of the gate

// TOP solid segment (blocked)
static constexpr float kEndVTopMinY = 50.0f;
static constexpr float kEndVTopMaxY = 250.0f;

// Middle GAP (pass-through)
static constexpr float kEndVGapMinY = 250.0f;
static constexpr float kEndVGapMaxY = 500.0f;

// BOTTOM solid segment (blocked)
static constexpr float kEndVBotMinY = 500.0f;
static constexpr float kEndVBotMaxY = 700.0f;

// Quick hit-test for a point against a center-anchored AABB.
static inline bool PointInsideCenterAABB(glm::vec2 p, glm::vec3 center, glm::vec3 scale) {
	const float hx = scale.x * 0.5f;
	const float hy = scale.y * 0.5f;
	return (p.x >= center.x - hx && p.x <= center.x + hx &&
		p.y >= center.y - hy && p.y <= center.y + hy);
}

Scene::Scene(GraphicsEngine& engine) : graphicsEngine(engine) {}

void Scene::LoadScene(const std::string& sceneName) {
	(void)sceneName; // Suppress unused parameter warning
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

// Build level colliders (rim, divider, gate) into the collision world.
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

void Scene::LoadTest() {
	// Set background image
	SetSceneBackground("../assets/Background.png");

	GameObject* player = SpawnSprite("../assets/mc_sprite_front.png",
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
		player->SetColliderOffset({ 0.0f, 0.0f });    // nudge down slightly

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

	// Keyboard movement intent (WASD) + facing texture swap
	glm::vec2 desiredMove{ 0.0f, 0.0f };
	if (inputManager.IsKeyPressed(GLFW_KEY_W)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_sprite_back.png"));
		desiredMove.y -= moveSpeed; // up
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_S)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_sprite_front.png"));
		desiredMove.y += moveSpeed; // down
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_A)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideleft", "../assets/mc_sprite_left.png"));
		desiredMove.x -= moveSpeed; // left
	}
	if (inputManager.IsKeyPressed(GLFW_KEY_D)) {
		sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideright", "../assets/mc_sprite_right.png"));
		desiredMove.x += moveSpeed; // right
	}

	// Click-to-Move behaviour
	// 1) If player is NOT selected: click must hit the player's collider to select.
	// 2) If already selected: the next left click sets the destination target.
	if (inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		auto mp = inputManager.GetMousePosition();
		glm::vec2 mouse{
			(float)mp.x, (float)mp.y
		};

		if (!playerSelected) {
			// Use collider (size+offset) as the selection area
			glm::vec2 csize = sprite->GetColliderSize();
			glm::vec2 coff = sprite->GetColliderOffset();
			glm::vec3 selCenter = position + glm::vec3(coff, 0.0f);
			glm::vec3 selScale = glm::vec3(csize, 1.0f);

			if (PointInsideCenterAABB(mouse, selCenter, selScale)) {
				playerSelected = true;  // selected; wait for destination click
				hasClickTarget = false; // clear any old target
				stuckFrames = 0;
			}
			// else: clicked empty space; ignore
		}
		else {
			// Player already selected, set destination
			clickTarget = mouse;
			hasClickTarget = true;
			stuckFrames = 0;
			// (Optional) drop a waypoint marker VFX here
		}
	}

	// Only apply click-to-move if we have an active target and the player is selected
	if (playerSelected && hasClickTarget) {
		glm::vec2 pos2(position.x, position.y);
		glm::vec2 toTarget = clickTarget - pos2;
		float dist = glm::length(toTarget);

		if (dist > 0.0f) {
			float maxStep = playerSpeed * deltaTime;
			glm::vec2 step = (dist <= maxStep)
				? toTarget           // final step hits the point exactly
				: (toTarget / dist) * maxStep;
			desiredMove += step;	 // click intent adds to keyboard intent
		}
	}

	// Right-click anywhere to deselect/cancel
	if (inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
		playerSelected = false;
		hasClickTarget = false;
		stuckFrames = 0;
	}

	// Build current AABB from collider size/offset for collision resolution
	collision::AABB startBox = collision::World::makeAABBFromCenter(
		position + glm::vec3(sprite->GetColliderOffset(), 0.0f),
		glm::vec3(sprite->GetColliderSize(), 1.0f)
	);

	// Resolve desired movement against world walls (X then Y sweep)
	glm::vec2 allowed = mCollision.resolve(startBox, desiredMove);

	// Apply allowed motion
	position.x += allowed.x;
	position.y += allowed.y;

	// Stuck detection for click-to-move: stop trying if progress stalls
	if (playerSelected && hasClickTarget) {
		const float intended = glm::length(desiredMove);
		const float moved = glm::length(allowed);

		// Compare distance to target before/after this frame
		glm::vec2 newPos2(position.x, position.y);
		float prevDist = glm::length(clickTarget - (newPos2 - glm::vec2(allowed.x, allowed.y)));
		float newDist = glm::length(clickTarget - newPos2);

		bool noProgress = (newDist >= prevDist - 0.25f); // didn’t get meaningfully closer
		bool barelyMoved = (moved <= 0.05f && intended > 0.0f);

		if (noProgress || barelyMoved) ++stuckFrames;
		else stuckFrames = 0;

		if (stuckFrames >= kStuckFramesToCancel) {
			hasClickTarget = false; // abort pathing into walls
			stuckFrames = 0;
		}
	}

	// Optional overall clamp to screen bounds (kept for safety)
	position.x = glm::clamp(position.x, 0.0f, 1200.0f);
	position.y = glm::clamp(position.y, 0.0f, 800.0f);

	// Push transforms back to the GameObject for rendering
	sprite->SetScale(scale);
	sprite->SetRotation(rotation, glm::vec3(0, 0, 1));
	sprite->SetPosition(position);
}
