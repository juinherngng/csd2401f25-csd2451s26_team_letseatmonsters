/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu
					Vu Phan Hung, phanhung.vu@digipen.edu
					Ng Juin Herng, juinherng.ng@digipen.edu

 DESCRIPTION:		Implements the Scene class, which is responsible for the high-level
					management, coordination, and per-frame updating of all entities, systems,
					and game logic within a scene.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/MenuButtonLogic.hpp"
#include "../Core/PauseButtonLogic.hpp" 

#include "SceneManager.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <Core/RuntimeLevel.hpp>
#include <exception>
#include <fstream>
#include <glm/ext/matrix_clip_space.hpp>
#include <iostream>
#include <random>

#include <Core/RuntimeLevel.hpp>
#include "../Core/MenuButtonLogic.hpp"
#include "../Core/PauseButtonLogic.hpp"
#include "../Core/AudioManager.hpp"
#include "SceneManager.hpp"
#include "GraphicsEngine.hpp"

namespace {
	// If an object has no collider yet, initialise an AABB that matches its visual size.
	void InitDefaultCollider(GameObject* obj) {
		if (!obj) {
			return;
		}

		Math::Vector2D col = obj->GetColliderSize();
		// Don't overwrite artist / prefab data if collider already exists
		if (col.x > 0.0f && col.y > 0.0f) {
			return;
		}

		glm::vec3 s = obj->GetScaleGLM();
		obj->SetColliderSize(Math::Vector2D{ s.x, s.y });
		obj->SetColliderOffset(Math::Vector2D{ 0.0f, 0.0f });
	}
}

// Simulation control
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

// Texture metadata helpers
const std::string& Scene::GetObjectTexturePath(int id) const {
	return entityManager.GetTexturePath(id);
}

void Scene::SetObjectTexturePath(int id, const std::string& path) {
	entityManager.SetTexturePath(id, path);
}

// Construction / core lifecycle
Scene::Scene(GraphicsEngine& engine, InputManager& inputMgr, AnimationManager& animMgr,
	MovementManager& moveMgr, PhysicsManager& physicsMgr, CollisionManager& collisionMgr)
	: graphicsEngine(engine), inputManager(inputMgr), animationManager(animMgr),
	movementManager(moveMgr), physicsManager(physicsMgr), collisionManager(collisionMgr) {
	// Allow AnimationManager to find objects
	animationManager.SetEntityManager(&entityManager);
	physicsManager.SetScene(this);
	collisionManager.SetScene(this);

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
	SetSceneBackground("../assets/Background.png");
}

void Scene::Update(float deltaTime, GLFWwindow* window) {
#ifdef _DEBUG
	UpdateAnimationControls();
#endif

	// Drive transitioned cutscene every frame so transitions progress
	UpdateCutsceneTransitioned(deltaTime);

	if (pendingClear_) {
		ClearAll();
		RebuildColliders();
		pendingClear_ = false;
		return;
	}

	inputCommandHandler.ProcessCommands(inputManager, physicsManager, movementManager, spriteID, useForces_, showAuxDebug_);

	if (inputManager.IsKeyJustPressed(GLFW_KEY_L)) {
		mLevelEditor.Toggle();
	}

#ifndef _DEBUG

	// Toggle FPS display with F1 in Release
	if (inputManager.IsKeyJustPressed(GLFW_KEY_F1)) {
		showFPS_ = !showFPS_;
		if (showFPS_) {
			// Lazy-load a small font for FPS
			FontSystem::Font* f = ResourceManager::Instance().GetFont("fps_font");
			if (!f) {
				f = FontSystem::FontManager::Instance().LoadFont("fps_font", "../assets/Font/ToThePointRegular-n9y4.ttf", 48);
			}
			if (f) {
				fpsText_.SetFont(f);
				fpsText_.SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); // yellow for visibility
				fpsText_.SetScale(1.5f); // 1.5x size for better visibility
				fpsText_.SetPosition(glm::vec2(10.0f, 60.0f)); // Move down to avoid clipping
				fpsAccumTime_ = 0.0f;
				fpsAccumFrames_ = 0;
				fpsValue_ = 60; // Start with a visible value
				fpsText_.SetText(std::string("FPS: ") + std::to_string(fpsValue_));
			}
		}
	}

#endif

	const float physicsDt = physicsStep_.resolveDt(inputManager, deltaTime);
	lastPhysicsDt_ = physicsDt;

	// ALWAYS update logic (menu buttons need this even with simulation disabled)
	logicManager.StartAll(*this);
	logicManager.UpdateAll(deltaTime, *this, inputManager);

	// NEW: seat customers at tables once.
	customerManager_.Update(physicsDt, *this);

	if (simulationActive) {
		if (useForces_) {
			physicsManager.UpdatePhysics(physicsDt, entityManager, inputManager);
		}
		const collision::WalkArea walk = GetWalkArea();
		npcSystem.Update(physicsDt, entityManager, collisionManager, walk);
		HandlePlayerCollisions(physicsDt, entityManager);
		ApplyFinalConstraints(entityManager);
	}

	// Process deferred level load after logic iteration completes
	if (hasPendingLevel_) {
		if (!pendingLevelPath_.empty()) {
			if (!RuntimeLevel::LoadAndBuild(pendingLevelPath_, *this)) {
				std::cerr << "[Scene] Deferred level load failed: " << pendingLevelPath_ << std::endl;
			}
			else {
				RebuildColliders();
				SetSimulationActive(pendingLevelSimActive_);
				inputManager.ClearState(); // avoid stale click replay

				// If we are coming from a cutscene, fade in the new level now
				if (cutTrans_.fadeInAfterLoad) {
					auto& gfx = GetGraphicsEngine();

					// Ensure a fade is active; if not, start a fade-in-only transition
					if (!gfx.IsTransitionActive() || !gfx.IsAtBlackout()) {
						// outSeconds = 0 starts from the current frame, then we only fade in
						gfx.StartSceneTransition(0.1f, cutTrans_.inSeconds);
					}

					gfx.ContinueTransitionFadeIn();
					cutTrans_.fadeInAfterLoad = false;
				}
			}
		}
		hasPendingLevel_ = false;
		pendingLevelPath_.clear();
	}

	// Update runtime particles
	particleSystem_.Update(deltaTime, entityManager);

	debugVisualizer.DrawDebugInfo(entityManager, collisionManager, movementManager, spriteID, showAuxDebug_);
	(void)window;

