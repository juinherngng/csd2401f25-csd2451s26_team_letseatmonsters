/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Declares the SceneManager (Scene) class, which orchestrates the lifecycle and
					high-level coordination of all major systems within a game scene. This includes:
					entity creation and management, event handling, physics and collision simulation,
					animation control, input processing, and rendering pipeline integration.

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

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

#include "AnimationManager.hpp"
#include "Animator.hpp"
#include "EntityManager.hpp"
#include "GraphicsEngine.hpp"
#include "Layer.hpp"

 /**
  * @class Scene
  * @brief Manages the lifecycle of a game scene, including objects, animations, and input.
  */
class Scene {
public:
	// Core Lifecycle testing

	/**
	 * @brief Construct a new Scene object.
	 * @param engine Reference to the graphics engine used for rendering.
	 * @param inputMgr Reference to the input manager system.
	 * @param animMgr Reference to the animation manager system.
	 * @param moveMgr Reference to the movement manager system.
	 * @param physicsMgr Reference to the physics manager system.
	 * @param collisionMgr Reference to the collision manager system.
	*/
	Scene(GraphicsEngine& engine, InputManager& inputMgr, AnimationManager& animMgr,
		  MovementManager& moveMgr, PhysicsManager& physicsMgr, CollisionManager& collisionMgr);

	// Set AudioManager for UI sounds
	void SetAudioManager(AudioManager* audioMgr) { audioManager_ = audioMgr; }

	void LoadScene(const std::string& sceneName);
	void Update(float deltaTime, GLFWwindow* window);

	void DrawUI();
	void ClearAll();
	void RequestClearAll();

	GraphicsEngine& GetGraphicsEngine();
	const GraphicsEngine& GetGraphicsEngine() const;

	// Simulation control
	void SetSimulationActive(bool active);
	bool IsSimulationActive() const;

	void ResetResizeBaseline();
	float GetLastPhysicsDt() const {
		return lastPhysicsDt_;
	}
	const physics::StepController& GetStepController() const {
		return physicsStep_;
	}

	// Spawning / object management

	/**
	  * @brief Spawns a static sprite with a given texture and size.
	  */
	GameObject* SpawnStaticSprite(const std::string& texturePath,
								  const glm::vec3 position,
								  const glm::vec2 size = glm::vec2(100.0f, 100.0f),
								  const std::string& layer = "Not set in JSON");

	/**
	 * @brief Spawns an animated sprite with frames and timing.
	 */
	GameObject* SpawnAnimatedSprite(const std::string& texturePath,
									const glm::vec3 position,
									const glm::vec2 size,
									const std::vector<glm::vec4> frames,
									float frameDuration, bool loop,
									const std::string& layer);

	GameObject* GetGameObjectByID(int targetID);
	std::vector<GameObject*> GetAllObjectsRaw();
	void DespawnByID(int targetID);
	void CollectRenderablePointers(std::vector<GameObject*>& out);

	// Scene / transform utilities

	/**
	 * @brief Set the background texture for the scene.
	 */
	void SetSceneBackground(const std::string& texturePath);

	void SetTransformFromLevel(int id, const glm::vec3& pos, const glm::vec3& scale, float rotation);
	void ClampToWalkArea(GameObject* obj);
	glm::vec2 ResolveWorldStep(GameObject* obj, const glm::vec2& desiredDelta);

	float ScaleXToCurrent(float referenceX) const;
	float ScaleYToCurrent(float referenceY) const;
	float ToRefX(float currentX) const;
	float ToRefY(float currentY) const;

	// Animation helpers
	bool HasAnimations(int id) const;
	std::vector<std::string> GetAnimationList(int id) const;
	std::string GetCurrentAnimationName(int id) const;

	void SetAnimation(int objID, const std::string& newAnim);
	void AttachDinoAnimations(int objID);
	void MarkAnimated(int id, bool state);

	void GenerateStressTest(int objectCount = 2500);
	void UpdateAnimationControls();

	// Tag-based logic helpers
	void AttachLogicForTag(int id, const std::string& tag);

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

	// Texture metadata (LevelEditor / JSON)
	const std::string& GetObjectTexturePath(int id) const;
	void SetObjectTexturePath(int id, const std::string& path);

	struct Defaults {
		glm::vec3 pos{ 0,0,0 };
		glm::vec2 size{ 128,128 };
		float rot{ 0.f };
		glm::vec2 colSize{ 64,128 };
		glm::vec2 colOff{ 0,0 };
		glm::vec2 vel{ 0,0 };
		std::string texture;
		std::string tag;
		std::string layer;
	};

	void SetDefaults(int id, const Defaults& d) {
		defaults_[id] = d;
	}
	Defaults GetDefaults(int id) const {
		auto it = defaults_.find(id);
		return (it != defaults_.end())?it->second:Defaults{};
	}

	// Layers
	void AddLayer(const std::string& name);
	Layer* GetLayer(const std::string& name);
	const std::unordered_map<std::string, Layer>& GetAllLayers() const;

	std::string GetObjectLayer(int objectID) const;
	void AssignObjectToLayer(int id, const std::string& newLayer);
	void RemoveLayer(const std::string& name);

	// World / collision rebuilds
	void BuildLevelColliders();
	void RebuildColliders();
	void ResolveInitialStaticOverlaps();

	collision::WalkArea GetWalkArea() const;
	void HandlePlayerCollisions(float deltaTime, EntityManager& entityMgr);
	void ApplyFinalConstraints(EntityManager& entityMgr);

	// Logic system access
	LogicManager& GetLogicManager() {
		return logicManager;
	}

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
	bool showAuxDebug_ = true;

	int spriteID = -1;
	int dinoID = -1;
	int otherID = -1;
	int otherID2 = -1;

	std::unordered_map<int, Defaults> defaults_;
	std::unordered_map<std::string, Layer> layers;

	int lastWidth_ = -1;
	int lastHeight_ = -1;
	bool resetBaseline_ = false;

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

	LevelEditor mLevelEditor;
	std::unordered_map<int, std::string> mTexturePathByID;
};
