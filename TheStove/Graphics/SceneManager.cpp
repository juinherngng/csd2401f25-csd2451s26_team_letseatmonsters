/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Implements the SceneManager class, which is responsible for the high-level
					management, coordination, and per-frame updating of all entities, systems,
					and game logic within a scene.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <fstream>
#include <glm/ext/matrix_clip_space.hpp>
#include <iostream>
#include <random>

#include "SceneManager.hpp"

namespace {
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
}

void Scene::SetSimulationActive(bool active) {
	simulationActive = active;

	if (active) {
		animationManager.Play();
	}
	else {
		animationManager.Stop();
	}
}

bool Scene::IsSimulationActive() const {
	return simulationActive;
}

const std::string& Scene::GetObjectTexturePath(int id) const {
	return entityManager.GetTexturePath(id);
}

void Scene::SetObjectTexturePath(int id, const std::string& path) {
	entityManager.SetTexturePath(id, path);
}

// Core Lifecycle
Scene::Scene(GraphicsEngine& engine, InputManager& inputMgr, AnimationManager& animMgr,
			 MovementManager& moveMgr, PhysicsManager& physicsMgr, CollisionManager& collisionMgr)
	: graphicsEngine(engine), inputManager(inputMgr), animationManager(animMgr),
	movementManager(moveMgr), physicsManager(physicsMgr), collisionManager(collisionMgr) {
	// Set the EntityManager reference in AnimationManager
	animationManager.SetEntityManager(&entityManager);

	// Basic default layer used when no explicit layer name is given
	AddLayer("1");
}

void Scene::LoadScene(const std::string& sceneName) {
	(void)sceneName;

	// Remember which level JSON we're using
	currentLevelPath_ = "../levels/kitchen01.json";

	// Optional: just pre-fill the path field for convenience
	mLevelEditor.SetPath("../levels/kitchen01.json");

	// Ensure we start EMPTY per rubric (no auto-spawned objects)
	ClearAll();

	// You can keep a background even with an empty level (or move this into JSON later)
	SetSceneBackground("../assets/Background_Full.png");
}

void Scene::Update(float deltaTime, GLFWwindow* window) {
	UpdateAnimationControls();

	// Deferred Clear
	if (pendingClear_) {
		ClearAll();
		RebuildColliders();
		pendingClear_ = false;
		return;  // Skip rest of update this frame
	}

	// Process input commands (debug toggles, force toggle, etc.)
	inputCommandHandler.ProcessCommands(inputManager, physicsManager, movementManager, spriteID, useForces_, showAuxDebug_);

	// Level editor toggle
	if (inputManager.IsKeyJustPressed(GLFW_KEY_L)) {
		mLevelEditor.Toggle();
	}

	// Resolve physics timestep
	const float physicsDt = physicsStep_.resolveDt(inputManager, deltaTime);
	lastPhysicsDt_ = physicsDt;

	if (simulationActive) {
		// Run all scripts
		logicManager.StartAll(*this);
		logicManager.UpdateAll(deltaTime, *this, inputManager);

		if (useForces_) {
			physicsManager.UpdatePhysics(physicsDt, entityManager, inputManager);
		}

		// Update NPC AI
		const collision::WalkArea walk = GetWalkArea();
		npcSystem.Update(physicsDt, entityManager, collisionManager, walk);

		// Handle player-NPC collisions
		HandlePlayerCollisions(physicsDt, entityManager);

		// Apply final constraints (gates, boundaries)
		ApplyFinalConstraints(entityManager);
	}

	// Debug visualization
	debugVisualizer.DrawDebugInfo(entityManager, collisionManager, movementManager, spriteID, showAuxDebug_);

	(void)window;
}

void Scene::ResetResizeBaseline() {
	resetBaseline_ = true;
}

void Scene::DrawUI() {
	if (mLevelEditor.IsEnabled()) {
		mLevelEditor.DrawUI(*this);
	}
}

void Scene::ClearAll() {
	logicManager.Clear(*this);  // <-- clear scripts first
	entityManager.Clear();
	animationManager.Clear();
	movementManager.Clear();
	npcSystem.Clear();

	spriteID = -1;
	dinoID = -1;
	otherID = -1;
	otherID2 = -1;
}

void Scene::SetPlayerID(int id) {
	spriteID = id;
}

GameObject* Scene::SpawnStaticSprite(const std::string& texturePath,
									 const glm::vec3 position,
									 const glm::vec2 size,
									 const std::string& layer) {
	GameObject* obj = entityManager.SpawnStaticSprite(texturePath, position, size);

	if (obj) {
		int id = obj->GetID();
		AssignObjectToLayer(id, layer);
	}

	return obj;
}

