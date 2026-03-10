/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(30%)
					Vu Phan Hung, phanhung.vu@digipen.edu   (15%)
					Ng Juin Herng, juinherng.ng@digipen.edu (15%)

 DESCRIPTION:		Declares the SceneManager (Scene) class, which orchestrates the lifecycle and
					high-level coordination of all major systems within a game scene. This includes:
					entity creation and management, event handling, physics and collision simulation,
					animation control, input processing, and rendering pipeline integration.

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Core/CollisionManager.hpp"
#include "../Core/DebugVisualizer.hpp"
#include "../Core/InputCommandHandler.hpp"
#include "../Core/InputManager.hpp"
#include "../Core/LevelEditor.hpp"
#include "../Core/LogicManager.hpp"
#include "../Core/Math.hpp"
#include "../Core/MovementManager.hpp" 
#include "../Core/NPCSystem.hpp"
#include "../Core/Physics.hpp"
#include "../Core/PhysicsManager.hpp"
#include "../Core/PlayerController.hpp"
#include "../Core/PlayerLogic.hpp"
#include "../Core/SimpleNpcLogic.hpp"
#include "../Core/TableLogic.hpp"
#include "../Core/WorkTableLogic.hpp"
#include "../Core/CustomerTableLogic.hpp"
#include "../Core/IngredientBoxLogic.hpp"
#include "../Core/CustomerManagerLogic.hpp"
#include "../Core/ExitGateLogic.hpp"
#include "../Core/HowToPlayButtonLogic.hpp"
#include "../Core/TrashCanLogic.hpp"
#include "../Core/Quota.hpp"
#include "../Core/OrderUILogic.hpp"
#include "../Core/GridPathfinder.hpp"
#include "../Core/ReplayManager.hpp"

#include "AnimationManager.hpp"
#include "Animator.hpp"
#include "EntityManager.hpp"
#include "GraphicsEngine.hpp"
#include "Layer.hpp"
#include "ParticleSystem.hpp"
#include "../Core/FontSystem.hpp"

class AudioManager;

 /**
  * @class Scene
  * @brief Manages the lifecycle of a game scene, including objects, animations, and input.
  */
class Scene {
public:
	// Systems and managers
	GraphicsEngine& GetGraphicsEngine();
	const GraphicsEngine& GetGraphicsEngine() const;

	// Input manager for player input and UI interactions
	MovementManager& GetMovementManager();
	const MovementManager& GetMovementManager() const;

	// Physics manager for physics simulation and queries
	CollisionManager& GetCollisionManager();
	const CollisionManager& GetCollisionManager() const;
	collision::World& GetCollisionWorld();
	const collision::World& GetCollisionWorld() const;

	// Entity manager for game object lifecycle and data access
	PlayerController& GetPlayerController() {
		return playerController;
	}
	const PlayerController& GetPlayerController() const {
		return playerController;
	}

	// Logic manager for tag-based behavior attachment and per-object logic updates
	LogicManager& GetLogicManager() {
		return logicManager;
	}
	const LogicManager& GetLogicManager() const {
		return logicManager;
	}

	// Animation manager for sprite animations and control
	Scene(GraphicsEngine& engine, InputManager& inputMgr, AnimationManager& animMgr,
		MovementManager& moveMgr, PhysicsManager& physicsMgr, CollisionManager& collisionMgr);

	// Set AudioManager for UI sounds and audio bindings
	void SetAudioManager(AudioManager* audioMgr) {
		audioManager_ = audioMgr;
	}

	// Get AudioManager (for audio bindings playback)
	AudioManager* GetAudioManager() const {
		return audioManager_;
	}

	// Audio binding playback helpers
	void PlaySpawnAudio(int objectId);
	void PlayInteractAudio(int objectId);
	void PlayDestroyAudio(int objectId);
	void PlayProcessingAudio(int objectId);   // Start looping processing audio
	void StopProcessingAudio(int objectId);   // Stop processing audio
	void StopAllObjectAudio();				  // Stop all audio bound to objects

