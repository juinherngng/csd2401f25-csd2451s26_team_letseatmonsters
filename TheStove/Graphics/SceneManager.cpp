/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (35%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(20%)
					Vu Phan Hung, phanhung.vu@digipen.edu   (15%)
					Ng Juin Herng, juinherng.ng@digipen.edu (30%)

 DESCRIPTION:		Implements the Scene class, which is responsible for the high-level
					management, coordination, and per-frame updating of all entities, systems,
					and game logic within a scene.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/AudioManager.hpp"
#include "../Core/FilePaths.hpp"
#include "../Core/LevelEditorPanelFonts.hpp"
#include "../Core/MessageBus.hpp"

#include "GraphicsEngine.hpp"
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
#include <unordered_set>

namespace {


	/**
	 * @brief Returns flow state name.
	 * @param state Parameter for state.
	 * @return Requested value.
	 */
	const char* FlowStateToString(Scene::FlowState state) {
		switch (state) {
		case Scene::FlowState::Bootstrapping: return "Bootstrapping";
		case Scene::FlowState::LoadingLevel: return "LoadingLevel";
		case Scene::FlowState::Transitioning: return "Transitioning";
		case Scene::FlowState::Cutscene: return "Cutscene";
		case Scene::FlowState::Gameplay: return "Gameplay";
		case Scene::FlowState::Paused: return "Paused";
		case Scene::FlowState::NonSimulation: return "NonSimulation";
		default: return "Unknown";
		}
	}
}

namespace {

	/**
	 * @brief Initializes default collider.
	 * @param obj Parameter for obj.
	 */
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


/**
 * @brief Sets simulation active.
 * @param active Parameter for active.
 * @return Result produced by this operation.
 */
void Scene::SetSimulationActive(bool active) {
	simulationActive = active;

	if (active) {
		animationManager.Play();
	}
	else {
		const std::string levelPath = GetCurrentLevelPath();
		const bool isMainMenu = levelPath.find("main_menu") != std::string::npos;

		if (isMainMenu) {
			animationManager.Play();
		}
		else {
			animationManager.Stop();
		}
	}
}

/**
 * @brief Returns whether simulation active.
 * @return True when the operation succeeds or the condition is met.
 */
bool Scene::IsSimulationActive() const {
	return simulationActive;
}

/**
 * @brief Returns object texture path.
 * @param id Parameter for id.
 * @return Requested value.
 */
const std::string& Scene::GetObjectTexturePath(int id) const {
	return entityManager.GetTexturePath(id);
}

/**
 * @brief Sets object texture path.
 * @param id Parameter for id.
 * @param path Path to process.
 * @return Result produced by this operation.
 */
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
	SetFlowState(FlowState::Bootstrapping);
}

/**
 * @brief Returns flow state name.
 * @return Requested value.
 */
const char* Scene::GetFlowStateName() const {
	return FlowStateToString(flowState_);
}

/**
 * @brief Updates this object.
 * @param deltaTime Frame delta time in seconds.
 * @param window Parameter for window.
 * @return Result produced by this operation.
 */
void Scene::Update(float deltaTime, GLFWwindow* window) {
#ifdef _DEBUG
	UpdateAnimationControls();
#endif

	UpdateCutscenePhase(deltaTime);
	if (!UpdateInputPhase(deltaTime)) {
		return;
	}

	const float physicsDt = physicsStep_.resolveDt(inputManager, deltaTime);
	lastPhysicsDt_ = physicsDt;

	UpdateSimulationPhase(deltaTime, physicsDt);
	HandleDeferredLoads();
	UpdateUiPhase(deltaTime, window);
	FinalizeFramePhase(deltaTime);
}

// Advances both cutscene state machines and level-transition state.
/**
 * @brief Updates cutscene phase.
 * @param deltaTime Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void Scene::UpdateCutscenePhase(float deltaTime) {

	// Drive both cutscene players every frame so transitions progress
	UpdateCutsceneTransitioned(deltaTime);
	UpdateCutscene(deltaTime);

	UpdateLevelTransition();
}

// Consumes input, toggles editor/FPS UI, and may clear the whole scene.
/**
 * @brief Updates input phase.
 * @param deltaTime Frame delta time in seconds.
 * @return Result produced by this operation.
 */
bool Scene::UpdateInputPhase(float deltaTime) {
#if defined(_DEBUG) && !defined(ENABLE_DEBUG_UI)
	(void)deltaTime;
#endif
	// Handle pending pause audio (pause channels after fade completes)
#ifndef _DEBUG
	if (pauseAudioPending_ && audioManager_) {
		pauseAudioTimer_ -= deltaTime;
		if (pauseAudioTimer_ <= 0.0f) {
			// Fade completed, now pause the channels to stop playback
			if (!pauseMusicChannel_.empty()) {
				audioManager_->PauseChannel(pauseMusicChannel_);
			}
			if (!pauseAmbienceChannel_.empty()) {
				audioManager_->PauseChannel(pauseAmbienceChannel_);
			}
			pauseAudioPending_ = false;
			std::cout << "[Scene] Paused audio channels after fade" << std::endl;
		}
	}
#endif

	if (pendingClear_) {
		ClearAll();
		RebuildColliders();
		pendingClear_ = false;
		return false;
	}

	// While any cutscene is active, discard input so UI/buttons cannot be pressed (this might need tweaking later, for future cutscenes that need input)
	if (IsAnyCutsceneActive()) {
		const bool spaceHeld = inputManager.IsKeyPressed(GLFW_KEY_SPACE);
		if (spaceHeld && !cutsceneSkipSpaceHeld_ && !cutsceneSkipConsumed_) {
			SkipActiveCutscene();
			cutsceneSkipConsumed_ = true;
		}
		cutsceneSkipSpaceHeld_ = spaceHeld;
		inputManager.ClearState();
	}
	else {
		cutsceneSkipSpaceHeld_ = false;
		cutsceneSkipConsumed_ = false;
#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
		inputCommandHandler.ProcessCommands(inputManager, physicsManager, movementManager, spriteID, useForces_, showAuxDebug_);
#endif
	}

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	if (inputManager.IsKeyJustPressed(GLFW_KEY_L)) {
		mLevelEditor.Toggle();
	}

#endif

#ifndef _DEBUG

	// Toggle FPS display with F1 in Release
	if (inputManager.IsKeyJustPressed(GLFW_KEY_F1)) {
		showFPS_ = !showFPS_;
		if (showFPS_) {
			// Lazy-load a small font for FPS
			FontSystem::Font* f = ResourceManager::Instance().GetFont("fps_font");
			if (!f) {
				f = FontSystem::FontManager::Instance().LoadFont("fps_font", FilePaths::Fonts::TO_THE_POINT, 48);
			}

			if (f) {
				fpsText_.SetFont(f);
				fpsText_.SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); // yellow for visibility
				fpsText_.SetScale(1.5f); // 1.5x size for better visibility
				// Position will be set dynamically in update loop to align to right side
				fpsAccumTime_ = 0.0f;
				fpsAccumFrames_ = 0;
				fpsValue_ = 60; // Start with a visible value
				std::string fpsStr = std::string("FPS: ") + std::to_string(fpsValue_);
				fpsText_.SetText(fpsStr);
				// Set initial position on the right side
				float estimatedTextWidth = static_cast<float>(fpsStr.length()) * 20.0f * fpsText_.GetScale();
				float rightPadding = 20.0f;
				float topPadding = 60.0f;
				fpsText_.SetPosition(glm::vec2(static_cast<float>(GraphicsEngine::kRefW) - estimatedTextWidth - rightPadding, topPadding));
			}
		}
	}

#endif

	return true;
}