GameObject* Scene::SpawnAnimatedSprite(
	const std::string& texturePath,
	const glm::vec3 position,
	const glm::vec2 size,
	const std::vector<glm::vec4> frames,
	float frameDuration, bool loop,
	const std::string& layer) {
	GameObject* obj = entityManager.SpawnAnimatedSprite(texturePath, position, size, frames, frameDuration, loop);

	if (obj) {
		int id = obj->GetID();
		AssignObjectToLayer(id, layer);
	}

	return obj;
}

GameObject* Scene::GetGameObjectByID(int targetID) {
	return entityManager.GetByID(targetID);
}

void Scene::DespawnByID(int targetID) {
	// Remove from entity manager (handles transforms too)
	entityManager.DespawnByID(targetID);
}

void Scene::CollectRenderablePointers(std::vector<GameObject*>& out) {
	out.clear();

	std::vector<GameObject*> all = entityManager.GetAllObjects();
	out.reserve(all.size());

	for (GameObject* g : all) {
		if (!g) {
			continue;
		}

		// Check the layer's visibility flag
		const std::string layerName = GetObjectLayer(g->GetID());
		Layer* layer = GetLayer(layerName);
		if (layer && !layer->IsVisible()) {
			continue;
		}

		out.push_back(g);
	}

	auto parseLayerNumber = [](const std::string& s) -> int {
		if (s.empty()) {
			return 1; // base layer
		}

		int result = 0;
		for (char c:s) {
			if (!std::isdigit(static_cast<unsigned char>(c))) {
				// Any non-numeric layer name behaves like a very "high" layer
				// so that it draws on top of numeric layers.
				return 1000000;
			}

			result = result * 10 + (c - '0');
		}

		return result;
	};

	std::sort(
		out.begin(),
		out.end(),
		[&](GameObject* a, GameObject* b) {
		const std::string laName = GetObjectLayer(a->GetID());
		const std::string lbName = GetObjectLayer(b->GetID());

		int la = parseLayerNumber(laName);
		int lb = parseLayerNumber(lbName);

		// Different layers: smaller layer number drawn first
		if (la != lb) {
			return la > lb;
		}

		// Same layer - higher Y drawn first (lower on screen appears in front)
		return a->GetPosition().y > b->GetPosition().y;
	}
	);
}

std::vector<GameObject*> Scene::GetAllObjectsRaw() {
	return entityManager.GetAllObjects();
}

// Scene / Transform Utilities
void Scene::SetSceneBackground(const std::string& texturePath) {
	graphicsEngine.SetBackground(texturePath);
}

void Scene::SetTransformFromLevel(int id, const glm::vec3& pos, const glm::vec3& scale, float rotation) {
	entityManager.SetPosition(id, pos);
	entityManager.SetScale(id, scale);
	entityManager.SetRotation(id, rotation);

	GameObject* obj = GetGameObjectByID(id);
	if (obj) {
		obj->SetPosition(pos);
		obj->SetScale(scale);
		obj->SetRotation(rotation, glm::vec3(0, 0, 1));
	}
}

// Animation
bool Scene::HasAnimations(int id) const {
	return animationManager.HasAnimator(id);
}

std::vector<std::string> Scene::GetAnimationList(int id) const {
	return animationManager.GetAnimationNames(id);
}

std::string Scene::GetCurrentAnimationName(int id) const {
	return animationManager.GetCurrentAnimation(id);
}

void Scene::SetAnimation(int objID, const std::string& animName) {
	animationManager.SetAnimation(objID, animName);
}

void Scene::AttachDinoAnimations(int objID) {
	animationManager.AttachDinoAnimations(objID);
}

void Scene::MarkAnimated(int id, bool state) {
	if (!state) {
		// Turning OFF animation: remove any per-object animation state
		if (GameObject* obj = GetGameObjectByID(id)) {
			// Ensure it renders the full texture as a static sprite
			obj->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
		}
	}
	else {
		// Turning ON: no-op here; AttachDinoAnimations() will populate maps.
		// (HasAnimations() will start returning true once frames are attached.)
	}
}