#ifndef _DEBUG
	// Update FPS accumulator when enabled (release builds only)
	if (showFPS_) {
		fpsAccumTime_ += deltaTime;
		fpsAccumFrames_ += 1;
		if (fpsAccumTime_ >= fpsUpdateInterval_) {
			float avg = static_cast<float>(fpsAccumFrames_) / fpsAccumTime_;
			fpsValue_ = static_cast<int>(avg + 0.5f);
			fpsAccumTime_ = 0.0f;
			fpsAccumFrames_ = 0;
			fpsText_.SetText(std::string("FPS: ") + std::to_string(fpsValue_));
		}
	}
#endif

	// Handle ESC to toggle pause overlay in Release
#ifndef _DEBUG
	if (inputManager.IsKeyJustPressed(GLFW_KEY_ESCAPE)) {
		if (IsSimulationActive()) {
			// Only allow pause during gameplay (not in main menu)
			ShowPauseOverlay();
		}
	}
#endif
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
	// Clear scripts first so they no longer reference objects
	logicManager.Clear(*this);
	entityManager.Clear();
	animationManager.Clear();
	movementManager.Clear();
	npcSystem.Clear();
	customerManager_.Reset();
	//ClearMenuButtonTexts();

	spriteID = -1;
	dinoID = -1;
	otherID = -1;
	otherID2 = -1;
}

void Scene::RequestClearAll() {
	pendingClear_ = true;
}

// Engine accessors
GraphicsEngine& Scene::GetGraphicsEngine() {
	return graphicsEngine;
}

const GraphicsEngine& Scene::GetGraphicsEngine() const {
	return graphicsEngine;
}

// Spawning / object management
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

		InitDefaultCollider(obj);
	}

	// Disabled by default, controlled by JSON
	obj->EnableShadow(false);

	obj->SetShadowSize(glm::vec2(size.x * 0.8f, size.y * 0.33f)); // ellipse sized to sprite
	obj->SetShadowOffset(glm::vec2(0.0f, 55.0f));       // sit near feet (tweak per origin)
	obj->SetShadowOpacity(0.65f);

	return obj;
}

GameObject* Scene::SpawnAnimatedSprite(const std::string& texturePath,
	const glm::vec3 position,
	const glm::vec2 size,
	const std::vector<glm::vec4> frames,
	float frameDuration, bool loop,
	const std::string& layer) {
	GameObject* obj = entityManager.SpawnAnimatedSprite(texturePath, position, size, frames, frameDuration, loop);

	if (obj) {
		int id = obj->GetID();
		AssignObjectToLayer(id, layer);

		InitDefaultCollider(obj);
	}

	// Disabled by default, controlled by JSON
	obj->EnableShadow(false);

	obj->SetShadowSize(glm::vec2(size.x * 0.8f, size.y * 0.33f)); // ellipse sized to sprite
	obj->SetShadowOffset(glm::vec2(0.0f, 55.0f));       // sit near feet (tweak per origin)
	obj->SetShadowOpacity(0.65f);

	return obj;
}

GameObject* Scene::SpawnStaticSpriteAtSamePos(int ownerID,
	const std::string& texturePath,
	float width,
	float height,
	const std::string& layer) {
	GameObject* owner = GetGameObjectByID(ownerID);
	if (!owner)
		return nullptr;

	glm::vec3 pos = owner->GetPositionGLM();

	GameObject* obj = SpawnStaticSprite(
		texturePath,
		glm::vec3(pos.x, pos.y, pos.z),
		glm::vec2(width, height),
		layer
	);
	if (!obj)
		return nullptr;

	const int id = obj->GetID();

	SetObjectTexturePath(id, texturePath);

	obj->SetColliderSize(Math::Vector2D(width, height));
	obj->SetColliderOffset(Math::Vector2D(0.0f, 0.0f));

	Scene::Defaults defs{};
	defs.pos = glm::vec3(pos.x, pos.y, pos.z);
	defs.size = glm::vec2(width, height);
	defs.rot = 0.0f;
	defs.colSize = glm::vec2(width, height);
	defs.colOff = glm::vec2(0.0f, 0.0f);
	defs.vel = glm::vec2(0.0f, 0.0f);
	defs.texture = texturePath;
	defs.tag = "ingredient";      // feel free to use something else
	defs.layer = layer;
	SetDefaults(id, defs);

	ClampToWalkArea(obj);

	return obj;
}


