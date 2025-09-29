#include "SceneManager.h"
#include <iostream>

// ---- canvas / room ----
static constexpr float kWorldW = 1200.0f;
static constexpr float kWorldH = 800.0f;

static constexpr float kWalkL = 144.0f;   // was 64; move right a bit to fix your left “leak”
static constexpr float kWalkR = 1116.0f; // adjust if right edge feels off
static constexpr float kWalkT = 84.0f;   // adjust top
static constexpr float kWalkB = 716.0f;  // adjust bottom

// Thickness of our blocking bars (thin = precise, easy to tune)
static constexpr float kEdgeThick = 6.0f;

// ---- wooden divider (vertical) ----
// X span of the wood (covering the planks). Tune to match your sprite.
static constexpr float kWoodX0 = 580.0f;   // left of wood
static constexpr float kWoodX1 = 598.0f;   // right of wood

// TOP solid segment
static constexpr float kWoodTopMinY = 84.0f;
static constexpr float kWoodTopMaxY = 250.0f;   // ↓ bring the top down a bit

// GAP (walk-through) — ~160 px tall
static constexpr float kWoodGapMinY = 300.0f;
static constexpr float kWoodGapMaxY = 500.0f;   // ↑ raise the bottom of the bottom piece

// BOTTOM solid segment
static constexpr float kWoodBotMinY = 500.0f;
static constexpr float kWoodBotMaxY = 716.0f;

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
	walls.clear();

	// ---- 4 inside edges (precise stop at the light-gray border) ----
	// Left (vertical bar just INSIDE the walkable area)
	walls.push_back({ { kWalkL - kEdgeThick, kWalkT }, { kWalkL, kWalkB } });

	// Right
	walls.push_back({ { kWalkR, kWalkT }, { kWalkR + kEdgeThick, kWalkB } });

	// Top
	walls.push_back({ { kWalkL, kWalkT - kEdgeThick }, { kWalkR, kWalkT } });

	// Bottom
	walls.push_back({ { kWalkL, kWalkB }, { kWalkR, kWalkB + kEdgeThick } });

	// ---- Wooden divider: solid TOP and BOTTOM, open GAP in the middle ----
	// Top solid piece
	walls.push_back({ { kWoodX0, kWoodTopMinY }, { kWoodX1, kWoodTopMaxY } });

	// Bottom solid piece
	walls.push_back({ { kWoodX0, kWoodBotMinY }, { kWoodX1, kWoodBotMaxY } });

	// NOTE: no collider for [kWoodGapMinY, kWoodGapMaxY] → walk-through
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

	}
	else {
		spriteID = -1; //invalid
		std::cerr << "Failed to spawn sprite" << std::endl;
	}

	//SpawnTriangle(glm::vec3(600, 400, 0), glm::vec3(100.0f), 180.0f);
	//SpawnTriangle(glm::vec3(300, 200, 0), glm::vec3(100.0f), 180.0f);

	BuildLevelColliders();

}

Scene::AABB Scene::MakeAABB(const glm::vec3& center, const glm::vec3& scale) const {
	// assume center-anchored sprite; half extents = scale.xy * 0.5
	glm::vec2 he = glm::vec2(scale.x * 0.5f, scale.y * 0.5f);
	return { glm::vec2(center.x, center.y) - he, glm::vec2(center.x, center.y) + he };
}

bool Scene::Overlaps(const AABB& a, const AABB& b) const {
	return !(a.max.x <= b.min.x || a.min.x >= b.max.x ||
		a.max.y <= b.min.y || a.min.y >= b.max.y);
}

glm::vec2 Scene::ResolveAgainstWalls(const AABB& startBox, glm::vec2 desiredDelta) const {
	// Move along X then Y, resolving penetration at each step
	glm::vec2 out = desiredDelta;

	// Step 1: X
	AABB movedX = startBox;
	movedX.min.x += out.x; movedX.max.x += out.x;
	for (const auto& w : walls) {
		if (Overlaps(movedX, w)) {
			if (out.x > 0) { // moving right; push left
				float pen = movedX.max.x - w.min.x;
				out.x -= pen;
				movedX.min.x -= pen; movedX.max.x -= pen;
			}
			else if (out.x < 0) { // moving left; push right
				float pen = w.max.x - movedX.min.x;
				out.x += pen;
				movedX.min.x += pen; movedX.max.x += pen;
			}
		}
	}

	// Step 2: Y
	AABB movedY = movedX;
	movedY.min.y += out.y; movedY.max.y += out.y;
	for (const auto& w : walls) {
		if (Overlaps(movedY, w)) {
			if (out.y > 0) { // moving down; push up (note: your Y+ is down)
				float pen = movedY.max.y - w.min.y;
				out.y -= pen;
				movedY.min.y -= pen; movedY.max.y -= pen;
			}
			else if (out.y < 0) { // moving up; push down
				float pen = w.max.y - movedY.min.y;
				out.y += pen;
				movedY.min.y += pen; movedY.max.y += pen;
			}
		}
	}

	return out;
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

	if (inputManager.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		auto mp = inputManager.GetMousePosition();
		clickTarget = glm::vec2((float)mp.x, (float)mp.y);
		hasClickTarget = true;
	}

	glm::vec2 desiredMove{ 0.0f, 0.0f };

	if (inputManager.IsKeyPressed(GLFW_KEY_LEFT)) {
		rotation -= rotationSpeed;
		if (rotation < 0.0f) rotation += 360.0f;

		std::cout << "Left key pressed: rotation = " << rotation << std::endl;
	}

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

	if (hasClickTarget) {
		glm::vec2 toTarget = clickTarget - glm::vec2(position.x, position.y);
		float dist = glm::length(toTarget);
		if (dist > 1.0f) {
			glm::vec2 dir = toTarget / dist;
			desiredMove += dir * moveSpeed;
		}
		else {
			hasClickTarget = false; // reached target
		}
	}

	AABB startBox = MakeAABB(position, scale);
	glm::vec2 allowed = ResolveAgainstWalls(startBox, desiredMove);

	position.x += allowed.x;
	position.y += allowed.y;

	// Optional: clamp to overall screen (kept if you still want hard bounds)
	position.x = glm::clamp(position.x, 0.0f, 1200.0f);
	position.y = glm::clamp(position.y, 0.0f, 800.0f);

	// Push transforms back to the GameObject for rendering
	sprite->SetScale(scale);
	sprite->SetRotation(rotation, glm::vec3(0, 0, 1));
	sprite->SetPosition(position);
}