void Scene::GenerateStressTest(int objectCount) {
	std::cout << "[Scene] Generating stress test with " << objectCount << " objects...\n";

	// Random number generation setup
	std::random_device rd;
	std::mt19937 gen(rd());

	// Random position ranges 
	std::uniform_real_distribution<float> posX(50.0f, 1150.0f);
	std::uniform_real_distribution<float> posY(50.0f, 750.0f);

	// Random velocity ranges
	std::uniform_real_distribution<float> velX(-100.0f, 100.0f);
	std::uniform_real_distribution<float> velY(-100.0f, 100.0f);

	// Random size range
	std::uniform_real_distribution<float> sizeRand(24.0f, 64.0f);

	// Textures to render
	std::vector<std::string> texturePaths = {
		"../assets/goat_sprite_front.png",
		"../assets/mc_sprite_front.png"
	};

	std::uniform_int_distribution<size_t> texIndex(0, texturePaths.size() - 1);

	// Spawn objects with random properties
	for (int i = 0; i < objectCount; i++) {
		// Random position
		glm::vec3 randomPos(posX(gen), posY(gen), 0.0f);

		// Random size
		float size = sizeRand(gen);
		glm::vec2 randomSize(size, size);

		// Random texture
		std::string texPath = texturePaths[texIndex(gen)];

		// Spawn using EntityManager
		GameObject* obj = entityManager.SpawnStaticSprite(texPath, randomPos, randomSize);

		if (obj) {
			// Set random velocity 
			Math::Vector2D randomVel(velX(gen), velY(gen));
			obj->SetVelocity(randomVel);
		}
	}
	std::cout << "[Scene] Stress test loaded\n";
}

void Scene::RequestClearAll() {
	pendingClear_ = true;
}

void Scene::AttachLogicForTag(int id, const std::string& tag) {
	if (tag == "player") {
		logicManager.AddLogic<PlayerLogic>(id);
		spriteID = id; // keep existing usage
	}
	else if (tag == "npc1" || tag == "npc2") {
		logicManager.AddLogic<SimpleNpcLogic>(id);
	}
	else if (tag == "dino") {
		logicManager.AddLogic<SimpleNpcLogic>(id);
		dinoID = id; // preserve your special ID if you rely on it elsewhere
	}
	// you can extend with more tags later
}

GraphicsEngine& Scene::GetGraphicsEngine() {
	return graphicsEngine;
}

const GraphicsEngine& Scene::GetGraphicsEngine() const {
	return graphicsEngine;
}

void Scene::AddLayer(const std::string& name) {
	layers.try_emplace(name, name); // Only add if missing
}

Layer* Scene::GetLayer(const std::string& name) {
	auto it = layers.find(name);
	return it != layers.end()?&(it->second):nullptr;
}

const std::unordered_map<std::string, Layer>& Scene::GetAllLayers() const {
	return layers;
}

std::string Scene::GetObjectLayer(int objectID) const {
	auto it = defaults_.find(objectID);
	if (it != defaults_.end()) {
		return it->second.layer;
	}

	return "";
}

void Scene::AssignObjectToLayer(int id, const std::string& newLayer) {
	std::string layerName = newLayer;
	if (layerName.empty()) layerName = "1";

	// Remove object from all layers' ID lists
	for (auto& pair : layers) {
		pair.second.RemoveObject(id);
	}

	// Register object ID with chosen layer (creates if missing)
	Layer& layer = layers[layerName];
	if (layer.GetName().empty()) {
		layer.SetName(layerName);
	}

	layer.AddObject(id);

	// Store on metadata used by the editor + JSON
	defaults_[id].layer = layerName;
}

void Scene::RemoveLayer(const std::string& name) {
	auto it = layers.find(name);
	if (it != layers.end()) {
		layers.erase(it);
	}
}

void Scene::UpdateAnimationControls() {
	// Do nothing when simulation is paused
	if (!simulationActive) return;

	// If UI is capturing keyboard, do not change gameplay animation state
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard) return;

	// Edge detection for single key presses
	const bool k1 = inputManager.IsKeyJustPressed(GLFW_KEY_1);
	const bool k2 = inputManager.IsKeyJustPressed(GLFW_KEY_2);
	const bool k3 = inputManager.IsKeyJustPressed(GLFW_KEY_3);
	if (!k1 && !k2 && !k3) return;

	const std::string anim = k1?"IDLE":(k2?"WALK":"ATTACK");

	// Make these known dino IDs use the chosen animation (skip missing objects)
	const int dinoIDs[] = { 1, 2, 3 };
	for (int id:dinoIDs) {
		GameObject* obj = GetGameObjectByID(id);
		if (!obj) continue; // not present in scene

		// Ensure animator is attached
		if (!animationManager.HasAnimator(id)) {
			animationManager.AttachDinoAnimations(id);
		}

		animationManager.SetAnimation(id, anim);
		std::cout << "[Scene] Playing animation " << anim << " for dino id=" << id << "\n";
	}
}