GameObject* Scene::GetGameObjectByID(int targetID) {
	return entityManager.GetByID(targetID);
}

std::vector<GameObject*> Scene::GetAllObjectsRaw() {
	return entityManager.GetAllObjects();
}

void Scene::DespawnByID(int targetID) {
	// Play destroy audio before removing the object
	PlayDestroyAudio(targetID);

	logicManager.RemoveAllFor(targetID, *this);

	objectTags_.erase(targetID);

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
		if (layer) {
			if (!layer->IsEnabled()) {
				continue;
			}
			if (!layer->IsVisible()) {
				continue;
			}
		}

		out.push_back(g);
	}

	// Helper to convert layer name to sort key
	auto parseLayerNumber = [](const std::string& s) -> int {
		if (s.empty()) {
			return 1; // base layer
		}

		int result = 0;
		for (char c : s) {
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

// Scene / Transform Utilities
void Scene::SetSceneBackground(const std::string& texturePath) {
	graphicsEngine.SetBackground(texturePath);
}

void Scene::SetTransformFromLevel(int id,
	const glm::vec3& pos,
	const glm::vec3& scale,
	float rotationDeg) {
	// Convert degrees to radians ONCE here
	const float rotationRad = rotationDeg * 3.14159265358979323846f / 180.0f;

	entityManager.SetPosition(id, pos);
	entityManager.SetScale(id, scale);
	entityManager.SetRotation(id, rotationRad);

	GameObject* obj = GetGameObjectByID(id);
	if (obj) {
		obj->SetPosition(pos);
		obj->SetScale(scale);
		obj->SetRotation(rotationRad, glm::vec3(0.0f, 0.0f, 1.0f));
	}
}

// Animation helpers
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

void Scene::AttachMenuAnimations(int objID) {
	animationManager.AttachMenuAnimations(objID);
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

// Tag-based logic helpers
void Scene::AttachLogicForTag(int id, const std::string& tag) {
	// ALWAYS wipe old logic from this object
	logicManager.RemoveAllFor(id, *this);

	std::cout << "[Scene] AttachLogicForTag id=" << id
		<< " tag='" << tag << "'\n";

	// Add only the logic that matches the new tag
	if (tag == "player") {
		logicManager.AddLogic<PlayerLogic>(id);
		spriteID = id;
		animationManager.AttachPlayerAnimations(id);
	}
	else if (tag == "npc1" || tag == "npc2") {
		logicManager.AddLogic<SimpleNpcLogic>(id);
	}
	else if (tag == "dino") {
		logicManager.AddLogic<SimpleNpcLogic>(id);
		dinoID = id;
	}
	else if (tag == "table") {
		logicManager.AddLogic<TableLogic>(id);
	}
	else if (tag == "work_table") {
		logicManager.AddLogic<WorkTableLogic>(id);
	}
	else if (tag == "customer_table") {
		logicManager.AddLogic<CustomerTableLogic>(id);
	}
	else if (tag == "ingredient_box") {
		logicManager.AddLogic<IngredientBoxLogic>(id);
	}
	else if (tag == "plate_box") {
		logicManager.AddLogic<IngredientBoxLogic>(id);
	}
	// Menu buttons etc
	else if (tag == "btn_play") {
		auto* logic = logicManager.AddLogic<MenuButtonLogic>(id, "../levels/kitchen01.json", true);
		if (logic && audioManager_) {
			logic->SetAudioManager(audioManager_);
		}
	}
	else if (tag == "btn_howtoplay") {
		logicManager.AddLogic<HowToPlayButtonLogic>(id);
	}
	else if (tag == "btn_quit") {
		logicManager.AddLogic<PauseButtonLogic>(id, PauseAction::Quit);
	}
}

void Scene::SetObjectTag(int id, const std::string& tag) {
	objectTags_[id] = tag;
}

std::string Scene::GetObjectTag(int id) const {
	auto it = objectTags_.find(id);
	if (it != objectTags_.end()) {
		return it->second;
	}

	// Fallback: if not stored, try defaults (still not hardcoding IDs)
	auto defIt = defaults_.find(id);
	if (defIt != defaults_.end() && !defIt->second.tag.empty()) {
		return defIt->second.tag;
	}

	return ""; // unknown/untagged
}

bool Scene::TagUsesVelocity(const std::string& tag) const {
	return (tag == "npc1" || tag == "npc2" || tag == "dino");
}

void Scene::ApplyTagRules(int id, const std::string& tag, float speedX, float speedY) {
	// Central place for special IDs (so the editor doesn't do string if-else)
	if (tag == "player") {
		SetPlayerID(id);
	}
	else if (tag == "npc1") {
		SetNPC1ID(id);
	}
	else if (tag == "npc2") {
		SetNPC2ID(id);
	}
	else if (tag == "dino") {
		SetDinoID(id);
	}

	// Only apply NPC velocity when this tag actually uses it
	if (TagUsesVelocity(tag)) {
		SetNPCVelocity(id, speedX, speedY);
	}
}

MovementManager& Scene::GetMovementManager() {
	return movementManager;
}

const MovementManager& Scene::GetMovementManager() const {
	return movementManager;
}

CollisionManager& Scene::GetCollisionManager() {
	return collisionManager;
}

const CollisionManager& Scene::GetCollisionManager() const {
	return collisionManager;
}

collision::World& Scene::GetCollisionWorld() {
	return collisionManager.GetCollisionWorld();
}

const collision::World& Scene::GetCollisionWorld() const {
	return collisionManager.GetCollisionWorld();
}

void Scene::AddLayer(const std::string& name) {
	layers.try_emplace(name, name); // Only add if missing
}

Layer* Scene::GetLayer(const std::string& name) {
	auto it = layers.find(name);
	return it != layers.end() ? &(it->second) : nullptr;
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

bool Scene::IsLayerEnabled(const std::string& layerName) const {
	auto it = layers.find(layerName);
	if (it == layers.end()) {
		return true;
	}
	return it->second.IsEnabled();
}

bool Scene::IsObjectLayerEnabled(int objectID) const {
	const std::string layerName = GetObjectLayer(objectID);
	if (layerName.empty()) {
		return true;
	}
	return IsLayerEnabled(layerName);
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

void Scene::QueueLevelLoad(const std::string& path, bool activateSimulation) {
	pendingLevelPath_ = path;
	pendingLevelSimActive_ = activateSimulation;
	hasPendingLevel_ = true;
}

void Scene::RequestStateChange(int newState) {
	pendingState_ = newState;
	hasPendingStateChange_ = true;
}

void Scene::ShowPauseOverlay() {
#ifndef _DEBUG
	if (pauseOverlayActive_) return;
	pauseOverlayActive_ = true;

	std::cout << "[Scene] ShowPauseOverlay()\n";

	// Pause simulation while overlay is active
	SetSimulationActive(false);

	const std::string uiLayer = "999999";

	// Pause overlay background
	if (GameObject* dim = SpawnStaticSprite("../assets/pause.png",
		{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f },
		{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) },
		uiLayer)) {
		pauseOverlayObjectIds_.push_back(dim->GetID());
		std::cout << "  [Scene] Pause background id=" << dim->GetID() << "\n";
	}

	auto spawnPauseBtn = [&](const char* tex, const glm::vec2& pos, PauseAction action) {
		if (GameObject* b = SpawnStaticSprite(tex, { pos.x, pos.y, 0.0f }, { 350.0f, 100.0f }, uiLayer)) {
			const int id = b->GetID();
			pauseOverlayObjectIds_.push_back(id);
			SetObjectTexturePath(id, tex);

			switch (action) {
			case PauseAction::Resume:
				logicManager.AddLogic<PauseButtonLogic>(id, PauseAction::Resume);
				std::cout << "  [Scene] Spawned Resume button id=" << id << " with PauseButtonLogic\n";
				break;

			case PauseAction::HowToPlay:
				logicManager.AddLogic<HowToPlayButtonLogic>(id);
				std::cout << "  [Scene] Spawned HowToPlay button id=" << id << " with HowToPlayButtonLogic\n";
				break;

			case PauseAction::Quit:
				logicManager.AddLogic<PauseButtonLogic>(id, PauseAction::Quit);
				std::cout << "  [Scene] Spawned Quit button id=" << id << " with PauseButtonLogic\n";
				break;
			}
		}
		else {
			std::cout << "  [Scene] ERROR: failed to spawn pause button for action=" << (int)action << "\n";
		}
		};

	spawnPauseBtn("../assets/continue_s.png", { 1300.f, 454.f }, PauseAction::Resume);
	spawnPauseBtn("../assets/how_s.png", { 1300.f, 584.f }, PauseAction::HowToPlay);
	spawnPauseBtn("../assets/quit_s.png", { 1300.f, 714.f }, PauseAction::Quit);
#endif
}



void Scene::HidePauseOverlay() {
#ifndef _DEBUG
	if (!pauseOverlayActive_) return;
	for (int id : pauseOverlayObjectIds_) {
		DespawnByID(id);
	}
	pauseOverlayObjectIds_.clear();
	pauseOverlayActive_ = false;
#endif
}

// text objects for menu buttons (disabled for now)
#if 0
void Scene::CreateMenuButtonTexts() {
	ClearMenuButtonTexts();

	// Get or load a font for menu buttons
	FontSystem::Font* font = ResourceManager::Instance().GetFont("menu_font");
	if (!font) {
		// Try to load a default font
		font = FontSystem::FontManager::Instance().LoadFont(
			"menu_font",
			"../assets/Font/ChrustyRock-ORLA.ttf",
			48
		);
		if (!font) {
			// Try alternative font
			font = FontSystem::FontManager::Instance().LoadFont(
				"menu_font",
				"../assets/Font/ToThePointRegular-n9y4.ttf",
				48
			);
		}
		if (!font) {
			std::cerr << "[Scene] Failed to load font for menu buttons\n";
			return;
		}
	}

	// Find menu button objects by their tags and create text for them
	const std::vector<std::pair<std::string, std::string>> buttonLabels = {
		{"btn_play", "PLAY"},
		{"btn_howtoplay", "HOW TO PLAY"},
		{"btn_quit", "QUIT"}
	};

	for (const auto& [tag, label] : buttonLabels) {
		// Find the button object with this tag
		std::vector<GameObject*> allObjects = entityManager.GetAllObjects();
		for (GameObject* obj : allObjects) {
			if (!obj) continue;

			// Check if this object has the matching tag
			Scene::Defaults defs = GetDefaults(obj->GetID());
			if (defs.tag == tag) {
				// Create text for this button
				MenuButtonText menuText;
				menuText.buttonID = obj->GetID();
				menuText.label = label;
				menuText.textObj.SetFont(font);
				menuText.textObj.SetText(label);

				// Get button position and size
				glm::vec3 btnPos = obj->GetPositionGLM();
				glm::vec3 btnSize = obj->GetScaleGLM();

				// Calculate actual text width and height using font glyph metrics when available
				FontSystem::Font* f = font;
				float textWidth = 0.0f;
				float maxHeight = 0.0f;
				if (f) {
					for (char c : label) {
						const FontSystem::Character* ch = f->GetCharacter(c);
						if (ch) {
							textWidth += static_cast<float>(ch->advance >> 6);
							maxHeight = std::max(maxHeight, static_cast<float>(ch->size.y));
						}
					}
				}

				// Fallback if metrics not available
				if (textWidth <= 0.0f) {
					float baseFontSize = 36.0f;
					textWidth = label.length() * baseFontSize * 0.4f;
					maxHeight = baseFontSize * 0.8f;
				}

				// Calculate scale to fit text within button bounds (with padding)
				float paddingW = 0.75f; // width padding
				float paddingH = 0.7f;  // height padding
				float targetW = btnSize.x * paddingW;
				float targetH = btnSize.y * paddingH;
				float scaleX = targetW / textWidth;
				float scaleY = targetH / maxHeight;
				float scale = std::min(scaleX, scaleY);

				// Final dimensions
				float finalWidth = textWidth * scale;
				float finalHeight = maxHeight * scale;

				// Position text centered on button (FontSystem renders from top-left baseline aware)
				float textX = btnPos.x - (finalWidth * 0.5f);
				float textY = btnPos.y - (finalHeight * 0.5f);

				menuText.textObj.SetPosition(glm::vec2(textX, textY));
				menuText.textObj.SetScale(scale);
				menuText.textObj.SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // White

				menuButtonTexts_.push_back(menuText);
				break; // Found the button for this tag
			}
		}
	}

	if (!menuButtonTexts_.empty()) {
		std::cout << "[Scene] Created text for " << menuButtonTexts_.size() << " menu buttons\n";
	}
}

void Scene::RenderMenuButtonTexts() {
	if (menuButtonTexts_.empty()) {
		return;
	}

	glm::mat4 projection = graphicsEngine.GetProjection();

	// Save current GL viewport so we can restore after drawing
	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	// If we're rendering into the scene FBO (non-default framebuffer), set viewport to FBO size
	GLint boundFBO = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundFBO);
	if (boundFBO != 0) {
		// Draw in FBO pixel coords (FBO matches reference canvas size)
		glViewport(0, 0, graphicsEngine.GetSceneWidth(), graphicsEngine.GetSceneHeight());
	}
	else {
		// We're rendering to the default framebuffer: apply the letterboxed viewport so positions match
		graphicsEngine.ApplyViewport();
	}

	// Disable depth test for text rendering and enable alpha blending
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Render each text object - positions are in reference space that matches projection
	for (auto& menuText : menuButtonTexts_) {
		FontSystem::TextRenderer::Instance().RenderText(menuText.textObj, projection);
	}

	// Restore GL state
	glDisable(GL_BLEND);
	// Restore previous viewport (default framebuffer expects full window viewport)
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}

void Scene::ClearMenuButtonTexts() {
	menuButtonTexts_.clear();
}

#endif

void Scene::RenderFPSText() {
#ifndef _DEBUG
	if (!showFPS_) {
		return;
	}

	static bool firstRender = true;
	if (firstRender) {
		std::cout << "[Scene] RenderFPSText() called for the first time" << std::endl;
		firstRender = false;
	}

	glm::mat4 projection = graphicsEngine.GetProjection();

	// Save current GL viewport so we can restore after drawing
	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	// If we're rendering into the scene FBO (non-default framebuffer), set viewport to FBO size
	GLint boundFBO = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundFBO);
	if (boundFBO != 0) {
		// Draw in FBO pixel coords (FBO matches reference canvas size)
		glViewport(0, 0, graphicsEngine.GetSceneWidth(), graphicsEngine.GetSceneHeight());
	}
	else {
		// We're rendering to the default framebuffer: apply the letterboxed viewport so positions match
		graphicsEngine.ApplyViewport();
	}

	// Disable depth test for text rendering and enable alpha blending
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Render FPS text
	FontSystem::TextRenderer::Instance().RenderText(fpsText_, projection);

	// Restore GL state
	glDisable(GL_BLEND);
	// Restore previous viewport (default framebuffer expects full window viewport)
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
#endif
}

// Audio binding playback helpers
void Scene::PlaySpawnAudio(int objectId) {
	if (!audioManager_) return;
	
	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) return;
	
	const Defaults& defs = it->second;
	if (defs.audioOnSpawn.empty()) return;
	
	// Check if sound exists and play it
	if (audioManager_->HasSound(defs.audioOnSpawn)) {
		// For looping audio, we need to handle it specially
		// The sound should have been loaded with loop flag from AudioCatalog
		audioManager_->PlaySound(defs.audioOnSpawn, 1.0f, false);
		std::cout << "[Scene] Playing spawn audio '" << defs.audioOnSpawn << "' for object " << objectId << std::endl;
	}
	else {
		std::cerr << "[Scene] Spawn audio '" << defs.audioOnSpawn << "' not found in AudioManager" << std::endl;
	}
}

void Scene::PlayInteractAudio(int objectId) {
	if (!audioManager_) return;
	
	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) return;
	
	const Defaults& defs = it->second;
	if (defs.audioOnInteract.empty()) return;
	
	if (audioManager_->HasSound(defs.audioOnInteract)) {
		audioManager_->PlaySound(defs.audioOnInteract, 1.0f, false);
		std::cout << "[Scene] Playing interact audio '" << defs.audioOnInteract << "' for object " << objectId << std::endl;
	}
	else {
		std::cerr << "[Scene] Interact audio '" << defs.audioOnInteract << "' not found in AudioManager" << std::endl;
	}
}