// Updates game logic, hooks, forces/physics, NPC movement, and collision constraints.
/**
 * @brief Updates simulation phase.
 * @param deltaTime Frame delta time in seconds.
 * @param physicsDt Parameter for physics dt.
 * @return Result produced by this operation.
 */
void Scene::UpdateSimulationPhase(float deltaTime, float physicsDt) {
	// Always update logic (menu buttons need this even with simulation disabled)
	logicManager.StartAll(*this);
	logicManager.UpdateAll(deltaTime, *this, inputManager);

	// Seat customers at tables once
	if (customerUpdateHook_) {
		customerUpdateHook_(physicsDt, *this);
	}

	if (simulationActive && simulationUpdateHook_) {
		simulationUpdateHook_(deltaTime, *this);
	}

	if (simulationActive) {
		if (useForces_) {
			physicsManager.UpdatePhysics(physicsDt, entityManager, inputManager);
		}
		const collision::WalkArea walk = GetWalkArea();
		npcSystem.Update(physicsDt, entityManager, collisionManager, walk);
		HandlePlayerCollisions(physicsDt, entityManager);
		ApplyFinalConstraints(entityManager);

		// Update 3D audio listener position to the center of the reference canvas
		if (audioManager_) {
			float listenerX = static_cast<float>(GraphicsEngine::kRefW) * 0.5f;
			float listenerY = static_cast<float>(GraphicsEngine::kRefH) * 0.5f;
			audioManager_->SetListenerPosition(listenerX, listenerY, 0.0f);
		}
	}
}