	// Scene lifecycle and updates
	void LoadScene(const std::string& sceneName);
	void Update(float deltaTime, GLFWwindow* window);

	// Rendering
	void DrawUI();
	void ClearAll();
	void RequestClearAll();
	void RenderLevelTextObjects();

	// Simulation control
	void SetSimulationActive(bool active);
	bool IsSimulationActive() const;

	// Physics step control
	void ResetResizeBaseline();
	float GetLastPhysicsDt() const {
		return lastPhysicsDt_;
	}
	const physics::StepController& GetStepController() const {
		return physicsStep_;
	}

	// Spawning / object management

	// Spawns a static sprite with a given texture, size, and layer. Returns the new GameObject* or nullptr on failure.
	GameObject* SpawnStaticSprite(const std::string& texturePath,
		const glm::vec3 position,
		const glm::vec2 size = glm::vec2(100.0f, 100.0f),
		const std::string& layer = "Not set in JSON");

	// Spawns an animated sprite with a given texture, size, and animation frames.
	GameObject* SpawnAnimatedSprite(const std::string& texturePath,
		const glm::vec3 position,
		const glm::vec2 size,
		const std::vector<glm::vec4> frames,
		float frameDuration, bool loop,
		const std::string& layer);

	// Spawns a static sprite at the same position as ownerID, with given texture/size/layer.
	// Returns the new GameObject* or nullptr on failure.
	GameObject* SpawnStaticSpriteAtSamePos(int ownerID,
		const std::string& texturePath,
		float width,
		float height,
		const std::string& layer);

	// Spawns an animated sprite at the same position as ownerID, with given texture/size/frames/layer.
	GameObject* GetGameObjectByID(int targetID);
	std::vector<GameObject*> GetAllObjectsRaw();
	const std::vector<std::unique_ptr<GameObject>>& GetObjectStorageRaw() const;
	void DespawnByID(int targetID);
	void CollectRenderablePointers(std::vector<GameObject*>& out);

	// Scene / transform utilities

	// Set the scene background texture
	void SetSceneBackground(const std::string& texturePath);

	// Set the transform of the object with given ID, using level editor semantics
	void SetTransformFromLevel(int id, const glm::vec3& pos, const glm::vec3& scale, float rotation);
	void ClampToWalkArea(GameObject* obj);
	glm::vec2 ResolveWorldStep(GameObject* obj, const glm::vec2& desiredDelta);

	// Coordinate scaling utilities (for level editor reference vs actual game units)
	float ScaleXToCurrent(float referenceX) const;
	float ScaleYToCurrent(float referenceY) const;
	float ToRefX(float currentX) const;
	float ToRefY(float currentY) const;

	// Animation helpers
	bool HasAnimations(int id) const;
	std::vector<std::string> GetAnimationList(int id) const;
	std::string GetCurrentAnimationName(int id) const;

	// Set the current animation of the object with given ID. Does nothing if object doesn't exist or doesn't have that animation.
	void SetAnimation(int objID, const std::string& newAnim);
	void AttachPlayerAnimations(int objID);
	void AttachDinoAnimations(int objID);
	void MarkAnimated(int id, bool state);
	void AttachMenuAnimations(int objID);

	// Stress test helper (spawns a large number of objects with simple movement logic to test performance and stability)
	void GenerateStressTest(int objectCount = 2500);
	void UpdateAnimationControls();