void Scene::PlayDestroyAudio(int objectId) {
	if (!audioManager_) return;
	
	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) return;
	
	const Defaults& defs = it->second;
	if (defs.audioOnDestroy.empty()) return;
	
	if (audioManager_->HasSound(defs.audioOnDestroy)) {
		audioManager_->PlaySound(defs.audioOnDestroy, 1.0f, false);
		std::cout << "[Scene] Playing destroy audio '" << defs.audioOnDestroy << "' for object " << objectId << std::endl;
	}
	else {
		std::cerr << "[Scene] Destroy audio '" << defs.audioOnDestroy << "' not found in AudioManager" << std::endl;
	}
}

void Scene::PlayProcessingAudio(int objectId) {
	if (!audioManager_) return;
	
	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) return;
	
	const Defaults& defs = it->second;
	if (defs.audioOnProcessing.empty()) return;
	
	if (audioManager_->HasSound(defs.audioOnProcessing)) {
		audioManager_->PlaySound(defs.audioOnProcessing, 1.0f, false);
		std::cout << "[Scene] Playing processing audio '" << defs.audioOnProcessing << "' for object " << objectId << std::endl;
	}
	else {
		std::cerr << "[Scene] Processing audio '" << defs.audioOnProcessing << "' not found in AudioManager" << std::endl;
	}
}