// Advances particles and UI slide animations; may draw debug overlays.
/**
 * @brief Updates ui phase.
 * @param deltaTime Frame delta time in seconds.
 * @param window Parameter for window.
 * @return Result produced by this operation.
 */
void Scene::UpdateUiPhase(float deltaTime, GLFWwindow* window) {
	// Update runtime particles
	particleSystem_.Update(deltaTime, entityManager);

	// Update any UI slide-in animations regardless of simulation flag
	UpdateUiSlides(deltaTime);

	UpdateRuntimeAnimatedFx(deltaTime);
	UpdateFloatingWorldTextFx(deltaTime);

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	debugVisualizer.DrawDebugInfo(entityManager, collisionManager, movementManager, spriteID, showAuxDebug_);
#endif
	(void)window;
}

// Despawns queued entities, updates FPS text, and handles pause-overlay toggles.
/**
 * @brief Performs finalize frame phase.
 * @param deltaTime Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void Scene::FinalizeFramePhase(float deltaTime) {
#ifdef _DEBUG
	(void)deltaTime;
#endif
	for (int id : pendingDespawns_) {
		DespawnByID(id);
	}

	pendingDespawns_.clear();

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
			std::string fpsStr = std::string("FPS: ") + std::to_string(fpsValue_);
			fpsText_.SetText(fpsStr);

			// Position FPS text on the right side of the screen
			// Use reference canvas width (kRefW) since projection uses reference space
			float estimatedTextWidth = static_cast<float>(fpsStr.length()) * 20.0f * fpsText_.GetScale();
			float rightPadding = 20.0f;
			float topPadding = 60.0f;
			fpsText_.SetPosition(glm::vec2(static_cast<float>(GraphicsEngine::kRefW) - estimatedTextWidth - rightPadding, topPadding));
		}
	}
#endif

	// Handle ESC to toggle pause overlay in Release
#ifndef _DEBUG
	if (inputManager.IsKeyJustPressed(GLFW_KEY_ESCAPE)) {
		if (IsPauseOverlayActive()) {
			HidePauseOverlay();
			RequestResumeFromPauseOverlay();
			inputManager.ConsumeNextKeyPress(GLFW_KEY_ESCAPE);
		}
		else if (IsSimulationActive()) {
			// Only allow pause during gameplay (not in main menu)
			ShowPauseOverlay();
			inputManager.ConsumeNextKeyPress(GLFW_KEY_ESCAPE);
		}
	}
#endif

#ifndef _DEBUG
	if (resumeFromPausePending_ && !pauseOverlayActive_) {
		SetSimulationActive(true);
		resumeFromPausePending_ = false;
		RefreshFlowState();
	}
#endif

}

/**
 * @brief Resets resize baseline.
 * @return Result produced by this operation.
 */
void Scene::ResetResizeBaseline() {
	resetBaseline_ = true;
}

/**
 * @brief Draws ui.
 * @return Result produced by this operation.
 */
void Scene::DrawUI() {
	if (mLevelEditor.IsEnabled()) {
		mLevelEditor.DrawUI(*this);
	}
}

/**
 * @brief Returns graphics engine.
 * @return Requested value.
 */
GraphicsEngine& Scene::GetGraphicsEngine() {
	return graphicsEngine;
}

/**
 * @brief Returns graphics engine.
 * @return Requested value.
 */
const GraphicsEngine& Scene::GetGraphicsEngine() const {
	return graphicsEngine;
}

/**
 * @brief Sets player id.
 * @param id Parameter for id.
 * @return Result produced by this operation.
 */
void Scene::SetPlayerID(int id) {
	spriteID = id;
}

/**
 * @brief Performs spawn static sprite.
 * @param texturePath Parameter for texture path.
 * @param position Parameter for position.
 * @param size Parameter for size.
 * @param layer Parameter for layer.
 * @return Result produced by this operation.
 */
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