	// Tag-based logic helpers
	void AttachLogicForTag(int id, const std::string& tag);
	using TagLogicBinder = std::function<void(Scene&, int, const std::string&)>;
	using TagRuleHook = std::function<void(Scene&, int, const std::string&, float, float)>;
	using PauseOverlayButtonBinder = std::function<void(Scene&, int, const std::string&)>;
	using CustomerUpdateHook = std::function<void(float, Scene&)>;
	using CustomerResetHook = std::function<void(Scene&)>;
	using RuntimeObjectSetupHook = std::function<void(Scene&, int, const std::string&, const std::string&, bool, const std::string&, float, float)>;
	using SimulationUpdateHook = std::function<void(float, Scene&)>;
	using DefaultSceneSetupHook = std::function<void(Scene&)>;
	using PostLevelLoadHook = std::function<void(Scene&, bool)>;
	using CutsceneFadeOutHook = std::function<void(Scene&, float)>;
	using CutsceneFirstFrameHook = std::function<void(Scene&, const std::string&)>;
	using CutsceneBeforeFinalLoadHook = std::function<void(Scene&, float)>;
	void SetTagLogicBinder(TagLogicBinder binder) {
		tagLogicBinder_ = std::move(binder);
	}
	void SetPauseOverlayButtonBinder(PauseOverlayButtonBinder binder) {
		pauseOverlayButtonBinder_ = std::move(binder);
	}
	void SetTagRuleHook(TagRuleHook hook) {
		tagRuleHook_ = std::move(hook);
	}
	void SetCustomerUpdateHook(CustomerUpdateHook hook) {
		customerUpdateHook_ = std::move(hook);
	}
	void SetCustomerResetHook(CustomerResetHook hook) {
		customerResetHook_ = std::move(hook);
	}
	void SetRuntimeObjectSetupHook(RuntimeObjectSetupHook hook) {
		runtimeObjectSetupHook_ = std::move(hook);
	}
	void SetSimulationUpdateHook(SimulationUpdateHook hook) {
		simulationUpdateHook_ = std::move(hook);
	}
	void SetDefaultSceneSetupHook(DefaultSceneSetupHook hook) {
		defaultSceneSetupHook_ = std::move(hook);
	}
	void SetPostLevelLoadHook(PostLevelLoadHook hook) {
		postLevelLoadHook_ = std::move(hook);
	}
	void SetCutsceneFadeOutHook(CutsceneFadeOutHook hook) {
		cutsceneFadeOutHook_ = std::move(hook);
	}
	void SetCutsceneFirstFrameHook(CutsceneFirstFrameHook hook) {
		cutsceneFirstFrameHook_ = std::move(hook);
	}
	void SetCutsceneBeforeFinalLoadHook(CutsceneBeforeFinalLoadHook hook) {
		cutsceneBeforeFinalLoadHook_ = std::move(hook);
	}
	void SetPauseOverlayAudioChannels(std::string musicChannel, std::string ambienceChannel) {
		pauseMusicChannel_ = std::move(musicChannel);
		pauseAmbienceChannel_ = std::move(ambienceChannel);
	}
	void ApplyRuntimeObjectSetup(int id, const std::string& tag, const std::string& texturePath, bool animated, const std::string& animName, float speedX, float speedY) {
		if (runtimeObjectSetupHook_) {
			runtimeObjectSetupHook_(*this, id, tag, texturePath, animated, animName, speedX, speedY);
		}
	}

	// Centralized tag metadata
	void SetObjectTag(int id, const std::string& tag);
	std::string GetObjectTag(int id) const;

	// Centralized tag rules
	void ApplyTagRules(int id, const std::string& tag, float speedX, float speedY);
	bool TagUsesVelocity(const std::string& tag) const;

	// ID / role helpers
	void SetPlayerID(int id);
	int GetPlayerID() const {
		return spriteID;
	}

	// NPC IDs can be extended as needed
	void SetNPC1ID(int id) {
		otherID = id;
		if (id >= 0) {
			npcSystem.RegisterLaneNPC(id, 1000.0f); // Only this npc1 gets lane behavior
		}
	}
	void SetNPC2ID(int id) {
		otherID2 = id;
		if (id >= 0) {
			npcSystem.RegisterLaneNPC(id, 1000.0f); //Only this npc2 gets lane behavior
		}
	}
	void SetDinoID(int id) {
		dinoID = id;
	}
	int GetNPC1ID() const {
		return otherID;
	}
	int GetNPC2ID() const {
		return otherID2;
	}
	int GetDinoID() const {
		return dinoID;
	}