void Scene::StopProcessingAudio(int objectId) {
	if (!audioManager_) return;
	
	auto it = defaults_.find(objectId);
	if (it == defaults_.end()) return;
	
	const Defaults& defs = it->second;
	if (defs.audioOnProcessing.empty()) return;
	
	if (audioManager_->HasSound(defs.audioOnProcessing)) {
		audioManager_->StopSound(defs.audioOnProcessing);
		std::cout << "[Scene] Stopped processing audio '" << defs.audioOnProcessing << "' for object " << objectId << std::endl;
	}
}

void Scene::StopAllObjectAudio() {
	if (!audioManager_) return;
	
	// Stop all audio that was bound to objects
	for (const auto& [id, defs] : defaults_) {
		if (!defs.audioOnSpawn.empty() && audioManager_->HasSound(defs.audioOnSpawn)) {
			audioManager_->StopSound(defs.audioOnSpawn);
		}
		if (!defs.audioOnInteract.empty() && audioManager_->HasSound(defs.audioOnInteract)) {
			audioManager_->StopSound(defs.audioOnInteract);
		}
		if (!defs.audioOnDestroy.empty() && audioManager_->HasSound(defs.audioOnDestroy)) {
			audioManager_->StopSound(defs.audioOnDestroy);
		}
		if (!defs.audioOnProcessing.empty() && audioManager_->HasSound(defs.audioOnProcessing)) {
			audioManager_->StopSound(defs.audioOnProcessing);
		}
	}
	
	std::cout << "[Scene] Stopped all object-bound audio" << std::endl;
}

