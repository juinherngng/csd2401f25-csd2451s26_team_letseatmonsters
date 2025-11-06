/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Declares the Scene class responsible for managing game objects,
					animations, and scene updates.

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "GraphicsEngine.hpp"
#include "Animator.hpp"
#include "EntityManager.hpp"
#include "AnimationManager.hpp"

#include "../Core/CollisionManager.hpp"
#include "../Core/MovementManager.hpp" 
#include "../Core/InputManager.hpp"
#include "../Core/PhysicsManager.hpp"
#include "../Core/Physics.hpp"
#include "../Core/Math.hpp"
#include "../Core/LevelEditor.hpp"
#include "../Core/InputCommandHandler.hpp"
#include "../Core/PlayerController.hpp"
#include "../Core/NPCSystem.hpp"
#include "../Core/DebugVisualizer.hpp"
#include "../Core/LogicManager.hpp"
#include "../Core/PlayerLogic.hpp"
#include "../Core/SimpleNpcLogic.hpp"


#include <string>
#include <vector>
#include <unordered_map>

 /**
  * @class Scene
  * @brief Manages the lifecycle of a game scene, including objects, animations, and input.
  */
class Scene {
public:
	// Core Lifecycle testing

	GraphicsEngine& GetGraphicsEngine();
	const GraphicsEngine& GetGraphicsEngine() const;

	/**
	 * @brief Construct a new Scene object.
	 * @param engine Reference to the graphics engine used for rendering.
	 */
	Scene(GraphicsEngine& engine);

	/**
	 * @brief Load a scene by name (dispatches to test scene for now).
	 * @param sceneName Name of the scene.
	 */
	void LoadScene(const std::string& sceneName);

	/**
	 * @brief Per-frame update function to update input, animations, physics, and rendering.
	 * @param deltaTime Time step for this frame.
	 * @param window Active GLFW window for input.
	 */
	void Update(float deltaTime, GLFWwindow* window);

	// Stress Test Generation
	void GenerateStressTest(int objectCount = 2500);

	void DrawUI();
	void ClearAll();

	// Spawning / Object Management

	/**
	 * @brief Spawns a triangle mesh object.
	 * @param position Position in world space.
	 * @param scale Scaling vector.
	 * @param rotation Rotation in degrees.
	 * @return Pointer to spawned GameObject, or nullptr if failed.
	 */
	 //GameObject* SpawnTriangle(const glm::vec3 position, const glm::vec3 scale, float rotation = 0.0f);

	 /**
	  * @brief Spawns a static sprite with a given texture and size.
	  */
	GameObject* SpawnStaticSprite(const std::string& texturePath, const glm::vec3 position,
		const glm::vec2 size = glm::vec2(100.0f, 100.0f));

	/**
	 * @brief Spawns an animated sprite with frames and timing.
	 */
	GameObject* SpawnAnimatedSprite(
		const std::string& texturePath,
		const glm::vec3 position,
		const glm::vec2 size,
		const std::vector<glm::vec4> frames,
		float frameDuration, bool loop);

	/**
	 * @brief Retrieve a game object by its ID.
	 */
	GameObject* GetGameObjectByID(int targetID);
	std::vector<GameObject*> GetAllObjectsRaw();

	/**
	 * @brief Remove a game object by its ID, including its animations.
	 */
	void DespawnByID(int targetID);

	/**
	 * @brief Collect raw pointers to all renderable game objects.
	 */
	void CollectRenderablePointers(std::vector<GameObject*>& out);

	// Scene / Transform Utilities

	/**
	 * @brief Set the background texture for the scene.
	 */
	void SetSceneBackground(const std::string& texturePath);

	// Set initial transform into the scene maps and the GameObject
	void SetTransformFromLevel(int id, const glm::vec3& pos, const glm::vec3& scale, float rotation);
	void ClampToWalkArea(GameObject* obj);
	const std::string& GetObjectTexturePath(int id) const;
	void SetObjectTexturePath(int id, const std::string& path);

	// Animation
	bool HasAnimations(int id) const;
	std::vector<std::string> GetAnimationList(int id) const;
	std::string GetCurrentAnimationName(int id) const;

	/**
	 * @brief Change the active animation of an object by ID.
	 */
	void SetAnimation(int objID, const std::string& newAnim);
	void AttachDinoAnimations(int objID);

	// ID Accessors
	void SetPlayerID(int id);
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
	void SetDinoID(int id) { dinoID = id; }

	int GetPlayerID() const { return spriteID; }
	int GetNPC1ID() const { return otherID; }
	int GetNPC2ID() const { return otherID2; }
	int GetDinoID() const { return dinoID; }

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

	// Defaults Struct
	struct Defaults {
		glm::vec3 pos{ 0,0,0 };
		glm::vec2 size{ 128,128 };
		float rot{ 0.f };
		glm::vec2 colSize{ 64,128 };
		glm::vec2 colOff{ 0,0 };
		glm::vec2 vel{ 0,0 };
		std::string texture;
		std::string tag;
	};

	void SetDefaults(int id, const Defaults& d) { defaults_[id] = d; }
	Defaults GetDefaults(int id) const {
		auto it = defaults_.find(id);
		return (it != defaults_.end()) ? it->second : Defaults{};
	}

	float ScaleXToCurrent(float referenceX) const;
	float ScaleYToCurrent(float referenceY) const;

	float ToRefX(float currentX) const;
	float ToRefY(float currentY) const;

	// Rebuild world/static colliders after level reload or editor reset
	void RebuildColliders();

	void SetSimulationActive(bool active);
	bool IsSimulationActive() const;

	void ResetResizeBaseline();

	void MarkAnimated(int id, bool state);

	void ResolveInitialStaticOverlaps();
	LogicManager& GetLogicManager() { return logicManager; }
	// new helper:
	void AttachLogicForTag(int id, const std::string& tag);

private:
	// Engine/input
	GraphicsEngine& graphicsEngine;
	InputManager inputManager;
	EntityManager entityManager;
	AnimationManager animationManager;
	MovementManager movementManager;
	CollisionManager collisionManager;
	PhysicsManager physicsManager;
	LogicManager logicManager;

	// Systems
	InputCommandHandler inputCommandHandler;
	PlayerController playerController;
	NPCSystem npcSystem;
	DebugVisualizer debugVisualizer;


	// Helper Methods
	void HandlePlayerCollisions(float deltaTime, EntityManager& entityManager);
	void ApplyFinalConstraints(EntityManager& entityManager);

	// World/collision
	void BuildLevelColliders();

	// Step-by-step controller
	physics::StepController physicsStep_;

	// Scene objects
	int spriteID = -1; // default invalid ID
	int dinoID = -1;   // for testing
	int otherID = -1;
	int otherID2 = -1;

	bool simulationActive = false;
	bool useForces_ = false;

	// Debug / Editor
	bool showAuxDebug_ = true;
	LevelEditor mLevelEditor;
	std::unordered_map<int, std::string> mTexturePathByID;

	// Defaults data
	std::unordered_map<int, Defaults> defaults_;

	// Resize tracking
	int lastWidth_ = -1;
	int lastHeight_ = -1;
	bool resetBaseline_ = false;
};