	// NPC System
	void SetNPCVelocity(int id, float vx, float vy) {
		npcSystem.SetNPCVelocity(id, glm::vec2(vx, vy));
	}
	glm::vec2 GetNPCVelocity(int id) const {
		return npcSystem.GetNPCVelocity(id);
	}
	void RegisterLaneNPC(int id, float laneX) {
		npcSystem.RegisterLaneNPC(id, laneX);
	}

	// Exit gate handling
	void RegisterExitGate(int id) {
		exitGateID_ = id;
		exitGateCached_ = false;
	}
	Math::Vector2D GetExitGateWorldPos() {
		if (exitGateID_ < 0) return { 0.f, 0.f };
		if (GameObject* g = GetGameObjectByID(exitGateID_)) {
			auto p = g->GetPositionGLM();
			return { p.x, p.y };
		}
		return { 0.f, 0.f };
	}

	// Texture metadata (LevelEditor / JSON)
	const std::string& GetObjectTexturePath(int id) const;
	void SetObjectTexturePath(int id, const std::string& path);

	// Default properties for objects by ID
	struct Defaults {
		glm::vec3 pos{ 0,0,0 };
		glm::vec2 size{ 128,128 };
		float rot{ 0.f };
		glm::vec2 colSize{ 64,128 };
		glm::vec2 colOff{ 0,0 };
		glm::vec2 vel{ 0,0 };
		glm::vec2 approachOffset{ 0,0 }; // Table approach point
		std::string texture;
		std::string tag;
		std::string layer;
		// Audio bindings
		std::string audioOnSpawn;
		std::string audioOnInteract;
		std::string audioOnDestroy;
		std::string audioOnProcessing;  // Audio that loops while work table is processing
		bool audioLoop{ false };
		// Per-object visibility (default visible)
		bool visible{ true };
	};

	// Set and get defaults for an object ID. Setting defaults will mark static state dirty for collision rebuild.
	void SetDefaults(int id, const Defaults& d) {
		defaults_[id] = d;
		collisionManager.MarkStaticStateDirty();
	}
	Defaults GetDefaults(int id) const {
		auto it = defaults_.find(id);
		return (it != defaults_.end()) ? it->second : Defaults{};
	}

	// Per-object visibility controls 
	void SetObjectVisible(int id, bool visible) {
		defaults_[id].visible = visible;
		collisionManager.MarkStaticStateDirty();
	}
	bool IsObjectVisible(int id) const {
		auto it = defaults_.find(id);
		return (it != defaults_.end()) ? it->second.visible : true;
	}

	// Particle system
	ParticleSystem particleSystem_;
	ParticleSystem& GetParticleSystem() {
		return particleSystem_;
	}

	// Layers
	void AddLayer(const std::string& name);
	Layer* GetLayer(const std::string& name);
	const std::unordered_map<std::string, Layer>& GetAllLayers() const;

	// Layer assignment helpers
	std::string GetObjectLayer(int objectID) const;
	Layer* GetObjectLayerPtr(int objectID);
	const Layer* GetObjectLayerPtr(int objectID) const;
	void AssignObjectToLayer(int id, const std::string& newLayer);
	void RemoveLayer(const std::string& name);

	// Layer enable/disable helpers
	bool IsLayerEnabled(const std::string& layerName) const;
	bool IsObjectLayerEnabled(int objectID) const;

	// Collision and walk area
	void BuildLevelColliders();
	void RebuildColliders();
	void ResolveInitialStaticOverlaps();
	collision::WalkArea GetWalkArea() const;
	void HandlePlayerCollisions(float deltaTime, EntityManager& entityMgr);
	void ApplyFinalConstraints(EntityManager& entityMgr);

	// Expose EntityManager for systems that need it
	EntityManager& GetEntityManager() {
		return entityManager;
	}

	// Level loading queue
	void QueueLevelLoad(const std::string& path, bool activateSimulation);
	bool HasPendingLevel() const {
		return hasPendingLevel_;
	}

	// Game state change request
	void RequestStateChange(int newState);
	bool HasPendingStateChange() const {
		return hasPendingStateChange_;
	}
	int GetPendingState() const {
		return pendingState_;
	}
	void ClearPendingStateChange() {
		hasPendingStateChange_ = false;
	}