/**
 * @brief Performs spawn animated sprite.
 * @param texturePath Parameter for texture path.
 * @param position Parameter for position.
 * @param size Parameter for size.
 * @param frames Parameter for frames.
 * @param frameDuration Parameter for frame duration.
 * @param loop Parameter for loop.
 * @param layer Parameter for layer.
 * @return Result produced by this operation.
 */
GameObject* Scene::SpawnAnimatedSprite(const std::string& texturePath,
	const glm::vec3 position,
	const glm::vec2 size,
	const std::vector<glm::vec4>& frames,
	float frameDuration, bool loop,
	const std::string& layer) {
	GameObject* obj = entityManager.SpawnAnimatedSprite(texturePath, position, size, frames, frameDuration, loop);

	if (obj) {
		int id = obj->GetID();
		AssignObjectToLayer(id, layer);
		InitDefaultCollider(obj);

		// This is the missing step:
		animationManager.AttachRuntimeAnimation(id, frames, frameDuration, loop);
	}

	if (obj) {
		obj->EnableShadow(false);
		obj->SetShadowSize(glm::vec2(size.x * 0.8f, size.y * 0.33f));
		obj->SetShadowOffset(glm::vec2(0.0f, 55.0f));
		obj->SetShadowOpacity(0.65f);
	}

	return obj;
}

/**
 * @brief Performs spawn static sprite at same pos.
 * @param ownerID Parameter for owner id.
 * @param texturePath Parameter for texture path.
 * @param width Width value in pixels.
 * @param height Height value in pixels.
 * @param layer Parameter for layer.
 * @return Result produced by this operation.
 */
