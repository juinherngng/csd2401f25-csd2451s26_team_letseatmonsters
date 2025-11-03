/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Declares the Scene class responsible for managing game objects,
					animations, and scene updates.

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "GraphicsEngine.hpp"
#include "Animator.hpp"

#include "../Core/InputManager.hpp"
#include "../Core/Collision.hpp"
#include "../Core/Physics.hpp"
#include "../Core/Math.hpp"
#include "../Core/SpatialGrid.hpp"
#include "../Core/LevelEditor.hpp"
#include "../Core/Forces.hpp"
#include "../Core/RigidBody2D.hpp"

#include <string>
#include <vector>
#include <unordered_map>

 /**
  * @class Scene
  * @brief Manages the lifecycle of a game scene, including objects, animations, and input.
  */
class Scene {
public:
	// Core Lifecycle

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
	GameObject* SpawnTriangle(const glm::vec3 position, const glm::vec3 scale, float rotation = 0.0f);

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
	void CollectRenderablePointers(std::vector<GameObject*>& out) const;

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
	void SetPlayerID(int id) { spriteID = id; }
	void SetNPC1ID(int id) { otherID = id; }
	void SetNPC2ID(int id) { otherID2 = id; }
	void SetDinoID(int id) { dinoID = id; }

	int GetPlayerID() const { return spriteID; }
	int GetNPC1ID() const { return otherID; }
	int GetNPC2ID() const { return otherID2; }
	int GetDinoID() const { return dinoID; }

	// NPC Velocity
	void SetNPCVelocity(int id, float vx, float vy) { npcVelocities_[id] = { vx, vy }; }
	glm::vec2 GetNPCVelocity(int id) const {
		auto it = npcVelocities_.find(id);
		return (it != npcVelocities_.end()) ? it->second : glm::vec2(0.0f);
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

private:
	// Engine/input
	GraphicsEngine& graphicsEngine;
	InputManager inputManager;

	// World/collision
	void BuildLevelColliders();
	collision::World mCollision;

	// Step-by-step controller
	physics::StepController physicsStep_;

	// Scene objects
	std::vector<std::unique_ptr<GameObject>> sceneObjects;
	int nextID = 0;	   // ID counter for sceneObjects
	int spriteID = -1; // default invalid ID
	int dinoID = -1;   // for testing
	int otherID = -1;
	int otherID2 = -1;

	// Reuse IDs of despawned objects
	std::vector<int> mFreeIDs;
	int AcquireID();

	// Per-object transforms
	std::unordered_map<int, glm::vec3> spriteScales;
	std::unordered_map<int, glm::vec3> spritePositions;
	std::unordered_map<int, float> spriteRotations;
	std::unordered_map<int, Animator2D> animators; // map GameObject ID to Animator2D

	// Map from GameObject ID to map of animation name to Animator2D
	std::unordered_map<int, std::unordered_map<std::string, Animator2D>> objectAnimations;

	// Current animation name for each object
	std::unordered_map<int, std::string> currentAnimation;

	// Scene content
	void LoadTest();

	// Click-to-move
	bool hasClickTarget = false;
	glm::vec2 clickTarget{ 0.0f, 0.0f };

	bool playerSelected = false;
	float playerSpeed = 260.0f;

	// Stuck detection (cancel click move if not progressing)
	int stuckFrames = 0;
	static constexpr int kStuckFramesToCancel = 12;

	// Debug / Editor
	bool showAuxDebug_ = true;
	LevelEditor mLevelEditor;
	std::unordered_map<int, std::string> mTexturePathByID;
	SpatialGrid mSpatialGrid{ 128.0f };

	// NPC / Defaults Data
	std::unordered_map<int, glm::vec2> npcVelocities_;
	std::unordered_map<int, Defaults> defaults_;

	// Physics / Forces
	ForceRegistry mForceRegistry{};
	RigidBody2D* playerRB_ = nullptr;
	Math::Vector2D seekTargetM{ 0.f, 0.f };
	Math::Vector2D playerPosM2D_{ 0.f, 0.f };
	bool useForceForClickMove_ = false;

	bool simulationActive_ = false;
	int lastWidth_ = -1;
	int lastHeight_ = -1;
	bool resetBaseline_ = false;
};