	// Pause overlay methods
	void ShowPauseOverlay();
	void HidePauseOverlay();
	bool IsPauseOverlayActive() const {
		return pauseOverlayActive_;
	}

	// Replay state exposure
	bool IsReplayPlaybackActive() const {
		return replayManager_.IsPlaybackActive();
	}
	float GetReplayFrameDt() const {
		return lastReplayFrameDt_;
	}
	bool HasReplayFrameDt() const {
		return lastReplayFrameDt_ > 0.0f;
	}


	// Menu button text rendering
#if 0
	void CreateMenuButtonTexts();
	void RenderMenuButtonTexts();
	void ClearMenuButtonTexts();
#endif

	// How-to-play overlay state
	void SetHowToPlayOverlayActive(bool active) {
		howToPlayOverlayActive_ = active;
	}
	bool IsHowToPlayOverlayActive() const {
		return howToPlayOverlayActive_;
	}
	// FPS display rendering
	void RenderFPSText();

	// Pending despawn queue (to avoid modifying EntityManager during iteration)
	void RequestDespawn(int id) {
		pendingDespawns_.push_back(id);
	}

	// Cutscene API
	// Starts a cutscene consisting of image paths played in sequence.
	// When finished, queues level load to 'levelJsonPath' and sets simulation according to 'activateSimulation'.
	void StartCutscene(const std::vector<std::string>& imagePaths,
		float holdSecondsPerImage,
		float fadeSeconds,
		const std::string& levelJsonPath,
		bool activateSimulation);

	// Starts a cutscene with fade transitions between frames. 'holdSeconds' controls how long each image is held at full opacity before transitioning to the next.
	void StartCutsceneTransitioned(const std::vector<std::string>& imagePaths,
		const std::string& levelJsonPath,
		bool activateSimulation,
		float fadeOutSeconds = 0.35f,
		float fadeInSeconds = 0.35f,
		float holdSeconds = 1.5f,
		int crossfadeFromIndex = -1,           // -1 = disabled; otherwise crossfade when transitioning to this target index
		float crossfadeSeconds = 0.75f);

	// Starts a cutscene with per-frame images. 'boundaryFlags' marks indices where a chapter boundary occurs.
	// At boundaries, the engine performs fade-out/in (or crossfade when 'crossfadeFromIndex' matches).
	// Between frames without a boundary, it swaps instantly with no transition (for smooth animation).
	void StartCutsceneTransitionedBounded(const std::vector<std::string>& imagePaths,
		const std::vector<bool>& boundaryFlags,
		const std::string& levelJsonPath,
		bool activateSimulation,
		float fadeOutSeconds = 0.35f,
		float fadeInSeconds = 0.35f,
		float holdSeconds = 1.0f / 12.0f, // default 12 FPS
		int crossfadeFromIndex = -1,
		float crossfadeSeconds = 0.75f);

	// Order UI slide-in API
	// Spawns an Order UI sprite off-screen at the top, then animates it sliding down to target.
	// Returns spawned object ID or -1 on failure.
	int TriggerOrderUiSlideIn(const glm::vec2& targetPos,
		const glm::vec2& size,
		const std::string& layer = "3",
		const std::string& texturePath = "../assets/Order_UI.png",
		float slideDuration = 0.45f);

	// Check if any cutscene is active
	bool IsAnyCutsceneActive() const {
		return cutscene_.active || cutTrans_.active;
	}

	// Skips whichever cutscene system is currently active and advances to queued target level.
	void SkipActiveCutscene();

private:
	// Engine/input
	GraphicsEngine& graphicsEngine;
	EntityManager entityManager;
	LogicManager logicManager;
	InputManager& inputManager;				// Changed from owned instance to reference
	AnimationManager& animationManager;		// Changed from owned instance to reference
	MovementManager& movementManager;		// Changed from owned instance to reference
	CollisionManager& collisionManager;		// Changed from owned instance to reference
	PhysicsManager& physicsManager;			// Changed from owned instance to reference
	ReplayManager replayManager_;