GameObject* Scene::SpawnStaticSpriteAtSamePos(int ownerID,
	const std::string& texturePath,
	float width,
	float height,
	const std::string& layer) {
	GameObject* owner = GetGameObjectByID(ownerID);
	if (!owner) {
		return nullptr;
	}

	glm::vec3 pos = owner->GetPositionGLM();

	GameObject* obj = SpawnStaticSprite(
		texturePath,
		glm::vec3(pos.x, pos.y, pos.z),
		glm::vec2(width, height),
		layer
	);
	if (!obj) {
		return nullptr;
	}

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

/**
 * @brief Returns game object by id.
 * @param targetID Parameter for target id.
 * @return Requested value.
 */
GameObject* Scene::GetGameObjectByID(int targetID) {
	return entityManager.GetByID(targetID);
}

/**
 * @brief Returns all objects raw.
 * @return Requested value.
 */
std::vector<GameObject*> Scene::GetAllObjectsRaw() {
	return entityManager.GetAllObjects();
}

/**
 * @brief Returns object storage raw.
 * @return Requested value.
 */
const std::vector<std::unique_ptr<GameObject>>& Scene::GetObjectStorageRaw() const {
	return entityManager.GetObjectStorage();
}

/**
 * @brief Performs despawn by id.
 * @param targetID Parameter for target id.
 * @return Result produced by this operation.
 */
void Scene::DespawnByID(int targetID) {
	// Play destroy audio before removing the object
	PlayDestroyAudio(targetID);

	// Remove stale layer membership and metadata before despawning.
	auto defaultsIt = defaults_.find(targetID);
	if (defaultsIt != defaults_.end()) {
		const std::string& layerName = defaultsIt->second.layer;
		if (!layerName.empty()) {
			auto layerIt = layers.find(layerName);
			if (layerIt != layers.end()) {
				layerIt->second.RemoveObject(targetID);
			}
		}

		defaults_.erase(defaultsIt);
	}

	logicManager.RemoveAllFor(targetID, *this);

	objectTags_.erase(targetID);
	mTexturePathByID.erase(targetID);

	animationManager.RemoveAnimator(targetID);

	// Remove from entity manager (handles transforms too)
	entityManager.DespawnByID(targetID);
}

/**
 * @brief Collects renderable pointers.
 * @param out Output value for out.
 * @return Result produced by this operation.
 */
void Scene::CollectRenderablePointers(std::vector<GameObject*>& out) {
	out.clear();
	const bool cutsceneActive = IsAnyCutsceneActive();

	const auto& all = entityManager.GetObjectStorage();
	out.reserve(all.size());

	for (const auto& objPtr : all) {
		GameObject* g = objPtr.get();
		if (!g) {
			continue;
		}

		// Skip if object is marked invisible in defaults (per-object visibility)
		const int objId = g->GetID();
		auto defIt = defaults_.find(objId);
		if (defIt != defaults_.end()) {
			if (!defIt->second.visible) {
				continue;
			}
		}

		const std::string layerName = (defIt != defaults_.end()) ? defIt->second.layer : "";

		// While cutscenes are active, render only cutscene/pause overlay layers.
		// This prevents gameplay objects/HUD strips from bleeding through in debug builds.
		if (cutsceneActive && layerName != cutTrans_.uiLayer && layerName != cutscene_.uiLayer && layerName != "999999") {
			continue;
		}

		// Check the layer's visibility flag
		Layer* layer = GetObjectLayerPtr(objId);
		if (layer) {
			if (!layer->IsEnabled()) {
				continue;
			}

			if (!layer->IsVisible()) {
				continue;
			}
		}

		// Set the render layer on the object for use in GraphicsEngine
		const std::string& texturePath = GetObjectTexturePath(objId);
		const bool isFootstepVfx = texturePath.find("run_vfx.png") != std::string::npos;
		g->SetRenderLayer(isFootstepVfx ? 0 : GetLayerSortKeyCached(layerName));

		out.push_back(g);
	}

	std::sort(
		out.begin(),
		out.end(),
		[&](GameObject* a, GameObject* b) {
			int la = a->GetRenderLayer();
			int lb = b->GetRenderLayer();

			// Different layers: smaller layer number drawn first (behind),
			// higher layer number drawn later (on top)
			if (la != lb) {
				return la < lb;
			}

			// Same layer: use explicit sort order first
			int sa = a->GetRenderSortOrder();
			int sb = b->GetRenderSortOrder();
			if (sa != sb) {
				return sa < sb;
			}

			// Same layer & same sort order - lower Y drawn first (higher on screen appears behind)
			return a->GetPosition().y < b->GetPosition().y;
		}
	);
}

/**
 * @brief Sets scene background.
 * @param texturePath Parameter for texture path.
 * @return Result produced by this operation.
 */
void Scene::SetSceneBackground(const std::string& texturePath) {
	sceneBackgroundPath_ = texturePath;
	graphicsEngine.SetBackground(texturePath);
}

/**
 * @brief Sets scene background overlay.
 * @param texturePath Parameter for texture path.
 * @return Result produced by this operation.
 */
void Scene::SetSceneBackgroundOverlay(const std::string& texturePath) {
	sceneBackgroundOverlayPath_ = texturePath;
	graphicsEngine.SetBackgroundOverlay(texturePath);
}

/**
 * @brief Clears scene background overlay.
 * @return Result produced by this operation.
 */
void Scene::ClearSceneBackgroundOverlay() {
	sceneBackgroundOverlayPath_.clear();
	graphicsEngine.ClearBackgroundOverlay();
}

/**
 * @brief Sets transform from level.
 * @param id Parameter for id.
 * @param pos Parameter for pos.
 * @param scale Parameter for scale.
 * @param rotationDeg Parameter for rotation deg.
 * @return Result produced by this operation.
 */
void Scene::SetTransformFromLevel(int id,
	const glm::vec3& pos,
	const glm::vec3& scale,
	float rotationDeg) {
	// Convert degrees to radians ONCE here
	const float rotationRad = rotationDeg * 3.14159265358979323846f / 180.0f;

	// EntityManager transform wrappers forward directly to the owning GameObject.
	entityManager.SetPosition(id, pos);
	entityManager.SetScale(id, scale);
	entityManager.SetRotation(id, rotationRad);

	GameObject* obj = GetGameObjectByID(id);
	if (obj) {
		obj->SetPosition(pos);
		obj->SetScale(scale);
		obj->SetRotation(rotationRad, glm::vec3(0.0f, 0.0f, 1.0f));
	}

	collisionManager.MarkStaticStateDirty();
}

/**
 * @brief Returns whether animations.
 * @param id Parameter for id.
 * @return True when the operation succeeds or the condition is met.
 */
bool Scene::HasAnimations(int id) const {
	return animationManager.HasAnimator(id);
}

/**
 * @brief Returns animation list.
 * @param id Parameter for id.
 * @return Requested value.
 */
std::vector<std::string> Scene::GetAnimationList(int id) const {
	return animationManager.GetAnimationNames(id);
}

/**
 * @brief Returns current animation name.
 * @param id Parameter for id.
 * @return Requested value.
 */
std::string Scene::GetCurrentAnimationName(int id) const {
	return animationManager.GetCurrentAnimation(id);
}

/**
 * @brief Sets animation.
 * @param objID Parameter for obj id.
 * @param animName Parameter for anim name.
 * @return Result produced by this operation.
 */
void Scene::SetAnimation(int objID, const std::string& animName) {
	animationManager.SetAnimation(objID, animName);
}

/**
 * @brief Performs attach player animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachPlayerAnimations(int objID) {
	animationManager.AttachPlayerAnimations(objID);
}

/**
 * @brief Performs attach dino animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachDinoAnimations(int objID) {
	animationManager.AttachDinoAnimations(objID);
}

/**
 * @brief Performs attach customers animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachCustomersAnimations(int objID) {
	animationManager.AttachCustomersAnimations(objID, GetObjectTexturePath(objID));
}

/**
 * @brief Performs attach customers animations.
 * @param objID Parameter for obj id.
 * @param texturePath Parameter for texture path.
 * @return Result produced by this operation.
 */
void Scene::AttachCustomersAnimations(int objID, const std::string& texturePath) {
	animationManager.AttachCustomersAnimations(objID, texturePath);
}

/**
 * @brief Performs attach work vfx cut animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachWorkVfxCutAnimations(int objID) {
	animationManager.AttachWorkVfxCutAnimations(objID);
}

/**
 * @brief Performs attach work vfx grill animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachWorkVfxGrillAnimations(int objID) {
	animationManager.AttachWorkVfxGrillAnimations(objID);
}

/**
 * @brief Performs attach work vfx stove animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachWorkVfxStoveAnimations(int objID) {
	animationManager.AttachWorkVfxStoveAnimations(objID);
}

/**
 * @brief Performs attach menu animations.
 * @param objID Parameter for obj id.
 * @return Result produced by this operation.
 */
void Scene::AttachMenuAnimations(int objID) {
	animationManager.AttachMenuAnimations(objID);
}

/**
 * @brief Performs mark animated.
 * @param id Parameter for id.
 * @param state Parameter for state.
 * @return Result produced by this operation.
 */
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

/**
 * @brief Performs attach logic for tag.
 * @param id Parameter for id.
 * @param tag Parameter for tag.
 * @return Result produced by this operation.
 */
void Scene::AttachLogicForTag(int id, const std::string& tag) {
	// ALWAYS wipe old logic from this object
	logicManager.RemoveAllFor(id, *this);

	if (tagLogicBinder_) {
		tagLogicBinder_(*this, id, tag);
	}
}

/**
 * @brief Sets object tag.
 * @param id Parameter for id.
 * @param tag Parameter for tag.
 * @return Result produced by this operation.
 */
void Scene::SetObjectTag(int id, const std::string& tag) {
	objectTags_[id] = tag;
}

/**
 * @brief Returns object tag.
 * @param id Parameter for id.
 * @return Requested value.
 */
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

/**
 * @brief Performs tag uses velocity.
 * @param tag Parameter for tag.
 * @return Result produced by this operation.
 */
bool Scene::TagUsesVelocity(const std::string& tag) const {
	if (tagUsesVelocityHook_) {
		return tagUsesVelocityHook_(tag);
	}
	return false;
}

/**
 * @brief Applies tag rules.
 * @param id Parameter for id.
 * @param tag Parameter for tag.
 * @param speedX Parameter for speed x.
 * @param speedY Parameter for speed y.
 * @return Result produced by this operation.
 */
void Scene::ApplyTagRules(int id, const std::string& tag, float speedX, float speedY) {
	if (tagRuleHook_) {
		tagRuleHook_(*this, id, tag, speedX, speedY);
	}
}

/**
 * @brief Returns movement manager.
 * @return Requested value.
 */
MovementManager& Scene::GetMovementManager() {
	return movementManager;
}

/**
 * @brief Returns movement manager.
 * @return Requested value.
 */
const MovementManager& Scene::GetMovementManager() const {
	return movementManager;
}

/**
 * @brief Returns collision manager.
 * @return Requested value.
 */
CollisionManager& Scene::GetCollisionManager() {
	return collisionManager;
}

/**
 * @brief Returns collision manager.
 * @return Requested value.
 */
const CollisionManager& Scene::GetCollisionManager() const {
	return collisionManager;
}

/**
 * @brief Returns collision world.
 * @return Requested value.
 */
collision::World& Scene::GetCollisionWorld() {
	return collisionManager.GetCollisionWorld();
}

/**
 * @brief Returns collision world.
 * @return Requested value.
 */
const collision::World& Scene::GetCollisionWorld() const {
	return collisionManager.GetCollisionWorld();
}

/**
 * @brief Returns layer sort key cached.
 * @param layerName Parameter for layer name.
 * @return Requested value.
 */
int Scene::GetLayerSortKeyCached(const std::string& layerName) const {
	auto it = layerSortKeyCache_.find(layerName);
	if (it != layerSortKeyCache_.end()) {
		return it->second;
	}

	int result = 1;
	if (!layerName.empty()) {
		result = 0;
		for (char c : layerName) {
			if (!std::isdigit(static_cast<unsigned char>(c))) {
				result = 1000000;
				break;
			}
			result = result * 10 + (c - '0');
		}
	}

	layerSortKeyCache_.emplace(layerName, result);
	return result;
}

/**
 * @brief Adds layer.
 * @param name Parameter for name.
 * @return Result produced by this operation.
 */
void Scene::AddLayer(const std::string& name) {
	layers.try_emplace(name, name); // Only add if missing
	layerSortKeyCache_.erase(name);
	collisionManager.MarkStaticStateDirty();
}

/**
 * @brief Returns layer.
 * @param name Parameter for name.
 * @return Requested value.
 */
Layer* Scene::GetLayer(const std::string& name) {
	auto it = layers.find(name);
	return it != layers.end() ? &(it->second) : nullptr;
}

/**
 * @brief Returns all layers.
 * @return Requested value.
 */
const std::unordered_map<std::string, Layer>& Scene::GetAllLayers() const {
	return layers;
}

/**
 * @brief Returns object layer.
 * @param objectID Identifier of the target object.
 * @return Requested value.
 */
std::string Scene::GetObjectLayer(int objectID) const {
	auto it = defaults_.find(objectID);
	if (it != defaults_.end()) {
		return it->second.layer;
	}

	return "";
}

/**
 * @brief Returns object layer ptr.
 * @param objectID Identifier of the target object.
 * @return Requested value.
 */
Layer* Scene::GetObjectLayerPtr(int objectID) {
	auto it = defaults_.find(objectID);
	if (it == defaults_.end() || it->second.layer.empty()) {
		return nullptr;
	}

	auto layerIt = layers.find(it->second.layer);
	return layerIt != layers.end() ? &(layerIt->second) : nullptr;
}

/**
 * @brief Returns object layer ptr.
 * @param objectID Identifier of the target object.
 * @return Requested value.
 */
const Layer* Scene::GetObjectLayerPtr(int objectID) const {
	auto it = defaults_.find(objectID);
	if (it == defaults_.end() || it->second.layer.empty()) {
		return nullptr;
	}

	auto layerIt = layers.find(it->second.layer);
	return layerIt != layers.end() ? &(layerIt->second) : nullptr;
}

/**
 * @brief Returns whether layer enabled.
 * @param layerName Parameter for layer name.
 * @return True when the operation succeeds or the condition is met.
 */
bool Scene::IsLayerEnabled(const std::string& layerName) const {
	auto it = layers.find(layerName);
	if (it == layers.end()) {
		return true;
	}
	return it->second.IsEnabled();
}

/**
 * @brief Returns whether object layer enabled.
 * @param objectID Identifier of the target object.
 * @return True when the operation succeeds or the condition is met.
 */
bool Scene::IsObjectLayerEnabled(int objectID) const {
	const std::string layerName = GetObjectLayer(objectID);
	if (layerName.empty()) {
		return true;
	}
	return IsLayerEnabled(layerName);
}

/**
 * @brief Performs assign object to layer.
 * @param id Parameter for id.
 * @param newLayer Parameter for new layer.
 * @return Result produced by this operation.
 */
void Scene::AssignObjectToLayer(int id, const std::string& newLayer) {
	std::string layerName = newLayer;
	if (layerName.empty()) layerName = "1";

	const auto defIt = defaults_.find(id);
	if (defIt != defaults_.end()) {
		const std::string& oldLayerName = defIt->second.layer;
		if (!oldLayerName.empty() && oldLayerName != layerName) {
			auto oldLayerIt = layers.find(oldLayerName);
			if (oldLayerIt != layers.end()) {
				oldLayerIt->second.RemoveObject(id);
			}
		}
	}

	// Register object ID with chosen layer (creates if missing)
	Layer& layer = layers[layerName];
	if (layer.GetName().empty()) {
		layer.SetName(layerName);
	}

	layer.AddObject(id);

	// Store on metadata used by the editor + JSON
	defaults_[id].layer = layerName;
	collisionManager.MarkStaticStateDirty();
}

/**
 * @brief Removes layer.
 * @param name Parameter for name.
 * @return Result produced by this operation.
 */
void Scene::RemoveLayer(const std::string& name) {
	auto it = layers.find(name);
	if (it != layers.end()) {
		layers.erase(it);
	}

	layerSortKeyCache_.erase(name);
	collisionManager.MarkStaticStateDirty();
}

/**
 * @brief Performs request state change.
 * @param newState Parameter for new state.
 * @return Result produced by this operation.
 */
void Scene::RequestStateChange(int newState) {
	pendingState_ = newState;
	hasPendingStateChange_ = true;
	SetFlowState(FlowState::Transitioning);
}


// text objects for menu buttons (disabled for now)
#if 0
/**
 * @brief Creates menu button texts.
 * @return Result produced by this operation.
 */
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

	const auto& allObjects = entityManager.GetObjectStorage();
	for (const auto& [tag, label] : buttonLabels) {
		// Find the button object with this tag
		for (const auto& objPtr : allObjects) {
			GameObject* obj = objPtr.get();
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

/**
 * @brief Renders menu button texts.
 * @return Result produced by this operation.
 */
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

/**
 * @brief Clears menu button texts.
 * @return Result produced by this operation.
 */
void Scene::ClearMenuButtonTexts() {
	menuButtonTexts_.clear();
}

#endif

/**
 * @brief Renders fpstext.
 * @return Result produced by this operation.
 */
void Scene::RenderFPSText() {
#ifndef _DEBUG
	if (!showFPS_) {
		return;
	}

	//static bool firstRender = true;
	//if (firstRender) {
	//	std::cout << "[Scene] RenderFPSText() called for the first time" << std::endl;
	//	firstRender = false;
	//}

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


void Scene::RenderLevelTextObjects() {
	const auto& objs = LEPANELFONTS::GetTextObjects();
	if (objs.empty()) return;

	const bool cutsceneActive = IsAnyCutsceneActive();
	if (cutsceneActive) return;

	const bool pauseActive = IsPauseOverlayActive();
	static const std::unordered_set<std::string> kHudTextNames = {
		"MoneyText", "QuotaText", "TimerText"
	};

	glm::mat4 projection = graphicsEngine.GetProjection();

	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	GLint boundFBO = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundFBO);

	if (boundFBO != 0) glViewport(0, 0, graphicsEngine.GetSceneWidth(), graphicsEngine.GetSceneHeight());
	else graphicsEngine.ApplyViewport();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	RenderFloatingWorldTextFx(projection, pauseActive, cutsceneActive);

	for (const auto& o : objs) {
		// Hide HUD text during cutscenes or pause overlay
		if ((cutsceneActive || pauseActive) && kHudTextNames.count(o.name)) continue;

		if (o.text.empty()) continue;
		if (o.fontName.empty()) continue;
		if (o.colorA <= 0.001f) continue;

		// (your existing layer visibility check is fine)
		if (!o.layer.empty()) {
			Layer* layer = GetLayer(o.layer);
			if (layer && (!layer->IsEnabled() || !layer->IsVisible()))
				continue;
		}

		FontSystem::Font* font = ResourceManager::Instance().GetFont(o.fontName);
		if (!font) continue;

		FontSystem::Text t;
		t.SetFont(font);
		t.SetText(o.text);
		t.SetColor(glm::vec4(o.colorR, o.colorG, o.colorB, o.colorA));
		t.SetScale(o.scale);
		t.SetRotation(o.rotation);
		t.SetPosition(glm::vec2(o.x, o.y));
		t.SetRotationMode(o.useBlockRotation
			? FontSystem::Text::RotationMode::Block
			: FontSystem::Text::RotationMode::PerCharacter);

		FontSystem::TextRenderer::Instance().RenderText(t, projection);
	}

	glDisable(GL_BLEND);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}