// Cutscene management

void Scene::StartCutscene(const std::vector<std::string>& imagePaths,
                          float holdSecondsPerImage,
                          float fadeSeconds,
                          const std::string& levelJsonPath,
                          bool activateSimulation) {
	if (imagePaths.empty()) {
		// If nothing to show, load level immediately
		QueueLevelLoad(levelJsonPath, activateSimulation);
		return;
	}

	// Clear any existing UI or pause overlays to avoid conflicts
	HidePauseOverlay();

	// Reset state
	CleanupCutsceneObjects();
	cutscene_.active = true;
	cutscene_.images = imagePaths;
	cutscene_.current = 0;
	cutscene_.holdTime = std::max(0.0f, holdSecondsPerImage);
	cutscene_.fadeTime = std::max(0.0f, fadeSeconds);
	cutscene_.t = 0.0f;
	cutscene_.phase = CutsceneState::Phase::FadeIn;
	cutscene_.targetLevelJson = levelJsonPath;
	cutscene_.targetActivateSim = activateSimulation;
	cutscene_.queuedFinalLoad = false;

	// Spawn the first sprite full-screen
	const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
	const glm::vec2 fullSize{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };

	if (GameObject* s = SpawnStaticSprite(cutscene_.images[0], center, fullSize, cutscene_.uiLayer)) {
		cutscene_.spriteA = s->GetID();
		// Detect alpha support by attempting to set alpha=0 then alpha=1
		// If shader ignores it, visuals won't change; we still run without fade.
		// Bring it in with fade-in
		SetSpriteAlpha(s, 0.0f);
	} else {
		// If spawn failed, abort cutscene and load level
		cutscene_.active = false;
		QueueLevelLoad(levelJsonPath, activateSimulation);
	}
}

