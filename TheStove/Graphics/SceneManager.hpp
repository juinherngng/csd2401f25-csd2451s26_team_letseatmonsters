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

#include <string>
#include <vector>
#include <unordered_map>

 /**
  * @class Scene
  * @brief Manages the lifecycle of a game scene, including objects, animations, and input.
  */
class Scene {
public:
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
	 * @brief Set the background texture for the scene.
	 */
	void SetSceneBackground(const std::string& texturePath);

	/**
	 * @brief Retrieve a game object by its ID.
	 */
	GameObject* GetGameObjectByID(int targetID);

	/**
	 * @brief Remove a game object by its ID, including its animations.
	 */
	void DespawnByID(int targetID);

	/**
	 * @brief Change the active animation of an object by ID.
	 */
	void SetAnimation(int objID, const std::string& newAnim);

	/**
	 * @brief Collect raw pointers to all renderable game objects.
	 */
	void CollectRenderablePointers(std::vector<GameObject*>& out) const;

	std::vector<GameObject*> GetAllObjectsRaw();
	const std::string& GetObjectTexturePath(int id) const;
	void SetObjectTexturePath(int id, const std::string& path);
	void DrawUI();

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
	int nextID = 1;	   // ID counter for sceneObjects
	int spriteID = -1; // default invalid ID
	int dinoID = -1;   // for testing
	int otherID = -1;
	int otherID2 = -1;

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

	bool showAuxDebug_ = true;

	SpatialGrid mSpatialGrid{ 128.0f };

	LevelEditor mLevelEditor; // PC editor
	std::unordered_map<int, std::string> mTexturePathByID;
};