	// Audio for UI sounds
	AudioManager* audioManager_ = nullptr;

	// Systems
	InputCommandHandler inputCommandHandler;
	PlayerController playerController;
	NPCSystem npcSystem;
	DebugVisualizer debugVisualizer;

	// Step-by-step controller
	physics::StepController physicsStep_;
	float lastPhysicsDt_ = 0.0f;

	// Scene state
	bool simulationActive = false;
	bool useForces_ = false;
	bool showAuxDebug_ = false;

	// Replay state
	float lastReplayFrameDt_ = 0.0f;

	// ID tracking for important objects (player, NPCs, etc.)
	int spriteID = -1;
	int dinoID = -1;
	int otherID = -1;
	int otherID2 = -1;

	// Cached exit gate info for quick access (assuming only one exit gate per level)
	int exitGateID_ = -1;
	bool exitGateCached_ = false;
	Math::Vector2D exitGateWorld_{ 0.0f, 0.0f };

	// Centralized defaults and metadata for objects, keyed by ID
	std::unordered_map<int, Defaults> defaults_;
	std::unordered_map<std::string, Layer> layers;
	int GetLayerSortKeyCached(const std::string& layerName) const;
	mutable std::unordered_map<std::string, int> layerSortKeyCache_;

	// Resize handling
	int lastWidth_ = -1;
	int lastHeight_ = -1;
	bool resetBaseline_ = false;

	// Clear all pending flag (to avoid modifying EntityManager during iteration)
	bool pendingClear_ = false;
	int editorSelectedId = -1;
	std::string currentLevelPath_;

	// Pending level load state
	std::string pendingLevelPath_;
	bool pendingLevelSimActive_ = false;
	bool hasPendingLevel_ = false;

	// Pending game state change
	int pendingState_ = -1;
	bool hasPendingStateChange_ = false;

	// Pause overlay state
	bool pauseOverlayActive_ = false;
	std::vector<int> pauseOverlayObjectIds_;

	// Pause audio fade state
	bool pauseAudioPending_ = false;
	float pauseAudioTimer_ = 0.0f;
	float pausedBgmVolume_ = 0.0f;
	float pausedAmbienceVolume_ = 0.0f;

	// Menu button text rendering
	struct MenuButtonText {
		FontSystem::Text textObj;
		int buttonID = 0;
		std::string label;
	};
	std::vector<MenuButtonText> menuButtonTexts_;

	// FPS display (release builds) - toggled with F1
	FontSystem::Text fpsText_;
	bool showFPS_ = false;
	float fpsAccumTime_ = 0.0f;
	int fpsAccumFrames_ = 0;
	int fpsValue_ = 0;
	const float fpsUpdateInterval_ = 0.25f; // update every 0.25s

	LevelEditor mLevelEditor;
	std::unordered_map<int, std::string> mTexturePathByID;
	std::unordered_map<int, std::string> objectTags_;
	TagLogicBinder tagLogicBinder_;
	TagRuleHook tagRuleHook_;
	PauseOverlayButtonBinder pauseOverlayButtonBinder_;
	CustomerUpdateHook customerUpdateHook_;
	CustomerResetHook customerResetHook_;
	RuntimeObjectSetupHook runtimeObjectSetupHook_;
	SimulationUpdateHook simulationUpdateHook_;
	DefaultSceneSetupHook defaultSceneSetupHook_;
	PostLevelLoadHook postLevelLoadHook_;
	CutsceneFadeOutHook cutsceneFadeOutHook_;
	CutsceneFirstFrameHook cutsceneFirstFrameHook_;
	CutsceneBeforeFinalLoadHook cutsceneBeforeFinalLoadHook_;
	std::string pauseMusicChannel_;
	std::string pauseAmbienceChannel_;

	bool howToPlayOverlayActive_ = false;

	std::vector<int> pendingDespawns_;

	// Basic cutscene runner with simple fade-out, image swap, fade-in sequence. No cross-fade or separate hold time.
	struct CutsceneState {
		bool active = false;
		std::vector<std::string> images;
		size_t current = 0;