void Scene::UpdateCutscene(float dt) {
	if (!cutscene_.active) return;

	// Helper to get object for id
	auto getObj = [&](int id) -> GameObject* { return GetGameObjectByID(id); };

	// Advance timers
	cutscene_.t += dt;

	switch (cutscene_.phase) {
	case CutsceneState::Phase::FadeIn: {
		// Fade in spriteA from 0 -> 1
		float alpha = (cutscene_.fadeTime > 0.0f) ? std::min(1.0f, cutscene_.t / cutscene_.fadeTime) : 1.0f;
		if (GameObject* a = getObj(cutscene_.spriteA)) {
			SetSpriteAlpha(a, alpha);
		}
		if (alpha >= 1.0f) {
			cutscene_.phase = CutsceneState::Phase::Hold;
			cutscene_.t = 0.0f;
		}
		break;
	}
	case CutsceneState::Phase::Hold: {
		if (cutscene_.t >= cutscene_.holdTime) {
			// Prepare next image if any
			if (cutscene_.current + 1 < cutscene_.images.size()) {
				// Spawn next spriteB on top (or cross-fade if supported)
				const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
				const glm::vec2 fullSize{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
				if (GameObject* b = SpawnStaticSprite(cutscene_.images[cutscene_.current + 1], center, fullSize, cutscene_.uiLayer)) {
					cutscene_.spriteB = b->GetID();
					// Start with alpha=0 to fade in
					SetSpriteAlpha(b, 0.0f);
					cutscene_.phase = CutsceneState::Phase::FadeOut;
					cutscene_.t = 0.0f;
				} else {
					// Could not spawn next; jump to end
					cutscene_.current = static_cast<size_t>(cutscene_.images.size());
					cutscene_.phase = CutsceneState::Phase::FadeOut;
					cutscene_.t = 0.0f;
				}
			} else {
				// Last image finished holding -> end cutscene and load level
				CleanupCutsceneObjects();
				cutscene_.active = false;
				if (!cutscene_.queuedFinalLoad) {
					cutscene_.queuedFinalLoad = true;
					QueueLevelLoad(cutscene_.targetLevelJson, cutscene_.targetActivateSim);
				}
			}
		}
		break;
	}
	case CutsceneState::Phase::FadeOut: {
		// Cross-fade: spriteA goes 1->0, spriteB goes 0->1
		float tNorm = (cutscene_.fadeTime > 0.0f) ? std::min(1.0f, cutscene_.t / cutscene_.fadeTime) : 1.0f;
		float alphaA = 1.0f - tNorm;
		float alphaB = tNorm;

		if (GameObject* a = getObj(cutscene_.spriteA)) SetSpriteAlpha(a, alphaA);
		if (GameObject* b = getObj(cutscene_.spriteB)) SetSpriteAlpha(b, alphaB);

		if (tNorm >= 1.0f) {
			// Despawn old A, promote B to A, advance index
			if (cutscene_.spriteA >= 0) DespawnByID(cutscene_.spriteA);
			cutscene_.spriteA = cutscene_.spriteB;
			cutscene_.spriteB = -1;
			cutscene_.current += 1;
			cutscene_.phase = CutsceneState::Phase::Hold;
			cutscene_.t = 0.0f;
		}
		break;
	}
	}
}

void Scene::CleanupCutsceneObjects() {
	if (cutscene_.spriteA >= 0) {
		DespawnByID(cutscene_.spriteA);
		cutscene_.spriteA = -1;
	}
	if (cutscene_.spriteB >= 0) {
		DespawnByID(cutscene_.spriteB);
		cutscene_.spriteB = -1;
	}
}

void Scene::SetSpriteAlpha(GameObject* obj, float alpha) {
	if (!obj) return;
	// If sprite shader supports a vertex color or uniform tint with alpha,
	// hook that here. As a safe no-op fallback, we reuse UV rect trick by
	// slightly shrinking when alpha ~0 to visually hide (not a real fade).
	// Replace with proper per-sprite color when available.
	if (alpha <= 0.01f) {
		// Hide by moving UV to a 0-sized rect (fallback).
		obj->SetUVRect({ 0.f, 0.f, 0.f, 0.f });
	} else {
		// Show full rect.
		obj->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
	}
}

// Call this from MenuButtonLogic on click
void Scene::StartCutsceneTransitioned(const std::vector<std::string>& imagePaths,
                                      const std::string& levelJsonPath,
                                      bool activateSimulation,
                                      float fadeOutSeconds,
                                      float fadeInSeconds,
                                      float holdSeconds) {
    if (imagePaths.empty()) {
        QueueLevelLoad(levelJsonPath, activateSimulation);
        return;
    }

#ifndef _DEBUG
    SetSimulationActive(false);
#endif
    HidePauseOverlay();

    if (cutTrans_.currentSpriteId >= 0) DespawnByID(cutTrans_.currentSpriteId);
    cutTrans_ = {};
    cutTrans_.active = true;
    cutTrans_.images = imagePaths;
    cutTrans_.index = 0;
    cutTrans_.targetLevelJson = levelJsonPath;
    cutTrans_.targetActivateSim = activateSimulation;
    cutTrans_.outSeconds = fadeOutSeconds;
    cutTrans_.inSeconds = fadeInSeconds;
    cutTrans_.holdSeconds = std::max(0.0f, holdSeconds);
    cutTrans_.holdElapsed = 0.0f;
    cutTrans_.holding = false;
    cutTrans_.awaitingBlackout = false;
    cutTrans_.awaitingInitialFadeIn = false;

    cutTrans_.useCrossfade = true;
    cutTrans_.crossfadeSeconds = fadeOutSeconds;
    cutTrans_.crossfadeFromIndex = 5;

	// Note: do not spawn the first image yet.
    // Start fade-out to blackout, will spawn at blackout and fade-in.
    auto& gfx = GetGraphicsEngine();
    gfx.StartSceneTransition(cutTrans_.outSeconds, cutTrans_.inSeconds);
    cutTrans_.awaitingBlackout = true;
}

void Scene::UpdateCutsceneTransitioned(float dt) {
    if (!cutTrans_.active) return;

    auto* gfx = &GetGraphicsEngine();
    if (!gfx) return;

    // Crossfade 5 -> 6 
    if (cutTrans_.useCrossfade && cutTrans_.crossfading) {
        cutTrans_.crossfadeT += dt;
        float tNorm = std::min(1.0f, cutTrans_.crossfadeT / cutTrans_.crossfadeSeconds);

        if (GameObject* a = GetGameObjectByID(cutTrans_.currentSpriteId)) SetSpriteAlpha(a, 1.0f - tNorm);
        if (GameObject* b = GetGameObjectByID(cutTrans_.nextSpriteId))    SetSpriteAlpha(b, tNorm);

        if (tNorm >= 1.0f) {
            if (cutTrans_.currentSpriteId >= 0) DespawnByID(cutTrans_.currentSpriteId);
            cutTrans_.currentSpriteId = cutTrans_.nextSpriteId;
            cutTrans_.nextSpriteId = -1;
            cutTrans_.crossfading = false;
            cutTrans_.holding = true;
            cutTrans_.holdElapsed = 0.0f;
        }
        return;
    }

    // Hold timing for subsequent transitions
    if (cutTrans_.holding && !gfx->IsTransitionActive()) {
        cutTrans_.holdElapsed += dt;
        if (cutTrans_.holdElapsed >= cutTrans_.holdSeconds) {
            const size_t nextIndex = cutTrans_.index + 1;
            if (nextIndex < cutTrans_.images.size()) {
                if (cutTrans_.useCrossfade && static_cast<int>(nextIndex) == cutTrans_.crossfadeFromIndex) {
                    const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
                    const glm::vec2 full{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
                    if (GameObject* b = SpawnStaticSprite(cutTrans_.images[nextIndex], center, full, cutTrans_.uiLayer)) {
                        cutTrans_.nextSpriteId = b->GetID();
                        SetSpriteAlpha(b, 0.0f);
                        cutTrans_.crossfading = true;
                        cutTrans_.crossfadeT = 0.0f;
                        cutTrans_.index = nextIndex;
                        cutTrans_.holding = false;
                    } else {
                        gfx->StartSceneTransition(cutTrans_.outSeconds, cutTrans_.inSeconds);
                        cutTrans_.awaitingBlackout = true;
                        cutTrans_.holding = false;
                        cutTrans_.holdElapsed = 0.0f;
                    }
                } else {
                    gfx->StartSceneTransition(cutTrans_.outSeconds, cutTrans_.inSeconds);
                    cutTrans_.awaitingBlackout = true;
                    cutTrans_.holding = false;
                    cutTrans_.holdElapsed = 0.0f;
                }
            } else {
                gfx->StartSceneTransition(cutTrans_.outSeconds, cutTrans_.inSeconds);
                cutTrans_.awaitingBlackout = true;
                cutTrans_.holding = false;
            }
        }
    }

    // Blackout handoff: spawn first image or next image, then fade-in and hold
    if (cutTrans_.awaitingBlackout && gfx->IsAtBlackout()) {
        cutTrans_.awaitingBlackout = false;

        // First image case: index==0 and nothing spawned yet
        if (cutTrans_.currentSpriteId < 0 && cutTrans_.index == 0) {
            const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
            const glm::vec2 full{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
            if (GameObject* s = SpawnStaticSprite(cutTrans_.images[0], center, full, cutTrans_.uiLayer)) {
                cutTrans_.currentSpriteId = s->GetID();
            }
            gfx->ContinueTransitionFadeIn();
            cutTrans_.holding = true;
            cutTrans_.holdElapsed = 0.0f;
            return;
        }

        const size_t nextIndex = cutTrans_.index + 1;
        if (nextIndex < cutTrans_.images.size()) {
            if (cutTrans_.currentSpriteId >= 0) DespawnByID(cutTrans_.currentSpriteId);

            const glm::vec3 center{ GraphicsEngine::kRefW * 0.5f, GraphicsEngine::kRefH * 0.5f, 0.0f };
            const glm::vec2 full{ static_cast<float>(GraphicsEngine::kRefW), static_cast<float>(GraphicsEngine::kRefH) };
            if (GameObject* s = SpawnStaticSprite(cutTrans_.images[nextIndex], center, full, cutTrans_.uiLayer)) {
                cutTrans_.currentSpriteId = s->GetID();
            }
            cutTrans_.index = nextIndex;

            gfx->ContinueTransitionFadeIn();
            cutTrans_.holding = true;
            cutTrans_.holdElapsed = 0.0f;
        } else {
            // Last blackout → load level, then fade-in after build (existing logic)
            if (cutTrans_.currentSpriteId >= 0) {
                DespawnByID(cutTrans_.currentSpriteId);
                cutTrans_.currentSpriteId = -1;
            }
            cutTrans_.active = false;
            QueueLevelLoad(cutTrans_.targetLevelJson, cutTrans_.targetActivateSim);
            cutTrans_.fadeInAfterLoad = true; // handled in Scene::Update after LoadAndBuild
        }
    }
}
