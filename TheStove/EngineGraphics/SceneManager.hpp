/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (30%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(30%)
					Vu Phan Hung, phanhung.vu@digipen.edu   (15%)
					Ng Juin Herng, juinherng.ng@digipen.edu (25%)

 DESCRIPTION:		Declares the SceneManager (Scene) class, which orchestrates the lifecycle and
					high-level coordination of all major systems within a game scene. This includes:
					entity creation and management, event handling, physics and collision simulation,
					animation control, input processing, and rendering pipeline integration.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "EngineCore/CollisionManager.hpp"
#include "EngineCore/DebugVisualizer.hpp"
#include "EngineCore/FontSystem.hpp"
#include "EngineCore/GameStateManager.hpp"
#include "EngineCore/GridPathfinder.hpp"
#include "EngineCore/InputCommandHandler.hpp"
#include "EngineCore/LogicManager.hpp"
#include "EngineCore/Math.hpp"
#include "EngineCore/NPCSystem.hpp"
#include "EngineCore/Physics.hpp"
#include "EngineCore/PlayerController.hpp"
#include "EngineCore/ReplayManager.hpp"
#include "EngineCore/RuntimeTextData.hpp"
#include "EngineGraphics/EntityManager.hpp"
#include "EngineGraphics/Layer.hpp"
#include "EngineGraphics/ParticleSystem.hpp"
#include "EngineGraphics/SceneObjectMetadata.hpp"
#include "EngineGraphics/VideoPlayer.hpp"

class AudioManager;
class GameObject;
class GraphicsEngine;
class InputManager;
class AnimationManager;
class MovementManager;
class CollisionManager;
class PhysicsManager;
struct GLFWwindow;
namespace CoreFramework {
	class MessageBus;
}

/**
 * @class Scene
 * @brief Manages the lifecycle of a game scene, including objects, animations, and input.
 */
class Scene {
public:
	enum class FlowState {
		Bootstrapping,
		LoadingLevel,
		Transitioning,
		Cutscene,
		Gameplay,
		Paused,
		NonSimulation
	};

	/** @name Core Manager Accessors */
	/// @{
	GraphicsEngine& GetGraphicsEngine();
	const GraphicsEngine& GetGraphicsEngine() const;
	MovementManager& GetMovementManager();
	const MovementManager& GetMovementManager() const;
	CollisionManager& GetCollisionManager();
	const CollisionManager& GetCollisionManager() const;
	collision::World& GetCollisionWorld();
	const collision::World& GetCollisionWorld() const;
	PlayerController& GetPlayerController() {
		return playerController;
	}
	const PlayerController& GetPlayerController() const {
		return playerController;
	}
	LogicManager& GetLogicManager() {
		return logicManager;
	}
	const LogicManager& GetLogicManager() const {
		return logicManager;
	}
	/// @}

	// -------------------------------------------------------------------------------------------------
	// Scene Lifetime And Frame Control
	// -------------------------------------------------------------------------------------------------

	/**
	 * @brief Constructs a `Scene` instance.
	 * @param engine Parameter for engine.
	 * @param inputMgr Parameter for input mgr.
	 * @param animMgr Parameter for anim mgr.
	 * @param moveMgr Parameter for move mgr.
	 * @param physicsMgr Parameter for physics mgr.
	 * @param collisionMgr Parameter for collision mgr.
	 */
	Scene(GraphicsEngine& engine, InputManager& inputMgr, AnimationManager& animMgr,
		MovementManager& moveMgr, PhysicsManager& physicsMgr, CollisionManager& collisionMgr);

	/**
	 * @brief Sets audio manager.
	 * @param audioMgr Parameter for audio mgr.
	 */
	void SetAudioManager(AudioManager* audioMgr) {
		audioManager_ = audioMgr;
	}

	/**
	 * @brief Sets message bus.
	 * @param bus Parameter for bus.
	 */
	void SetMessageBus(CoreFramework::MessageBus* bus) {
		messageBus_ = bus;
	}

	/**
	 * @brief Returns audio manager.
	 * @return Requested value.
	 */
	AudioManager* GetAudioManager() const {
		return audioManager_;
	}

	/** @name Object-Bound Audio Helpers
	 * Helpers for playing and stopping sounds configured on scene object defaults.
	 */
	 /// @{
	void PlaySpawnAudio(int objectId);
	void PlayInteractAudio(int objectId);
	void PlayDestroyAudio(int objectId);
	void PlayProcessingAudio(int objectId);   // Start looping processing audio
	void StopProcessingAudio(int objectId);   // Stop processing audio
	void StopAllObjectAudio();				  // Stop all audio bound to objects
	/// @}

	/** @name Scene Lifecycle And Presentation */
	/// @{
	void LoadScene(const std::string& sceneName);
	void Update(float deltaTime, GLFWwindow* window);
	void DrawUI();
	void ClearAll();
	void RequestClearAll();
	void RenderLevelTextObjects();
	const std::vector<RuntimeTextData>& GetRuntimeTextObjects() const {
		return runtimeTextObjects_;
	}
	void SetRuntimeTextObjects(const std::vector<RuntimeTextData>& textObjects);
	bool SetRuntimeTextByName(const std::string& name, const std::string& newText);
	void ClearRuntimeTextObjects();
	void SetPauseSuppressedRuntimeTextNames(std::vector<std::string> textNames);
	const std::unordered_set<std::string>& GetEditorPreservedRuntimeTextNames() const {
		return editorPreservedRuntimeTextNames_;
	}
	void SetEditorPreservedRuntimeTextNames(std::vector<std::string> textNames);
	void SetEditorUiHook(std::function<void(Scene&)> hook) {
		editorUiHook_ = std::move(hook);
	}
	void SetEditorToggleHook(std::function<void()> hook) {
		editorToggleHook_ = std::move(hook);
	}
	void SetEditorEnabledQuery(std::function<bool()> query) {
		editorEnabledQuery_ = std::move(query);
	}
	void SetEditorPlayingQuery(std::function<bool()> query) {
		editorPlayingQuery_ = std::move(query);
	}
	/// @}

	/**
	 * @brief Sets simulation active.
	 * @param active Parameter for active.
	 */
	void SetSimulationActive(bool active);

	/**
	 * @brief Returns whether simulation active.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsSimulationActive() const;

	/**
	 * @brief Resets resize baseline.
	 */
	void ResetResizeBaseline();

	/**
	 * @brief Returns last physics dt.
	 * @return Requested value.
	 */
	float GetLastPhysicsDt() const {
		return lastPhysicsDt_;
	}

	/**
	 * @brief Returns step controller.
	 * @return Requested value.
	 */
	const physics::StepController& GetStepController() const {
		return physicsStep_;
	}

	// -------------------------------------------------------------------------------------------------
	// Spawning, Object Access, And World Utilities
	// -------------------------------------------------------------------------------------------------

	// Spawning / object management

	/**
	 * @brief Performs spawn static sprite.
	 * @param texturePath Parameter for texture path.
	 * @param position Parameter for position.
	 * @param size Parameter for size.
	 * @param layer Parameter for layer.
	 * @return Result produced by this operation.
	 */
	GameObject* SpawnStaticSprite(const std::string& texturePath,
		const glm::vec3 position,
		const glm::vec2 size = glm::vec2(100.0f, 100.0f),
		const std::string& layer = "Not set in JSON");

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
	GameObject* SpawnAnimatedSprite(const std::string& texturePath,
		const glm::vec3 position,
		const glm::vec2 size,
		const std::vector<glm::vec4>& frames,
		float frameDuration, bool loop,
		const std::string& layer);

	// Spawns a static sprite at the same position as ownerID, with given texture/size/layer.
	/**
	 * @brief Performs spawn static sprite at same pos.
	 * @param ownerID Parameter for owner id.
	 * @param texturePath Parameter for texture path.
	 * @param width Width value in pixels.
	 * @param height Height value in pixels.
	 * @param layer Parameter for layer.
	 * @return Result produced by this operation.
	 */
	GameObject* SpawnStaticSpriteAtSamePos(int ownerID,
		const std::string& texturePath,
		float width,
		float height,
		const std::string& layer);

	/**
	 * @brief Returns game object by id.
	 * @param targetID Parameter for target id.
	 * @return Requested value.
	 */
	GameObject* GetGameObjectByID(int targetID);

	/**
	 * @brief Returns all objects raw.
	 * @return Requested value.
	 */
	std::vector<GameObject*> GetAllObjectsRaw();

	/**
	 * @brief Returns object storage raw.
	 * @return Requested value.
	 */
	const std::vector<std::unique_ptr<GameObject>>& GetObjectStorageRaw() const;

	/**
	 * @brief Performs despawn by id.
	 * @param targetID Parameter for target id.
	 */
	void DespawnByID(int targetID);

	/**
	 * @brief Collects renderable pointers.
	 * @param out Output value for out.
	 */
	void CollectRenderablePointers(std::vector<GameObject*>& out);

	// -------------------------------------------------------------------------------------------------
	// Scene Presentation, Transform, And Animation Utilities
	// -------------------------------------------------------------------------------------------------

	/** @name Scene Background Helpers */
	/// @{
	void SetSceneBackground(const std::string& texturePath);
	void SetSceneBackgroundOverlay(const std::string& texturePath);
	void ClearSceneBackgroundOverlay();
	const std::string& GetSceneBackground() const {
		return sceneBackgroundPath_;
	}
	const std::string& GetSceneBackgroundOverlay() const {
		return sceneBackgroundOverlayPath_;
	}
	/// @}

	/**
	 * @brief Sets transform from level.
	 * @param id Parameter for id.
	 * @param pos Parameter for pos.
	 * @param scale Parameter for scale.
	 * @param rotation Parameter for rotation.
	 */
	void SetTransformFromLevel(int id, const glm::vec3& pos, const glm::vec3& scale, float rotation);

	/**
	 * @brief Performs clamp to walk area.
	 * @param obj Parameter for obj.
	 */
	void ClampToWalkArea(GameObject* obj);

	/**
	 * @brief Resolves world step.
	 * @param obj Parameter for obj.
	 * @param desiredDelta Parameter for desired delta.
	 * @return Result produced by this operation.
	 */
	glm::vec2 ResolveWorldStep(GameObject* obj, const glm::vec2& desiredDelta);

	/**
	 * @brief Performs scale xto current.
	 * @param referenceX Parameter for reference x.
	 * @return Result produced by this operation.
	 */
	float ScaleXToCurrent(float referenceX) const;

	/**
	 * @brief Performs scale yto current.
	 * @param referenceY Parameter for reference y.
	 * @return Result produced by this operation.
	 */
	float ScaleYToCurrent(float referenceY) const;

	/**
	 * @brief Performs to ref x.
	 * @param currentX Parameter for current x.
	 * @return Result produced by this operation.
	 */
	float ToRefX(float currentX) const;

	/**
	 * @brief Performs to ref y.
	 * @param currentY Parameter for current y.
	 * @return Result produced by this operation.
	 */
	float ToRefY(float currentY) const;

	/** @name Animation Helpers */
	/// @{
	bool HasAnimations(int id) const;
	std::vector<std::string> GetAnimationList(int id) const;
	std::string GetCurrentAnimationName(int id) const;
	void SetAnimation(int objID, const std::string& newAnim);
	void AttachPlayerAnimations(int objID);
	void AttachDinoAnimations(int objID);
	void AttachCustomersAnimations(int objID);
	void AttachCustomersAnimations(int objID, const std::string& texturePath);
	void AttachWorkVfxCutAnimations(int objID);
	void AttachWorkVfxGrillAnimations(int objID);
	void AttachWorkVfxStoveAnimations(int objID);
	void MarkAnimated(int id, bool state);
	void AttachMenuAnimations(int objID);
	/// @}

	/**
	 * @brief Generates stress test.
	 * @param objectCount Parameter for object count.
	 */
	void GenerateStressTest(int objectCount = 2500);

	/**
	 * @brief Updates animation controls.
	 */
	void UpdateAnimationControls();

	/**
	 * @brief Performs attach logic for tag.
	 * @param id Parameter for id.
	 * @param tag Parameter for tag.
	 */
	void AttachLogicForTag(int id, const std::string& tag);

	// -------------------------------------------------------------------------------------------------
	// Scene Extension Hooks
	// -------------------------------------------------------------------------------------------------

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
	using SkipCutsceneAudioHook = std::function<void(Scene&, float)>;
	using TagUsesVelocityHook = std::function<bool(const std::string&)>;
	using NavigationBlockerCollector = std::function<void(Scene&, int, std::vector<collision::AABB>&)>;

	/** @name Scene Hook Setters */
	/// @{
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

	/**
	 * @brief Runs the configured post-level-load setup hook immediately.
	 * @param activeSimulation Whether the freshly loaded scene should be treated as gameplay-active.
	 */
	void RunPostLevelLoadSetup(bool activeSimulation);
	void SetCutsceneFadeOutHook(CutsceneFadeOutHook hook) {
		cutsceneFadeOutHook_ = std::move(hook);
	}
	void SetCutsceneFirstFrameHook(CutsceneFirstFrameHook hook) {
		cutsceneFirstFrameHook_ = std::move(hook);
	}
	void SetCutsceneBeforeFinalLoadHook(CutsceneBeforeFinalLoadHook hook) {
		cutsceneBeforeFinalLoadHook_ = std::move(hook);
	}
	void SetNavigationBlockerCollector(NavigationBlockerCollector collector) {
		navigationBlockerCollector_ = std::move(collector);
	}
	void SetSkipCutsceneAudioHook(SkipCutsceneAudioHook hook) {
		skipCutsceneAudioHook_ = std::move(hook);
	}
	void SetTagUsesVelocityHook(TagUsesVelocityHook hook) {
		tagUsesVelocityHook_ = std::move(hook);
	}
	void SetPauseOverlayAudioChannels(std::string musicChannel, std::string ambienceChannel) {
		pauseMusicChannel_ = std::move(musicChannel);
		pauseAmbienceChannel_ = std::move(ambienceChannel);
	}
	void SetPauseOverlayAdditionalAudioChannels(std::vector<std::string> channels) {
		pauseAdditionalChannels_ = std::move(channels);
	}
	/// @}

	// -------------------------------------------------------------------------------------------------
	// Object Metadata, Identity, And Visibility
	// -------------------------------------------------------------------------------------------------

	/**
	 * @brief Applies runtime object setup.
	 * @param id Parameter for id.
	 * @param tag Parameter for tag.
	 * @param texturePath Parameter for texture path.
	 * @param animated Parameter for animated.
	 * @param animName Parameter for anim name.
	 * @param speedX Parameter for speed x.
	 * @param speedY Parameter for speed y.
	 */
	void ApplyRuntimeObjectSetup(int id, const std::string& tag, const std::string& texturePath, bool animated, const std::string& animName, float speedX, float speedY) {
		if (runtimeObjectSetupHook_) {
			runtimeObjectSetupHook_(*this, id, tag, texturePath, animated, animName, speedX, speedY);
		}
	}

	/**
	 * @brief Sets object tag.
	 * @param id Parameter for id.
	 * @param tag Parameter for tag.
	 */
	void SetObjectTag(int id, const std::string& tag);

	/**
	 * @brief Returns object tag.
	 * @param id Parameter for id.
	 * @return Requested value.
	 */
	std::string GetObjectTag(int id) const;

	/**
	 * @brief Applies tag rules.
	 * @param id Parameter for id.
	 * @param tag Parameter for tag.
	 * @param speedX Parameter for speed x.
	 * @param speedY Parameter for speed y.
	 */
	void ApplyTagRules(int id, const std::string& tag, float speedX, float speedY);

	/**
	 * @brief Returns whether tagusesvelocity.
	 * @param tag Parameter for tag.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool TagUsesVelocity(const std::string& tag) const;

	/**
	 * @brief Sets player id.
	 * @param id Parameter for id.
	 */
	void SetPlayerID(int id);

	/**
	 * @brief Returns player id.
	 * @return Requested value.
	 */
	int GetPlayerID() const {
		return spriteID;
	}

	/**
	 * @brief Sets npc1 id.
	 * @param id Parameter for id.
	 */
	void SetNPC1ID(int id) {
		otherID = id;
		if (id >= 0) {
			npcSystem.RegisterLaneNPC(id, 1000.0f); // Only this npc1 gets lane behavior
		}
	}

	/**
	 * @brief Sets npc2 id.
	 * @param id Parameter for id.
	 */
	void SetNPC2ID(int id) {
		otherID2 = id;
		if (id >= 0) {
			npcSystem.RegisterLaneNPC(id, 1000.0f); //Only this npc2 gets lane behavior
		}
	}

	/**
	 * @brief Sets dino id.
	 * @param id Parameter for id.
	 */
	void SetDinoID(int id) {
		dinoID = id;
	}

	/**
	 * @brief Returns npc1 id.
	 * @return Requested value.
	 */
	int GetNPC1ID() const {
		return otherID;
	}

	/**
	 * @brief Returns npc2 id.
	 * @return Requested value.
	 */
	int GetNPC2ID() const {
		return otherID2;
	}

	/**
	 * @brief Returns dino id.
	 * @return Requested value.
	 */
	int GetDinoID() const {
		return dinoID;
	}

	/**
	 * @brief Sets npcvelocity.
	 * @param id Parameter for id.
	 * @param vx Parameter for vx.
	 * @param vy Parameter for vy.
	 */
	void SetNPCVelocity(int id, float vx, float vy) {
		npcSystem.SetNPCVelocity(id, glm::vec2(vx, vy));
	}

	/**
	 * @brief Returns npcvelocity.
	 * @param id Parameter for id.
	 * @return Requested value.
	 */
	glm::vec2 GetNPCVelocity(int id) const {
		return npcSystem.GetNPCVelocity(id);
	}

	/**
	 * @brief Registers lane npc.
	 * @param id Parameter for id.
	 * @param laneX Parameter for lane x.
	 */
	void RegisterLaneNPC(int id, float laneX) {
		npcSystem.RegisterLaneNPC(id, laneX);
	}

	/**
	 * @brief Registers exit gate.
	 * @param id Parameter for id.
	 */
	void RegisterExitGate(int id) {
		exitGateID_ = id;
		exitGateCached_ = false;
	}

	/**
	 * @brief Returns exit gate world pos.
	 * @return Requested value.
	 */
	Math::Vector2D GetExitGateWorldPos() {
		if (exitGateID_ < 0) return { 0.f, 0.f };
		if (GameObject* g = GetGameObjectByID(exitGateID_)) {
			auto p = g->GetPositionGLM();
			return { p.x, p.y };
		}
		return { 0.f, 0.f };
	}

	/**
	 * @brief Returns object texture path.
	 * @param id Parameter for id.
	 * @return Requested value.
	 */
	const std::string& GetObjectTexturePath(int id) const;

	/**
	 * @brief Sets object texture path.
	 * @param id Parameter for id.
	 * @param path Path to process.
	 */
	void SetObjectTexturePath(int id, const std::string& path);

	using Defaults = SceneObjectDefaults;

	/**
	 * @brief Sets defaults.
	 * @param id Parameter for id.
	 * @param d Parameter for d.
	 */
	void SetDefaults(int id, const Defaults& d) {
		objectMetadata_.SetDefaults(id, d);
		collisionManager.MarkStaticStateDirty();
	}

	/**
	 * @brief Returns defaults.
	 * @param id Parameter for id.
	 * @return Requested value.
	 */
	Defaults GetDefaults(int id) const {
		return objectMetadata_.GetDefaults(id);
	}

	/**
	 * @brief Sets object visible.
	 * @param id Parameter for id.
	 * @param visible Parameter for visible.
	 */
	void SetObjectVisible(int id, bool visible) {
		objectMetadata_.SetVisible(id, visible);
		collisionManager.MarkStaticStateDirty();
	}

	/**
	 * @brief Returns whether object visible.
	 * @param id Parameter for id.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsObjectVisible(int id) const {
		return objectMetadata_.IsVisible(id);
	}

	// -------------------------------------------------------------------------------------------------
	// Layers, Collision, Navigation, And Runtime Systems
	// -------------------------------------------------------------------------------------------------

	// Particle system
	ParticleSystem particleSystem_;

	/**
	 * @brief Returns particle system.
	 * @return Requested value.
	 */
	ParticleSystem& GetParticleSystem() {
		return particleSystem_;
	}

	/**
	 * @brief Adds layer.
	 * @param name Parameter for name.
	 */
	void AddLayer(const std::string& name);

	/**
	 * @brief Returns layer.
	 * @param name Parameter for name.
	 * @return Requested value.
	 */
	Layer* GetLayer(const std::string& name);

	/**
	 * @brief Returns all layers.
	 * @return Requested value.
	 */
	const std::unordered_map<std::string, Layer>& GetAllLayers() const;

	/**
	 * @brief Returns object layer.
	 * @param objectID Identifier of the target object.
	 * @return Requested value.
	 */
	std::string GetObjectLayer(int objectID) const;

	/**
	 * @brief Returns object layer ptr.
	 * @param objectID Identifier of the target object.
	 * @return Requested value.
	 */
	Layer* GetObjectLayerPtr(int objectID);

	/**
	 * @brief Returns object layer ptr.
	 * @param objectID Identifier of the target object.
	 * @return Requested value.
	 */
	const Layer* GetObjectLayerPtr(int objectID) const;

	/**
	 * @brief Performs assign object to layer.
	 * @param id Parameter for id.
	 * @param newLayer Parameter for new layer.
	 */
	void AssignObjectToLayer(int id, const std::string& newLayer);

	/**
	 * @brief Removes layer.
	 * @param name Parameter for name.
	 */
	void RemoveLayer(const std::string& name);

	/**
	 * @brief Returns whether layer enabled.
	 * @param layerName Parameter for layer name.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsLayerEnabled(const std::string& layerName) const;

	/**
	 * @brief Returns whether object layer enabled.
	 * @param objectID Identifier of the target object.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsObjectLayerEnabled(int objectID) const;

	/**
	 * @brief Builds level colliders.
	 */
	void BuildLevelColliders();

	/**
	 * @brief Performs rebuild colliders.
	 */
	void RebuildColliders();

	/**
	 * @brief Resolves initial static overlaps.
	 */
	void ResolveInitialStaticOverlaps();

	/**
	 * @brief Returns walk area.
	 * @return Requested value.
	 */
	collision::WalkArea GetWalkArea() const;

	/**
	 * @brief Handles player collisions.
	 * @param deltaTime Frame delta time in seconds.
	 * @param entityMgr Parameter for entity mgr.
	 */
	void HandlePlayerCollisions(float deltaTime, EntityManager& entityMgr);

	/**
	 * @brief Applies final constraints.
	 * @param entityMgr Parameter for entity mgr.
	 */
	void ApplyFinalConstraints(EntityManager& entityMgr);

	/**
	 * @brief Collects navigation blocker boxes.
	 * @param moverObjectID Parameter for mover object id.
	 * @param outBoxes Output value for out boxes.
	 */
	void CollectNavigationBlockerBoxes(int moverObjectID, std::vector<collision::AABB>& outBoxes);

	/**
	 * @brief Returns entity manager.
	 * @return Requested value.
	 */
	EntityManager& GetEntityManager() {
		return entityManager;
	}

	// -------------------------------------------------------------------------------------------------
	// Flow State, Pause, Replay, And Cutscene Control
	// -------------------------------------------------------------------------------------------------

	/**
	 * @brief Performs queue level load.
	 * @param path Path to process.
	 * @param activateSimulation Parameter for activate simulation.
	 */
	void QueueLevelLoad(const std::string& path, bool activateSimulation);

	/**
	 * @brief Sets current level path.
	 * @param path Path to process.
	 */
	void SetCurrentLevelPath(const std::string& path) {
		currentLevelPath_ = path;
	}

	/**
	 * @brief Returns current level path.
	 * @return Requested value.
	 */
	const std::string& GetCurrentLevelPath() const {
		return currentLevelPath_;
	}

	/**
	 * @brief Returns whether pending level.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasPendingLevel() const {
		return pendingLevelLoad_.has_value();
	}

	/**
	 * @brief Performs request state change.
	 * @param newState Parameter for new state.
	 */
	void RequestStateChange(Framework::GameState newState);

	/**
	 * @brief Queues a transition back to the main menu state.
	 */
	void RequestMainMenuStateChange();

	/**
	 * @brief Queues a transition into the tutorial state.
	 */
	void RequestTutorialStateChange();

	/**
	 * @brief Queues a transition into the first kitchen gameplay state.
	 */
	void RequestKitchen01StateChange();

	/**
	 * @brief Returns whether pending state change.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasPendingStateChange() const {
		return hasPendingStateChange_;
	}

	/**
	 * @brief Returns pending state.
	 * @return Requested value.
	 */
	Framework::GameState GetPendingState() const {
		return pendingState_;
	}

	/**
	 * @brief Returns and clears the currently queued state transition if one exists.
	 * @return Pending state change when queued; otherwise no value.
	 */
	std::optional<Framework::GameState> ConsumePendingStateChange();

	/**
	 * @brief Performs show pause overlay.
	 */
	void ShowPauseOverlay();

	/**
	 * @brief Performs hide pause overlay.
	 */
	void HidePauseOverlay();

	/**
	 * @brief Performs request resume from pause overlay.
	 */
	void RequestResumeFromPauseOverlay();

	/**
	 * @brief Returns whether pause overlay active.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsPauseOverlayActive() const {
		return pauseOverlayActive_;
	}

	/**
	 * @brief Returns whether replay playback active.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsReplayPlaybackActive() const {
		return replayManager_.IsPlaybackActive();
	}

	/**
	 * @brief Returns replay frame dt.
	 * @return Requested value.
	 */
	float GetReplayFrameDt() const {
		return lastReplayFrameDt_;
	}

	/**
	 * @brief Returns whether replay frame dt.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasReplayFrameDt() const {
		return lastReplayFrameDt_ > 0.0f;
	}

	/**
	 * @brief Sets how to play overlay active.
	 * @param active Parameter for active.
	 */
	void SetHowToPlayOverlayActive(bool active) {
		howToPlayOverlayActive_ = active;
	}

	/**
	 * @brief Returns whether how to play overlay active.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsHowToPlayOverlayActive() const {
		return howToPlayOverlayActive_;
	}

	/**
	 * @brief Returns whether runtime behavior should match the release build.
	 * @return True in release, or in debug when the editor is hidden or actively simulating.
	 */
	bool ShouldUseRuntimeParityMode() const {
#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
		const bool editorVisible = editorEnabledQuery_ && editorEnabledQuery_();
		const bool editorPlaying = editorPlayingQuery_ && editorPlayingQuery_();
		return !editorVisible || editorPlaying;
#else
		return true;
#endif
	}

	/**
	 * @brief Sets whether a menu-owned modal popup is currently blocking other menu input.
	 * @param active Parameter for active.
	 */
	void SetMenuModalActive(bool active) {
		menuModalActive_ = active;
	}

	/**
	 * @brief Returns whether a menu-owned modal popup is blocking other menu input.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsMenuModalActive() const {
		return menuModalActive_;
	}

	/**
	 * @brief Renders fpstext.
	 */
	void RenderFPSText();

	/**
	 * @brief Performs request despawn.
	 * @param id Parameter for id.
	 */
	void RequestDespawn(int id) {
		pendingDespawns_.push_back(id);
	}

	// Cutscene API
	// Starts a cutscene consisting of image paths played in sequence.
	/**
	 * @brief Performs start cutscene.
	 * @param imagePaths Parameter for image paths.
	 * @param holdSecondsPerImage Parameter for hold seconds per image.
	 * @param fadeSeconds Parameter for fade seconds.
	 * @param levelJsonPath Parameter for level json path.
	 * @param activateSimulation Parameter for activate simulation.
	 */
	void StartCutscene(const std::vector<std::string>& imagePaths,
		float holdSecondsPerImage,
		float fadeSeconds,
		const std::string& levelJsonPath,
		bool activateSimulation);

	/**
	 * @brief Performs start cutscene transitioned.
	 * @param imagePaths Parameter for image paths.
	 * @param levelJsonPath Parameter for level json path.
	 * @param activateSimulation Parameter for activate simulation.
	 * @param fadeOutSeconds Parameter for fade out seconds.
	 * @param fadeInSeconds Parameter for fade in seconds.
	 * @param holdSeconds Parameter for hold seconds.
	 * @param crossfadeFromIndex Parameter for crossfade from index.
	 * @param crossfadeSeconds Parameter for crossfade seconds.
	 */
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
	/**
	 * @brief Performs start cutscene transitioned bounded.
	 * @param imagePaths Parameter for image paths.
	 * @param boundaryFlags Parameter for boundary flags.
	 * @param levelJsonPath Parameter for level json path.
	 * @param activateSimulation Parameter for activate simulation.
	 * @param fadeOutSeconds Parameter for fade out seconds.
	 * @param fadeInSeconds Parameter for fade in seconds.
	 * @param holdSeconds Parameter for hold seconds.
	 * @param crossfadeFromIndex Parameter for crossfade from index.
	 * @param crossfadeSeconds Parameter for crossfade seconds.
	 */
	void StartCutsceneTransitionedBounded(const std::vector<std::string>& imagePaths,
		const std::vector<bool>& boundaryFlags,
		const std::string& levelJsonPath,
		bool activateSimulation,
		float fadeOutSeconds = 0.35f,
		float fadeInSeconds = 0.35f,
		float holdSeconds = 1.0f / 12.0f, // default 12 FPS
		int crossfadeFromIndex = -1,
		float crossfadeSeconds = 0.75f);

	/**
	 * @brief Starts a fullscreen MP4/video cutscene rendered through a streaming texture.
	 * @param videoPath Path to the video asset that should be played.
	 * @param levelJsonPath Level JSON path queued once playback completes or is skipped.
	 * @param activateSimulation Whether the follow-up level should resume simulation.
	 * @param loop Whether the video should restart automatically after ending.
	 */
	void StartVideoCutscene(const std::string& videoPath,
		const std::string& levelJsonPath,
		bool activateSimulation,
		bool loop = false);

	/**
	 * @brief Starts a fade-to-black transition, then begins a fullscreen video cutscene at blackout.
	 * @param videoPath Path to the video asset that should be played.
	 * @param levelJsonPath Level JSON path queued once playback completes or is skipped.
	 * @param activateSimulation Whether the follow-up level should resume simulation.
	 * @param loop Whether the video should restart automatically after ending.
	 * @param fadeOutSeconds Seconds spent fading out to blackout before the video appears.
	 * @param fadeInSeconds Seconds spent fading back in over the video once playback begins.
	 */
	void StartVideoCutsceneTransitioned(const std::string& videoPath,
		const std::string& levelJsonPath,
		bool activateSimulation,
		bool loop = false,
		float fadeOutSeconds = 0.35f,
		float fadeInSeconds = 0.35f);

	// Order UI slide-in API
	// Spawns an Order UI sprite off-screen at the top, then animates it sliding down to target.
	/**
	 * @brief Triggers order ui slide in.
	 * @param targetPos Parameter for target pos.
	 * @param size Parameter for size.
	 * @param layer Parameter for layer.
	 * @param texturePath Parameter for texture path.
	 * @param slideDuration Parameter for slide duration.
	 * @return Result produced by this operation.
	 */
	int TriggerOrderUiSlideIn(const glm::vec2& targetPos,
		const glm::vec2& size,
		const std::string& layer = "3",
		const std::string& texturePath = "../assets/UI/Order_UI.png",
		float slideDuration = 0.45f);

	/**
	 * @brief Returns whether any cutscene active.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsAnyCutsceneActive() const {
		return cutscene_.active || cutTrans_.active || videoCutscene_.active;
	}

	/**
	 * @brief Returns flow state.
	 * @return Requested value.
	 */
	FlowState GetFlowState() const {
		return flowState_;
	}

	/**
	 * @brief Returns flow state name.
	 * @return Requested value.
	 */
	const char* GetFlowStateName() const;

	/**
	 * @brief Performs skip active cutscene.
	 */
	void SkipActiveCutscene();

	/**
	 * @brief Advances the active video cutscene player and updates its sprite texture.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateVideoCutscene(float dt);

	/**
	 * @brief Advances a pending fade-into-video request until blackout is reached.
	 */
	void UpdatePendingVideoCutsceneTransition();

private:
	// -------------------------------------------------------------------------------------------------
	// Core References And Shared Systems
	// -------------------------------------------------------------------------------------------------

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
	CoreFramework::MessageBus* messageBus_ = nullptr;

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

	// Centralized runtime metadata for objects, keyed by object ID.
	SceneObjectMetadataStore objectMetadata_;
	std::unordered_map<std::string, Layer> layers;

	/**
	 * @brief Returns layer sort key cached.
	 * @param layerName Parameter for layer name.
	 * @return Requested value.
	 */
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
	std::string sceneBackgroundPath_;
	std::string sceneBackgroundOverlayPath_;

	// Pending level load state
	struct DeferredLevelLoadRequest {
		std::string path;
		bool activateSimulation = false;
	};
	std::optional<DeferredLevelLoadRequest> pendingLevelLoad_;

	// Pending game state change
	Framework::GameState pendingState_ = Framework::GameState::MainMenu;
	bool hasPendingStateChange_ = false;

	// Pause overlay state
	bool pauseOverlayActive_ = false;
	bool resumeFromPausePending_ = false;
	std::vector<int> pauseOverlayObjectIds_;
	FlowState flowState_ = FlowState::Bootstrapping;
	FlowState flowStateBeforePause_ = FlowState::Gameplay;
	std::vector<RuntimeTextData> runtimeTextObjects_;
	std::unordered_set<std::string> pauseSuppressedRuntimeTextNames_;
	std::unordered_set<std::string> editorPreservedRuntimeTextNames_;

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
	NavigationBlockerCollector navigationBlockerCollector_;
	SkipCutsceneAudioHook skipCutsceneAudioHook_;
	TagUsesVelocityHook tagUsesVelocityHook_;
	std::string pauseMusicChannel_;
	std::string pauseAmbienceChannel_;
	std::vector<std::string> pauseAdditionalChannels_;

	bool howToPlayOverlayActive_ = false;
	bool menuModalActive_ = false;

	std::vector<int> pendingDespawns_;

	// Prevent repeated skip-trigger while space is held during cutscenes.
	bool cutsceneSkipSpaceHeld_ = false;
	// Ensure skip only runs once for the currently active cutscene sequence.
	bool cutsceneSkipConsumed_ = false;

	std::function<void(Scene&)> editorUiHook_;
	std::function<void()> editorToggleHook_;
	std::function<bool()> editorEnabledQuery_;
	std::function<bool()> editorPlayingQuery_;

	// -------------------------------------------------------------------------------------------------
	// Cutscene, Transition, And Runtime UI Animation State
	// -------------------------------------------------------------------------------------------------

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

	struct VideoCutsceneState {
		bool active = false;
		std::string videoPath;
		std::string uiLayer = "999998";
		int spriteId = -1;
		std::string targetLevelJson;
		bool targetActivateSim = true;
		bool queuedFinalLoad = false;
		bool loop = false;
		VideoPlayer player;
	} videoCutscene_;

	struct PendingVideoCutsceneTransition {
		bool active = false;
		bool awaitingBlackout = false;
		std::string videoPath;
		std::string targetLevelJson;
		bool targetActivateSim = true;
		bool loop = false;
		float fadeOutSeconds = 0.35f;
		float fadeInSeconds = 0.35f;
	} pendingVideoCutsceneTransition_;

	// Runtime ID for top-right "press space to skip" cutscene sprite
	int cutsceneSkipPromptId_ = -1;

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

	/**
	 * @brief Updates cutscene.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateCutscene(float dt);

	/**
	 * @brief Updates cutscene transitioned.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateCutsceneTransitioned(float dt);

	/**
	 * @brief Performs cleanup cutscene objects.
	 */
	void CleanupCutsceneObjects();

	/**
	 * @brief Sets sprite alpha.
	 * @param obj Parameter for obj.
	 * @param alpha Parameter for alpha.
	 */
	void SetSpriteAlpha(GameObject* obj, float alpha); // no-op if shader lacks alpha tint

	/**
	 * @brief Performs spawn cutscene skip prompt.
	 */
	void SpawnCutsceneSkipPrompt();

	/**
	 * @brief Performs despawn cutscene skip prompt.
	 */
	void DespawnCutsceneSkipPrompt();

	/**
	 * @brief Updates ui slides.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateUiSlides(float dt);

	/**
	 * @brief Resets object-backed scene state that should not survive a level rebuild.
	 */
	void ResetLevelObjectState();

	/**
	 * @brief Sets flow state.
	 * @param newState Parameter for new state.
	 */
	void SetFlowState(FlowState newState);

	/**
	 * @brief Refreshes flow state from current scene flags.
	 */
	void RefreshFlowState();

	/**
	 * @brief Returns fallback steady flow state.
	 * @return Requested value.
	 */
	FlowState ComputeSteadyFlowState() const;

	/**
	 * @brief Performs ease out cubic.
	 * @param x Parameter for x.
	 * @return Result produced by this operation.
	 */
	static float EaseOutCubic(float x) {
		float inv = 1.0f - x;
		return 1.0f - inv * inv * inv;
	}

public:
	// -------------------------------------------------------------------------------------------------
	// Advanced Navigation And Runtime Feedback API
	// -------------------------------------------------------------------------------------------------

	/**
	 * @brief Performs start level transition.
	 * @param levelJsonPath Parameter for level json path.
	 * @param activateSimulation Parameter for activate simulation.
	 * @param fadeOutSeconds Parameter for fade out seconds.
	 * @param fadeInSeconds Parameter for fade in seconds.
	 */
	void StartLevelTransition(const std::string& levelJsonPath,
		bool activateSimulation,
		float fadeOutSeconds = 0.35f,
		float fadeInSeconds = 0.35f);

	/**
	 * @brief Builds navigation grid for object.
	 * @param moverObjectID Parameter for mover object id.
	 * @param outGrid Output value for out grid.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool BuildNavigationGridForObject(int moverObjectID, NavGrid& outGrid);

	/**
	 * @brief Finds path for object.
	 * @param moverObjectID Parameter for mover object id.
	 * @param startWorld Parameter for start world.
	 * @param goalWorld Parameter for goal world.
	 * @param outPath Output value for out path.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool FindPathForObject(int moverObjectID,
		const glm::vec2& startWorld,
		const glm::vec2& goalWorld,
		std::vector<glm::vec2>& outPath);

	/**
	 * @brief Returns nearest navigation cell center for object.
	 * @param moverObjectID Parameter for mover object id.
	 * @param worldPos Parameter for world pos.
	 * @param outCenter Output value for out center.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool GetNearestNavigationCellCenterForObject(int moverObjectID,
		const glm::vec2& worldPos,
		glm::vec2& outCenter);

	/**
	 * @brief Returns whether direct path for object.
	 * @param moverObjectID Parameter for mover object id.
	 * @param startWorld Parameter for start world.
	 * @param goalWorld Parameter for goal world.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasDirectPathForObject(int moverObjectID,
		const glm::vec2& startWorld,
		const glm::vec2& goalWorld);

	/**
	 * @brief Triggers customer payment feedback.
	 * @param tableObjectID Parameter for table object id.
	 * @param amount Parameter for amount.
	 */
	void TriggerCustomerPaymentFeedback(int tableObjectID, int amount);

	/**
	 * @brief Triggers the shared star-burst hover feedback for blue UI buttons.
	 * @param buttonObjectID Identifier of the hovered button.
	 */
	void TriggerUiButtonHoverFeedback(int buttonObjectID);

private:
	// -------------------------------------------------------------------------------------------------
	// Runtime Effects And Per-Frame Internal Helpers
	// -------------------------------------------------------------------------------------------------

	// Simpler version of cutscene transition for level changes without per-frame images. Reuses some cutTrans_ state for convenience.
	struct LevelTrans {
		bool active = false;
		bool awaitingBlackout = false;
		std::string targetJson;
		bool targetActivateSim = false;
		float outSec = 0.35f;
		float inSec = 0.35f;
	} levelTrans_;

	struct RuntimeAnimatedFx {
		int objectId = -1;
		glm::vec3 baseScale{ 1.0f, 1.0f, 1.0f };
		float elapsed = 0.0f;
		float lifetime = 0.0f;
		bool animateScale = true;
	};

	struct FloatingWorldTextFx {
		std::string text;
		glm::vec2 pos{ 0.0f, 0.0f };
		glm::vec2 velocity{ 0.0f, -55.0f }; // negative Y = move up in your game
		glm::vec4 color{ 1.0f, 0.92f, 0.30f, 1.0f };
		float baseScale = 1.0f;
		float elapsed = 0.0f;
		float lifetime = 0.9f;
		std::string layer = "6";
	};

	std::vector<RuntimeAnimatedFx> runtimeAnimatedFx_;
	std::vector<FloatingWorldTextFx> floatingWorldTextFx_;

	/**
	 * @brief Updates runtime animated fx.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateRuntimeAnimatedFx(float dt);

	/**
	 * @brief Updates floating world text fx.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateFloatingWorldTextFx(float dt);

	/**
	 * @brief Renders floating world text fx.
	 * @param projection Parameter for projection.
	 * @param pauseActive Parameter for pause active.
	 * @param cutsceneActive Parameter for cutscene active.
	 */
	void RenderFloatingWorldTextFx(const glm::mat4& projection, bool pauseActive, bool cutsceneActive);

	/**
	 * @brief Updates level transition.
	 */
	void UpdateLevelTransition();

	/**
	 * @brief Updates cutscene phase.
	 * @param deltaTime Frame delta time in seconds.
	 */
	void UpdateCutscenePhase(float deltaTime);

	/**
	 * @brief Updates input phase.
	 * @param deltaTime Frame delta time in seconds.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool UpdateInputPhase(float deltaTime);

	/**
	 * @brief Updates simulation phase.
	 * @param deltaTime Frame delta time in seconds.
	 * @param physicsDt Parameter for physics dt.
	 */
	void UpdateSimulationPhase(float deltaTime, float physicsDt);

	/**
	 * @brief Handles deferred loads.
	 */
	void HandleDeferredLoads();

	/**
	 * @brief Updates ui phase.
	 * @param deltaTime Frame delta time in seconds.
	 * @param window Parameter for window.
	 */
	void UpdateUiPhase(float deltaTime, GLFWwindow* window);

	/**
	 * @brief Performs finalize frame phase.
	 * @param deltaTime Frame delta time in seconds.
	 */
	void FinalizeFramePhase(float deltaTime);
};