		// timing
		float holdTime = 1.5f;   // seconds each image is held
		float fadeTime = 0.5f;   // seconds to fade out/in (cross-fade if supported)
		float t = 0.0f;          // time accumulator within current phase

		// phase control
		enum class Phase {
			FadeIn, Hold, FadeOut
		} phase = Phase::FadeIn;

		// objects
		int spriteA = -1;        // current image object
		int spriteB = -1;        // next image object (for cross-fade)
		std::string uiLayer = "999998"; // cutscene layer below pause overlay

		// completion
		std::string targetLevelJson;
		bool targetActivateSim = true;
		bool queuedFinalLoad = false;

		// alpha support flag detected on first use
		bool supportsAlpha = false;
	} cutscene_;

	// More advanced cutscene runner with support for fade transitions between frames, optional cross-fade, and separate hold time after fade-in.
	struct CutsceneTrans {
		bool active = false;
		std::vector<std::string> images;
		size_t index = 0;
		std::string uiLayer = "999998";
		int currentSpriteId = -1;
		std::string targetLevelJson;
		bool targetActivateSim = true;
		float outSeconds = 0.35f;
		float inSeconds = 0.35f;
		bool fadeInAfterLoad = false;

		// cross-fade support
		bool useCrossfade = false;
		float crossfadeSeconds = 0.75f;
		float crossfadeT = 0.0f;
		int nextSpriteId = -1;
		bool crossfading = false;

		// hold control
		float holdSeconds = 1.5f;      // how long each image stays after fade-in
		float holdElapsed = 0.0f;
		bool holding = false;

		bool awaitingBlackout = false;

		// crossfade index control
		int crossfadeFromIndex = -1; // -1 = disabled; otherwise crossfade when transitioning to this target index

		// initial fade-in control
		bool awaitingInitialFadeIn = false;
	} cutTrans_;

	// Order UI slide-in state
	struct UiSlide {
		int objectId = -1;
		glm::vec2 startPos{};
		glm::vec2 targetPos{};
		float t = 0.0f;
		float duration = 0.5f;
		bool active = false;
	};
	std::vector<UiSlide> uiSlides_; // multiple parallel slides if needed

	// Internal helpers
	void UpdateCutscene(float dt);
	void UpdateCutsceneTransitioned(float dt);
	void CleanupCutsceneObjects();
	void SetSpriteAlpha(GameObject* obj, float alpha); // no-op if shader lacks alpha tint

	// Update all active UI slides
	void UpdateUiSlides(float dt);
	// Ease-out cubic for snappy drop
	static float EaseOutCubic(float x) {
		float inv = 1.0f - x;
		return 1.0f - inv * inv * inv;
	}

public:
	// Starts a level transition cutscene with fade-out, image swap, fade-in sequence. No cross-fade or separate hold time.
	void StartLevelTransition(const std::string& levelJsonPath,
		bool activateSimulation,
		float fadeOutSeconds = 0.35f,
		float fadeInSeconds = 0.35f);

		bool BuildNavigationGridForObject(int moverObjectID, NavGrid& outGrid);
		bool FindPathForObject(int moverObjectID,
			const glm::vec2& startWorld,
			const glm::vec2& goalWorld,
			std::vector<glm::vec2>& outPath);

		bool GetNearestNavigationCellCenterForObject(int moverObjectID,
			const glm::vec2& worldPos,
			glm::vec2& outCenter);

		bool HasDirectPathForObject(int moverObjectID,
			const glm::vec2& startWorld,
			const glm::vec2& goalWorld);

private:
	// Simpler version of cutscene transition for level changes without per-frame images. Reuses some cutTrans_ state for convenience.
	struct LevelTrans {
		bool active = false;
		bool awaitingBlackout = false;
		std::string targetJson;
		bool targetActivateSim = false;
		float outSec = 0.35f;
		float inSec = 0.35f;
	} levelTrans_;

	void UpdateLevelTransition();
};
