#pragma once

#include "GraphicsEngine.h"
#include "../Core/InputManager.h"
#include "Animator.h"
#include "Collision.h"
#include <string>
#include <vector>
#include <unordered_map>

class Scene {
public:
	Scene(GraphicsEngine& engine);

	void LoadScene(const std::string& sceneName);
	void Update(float deltaTime, GLFWwindow* window);

    // Scene-specific object creation
    GameObject* SpawnTriangle(const glm::vec3& position, const glm::vec3& scale, float rotation = 0.0f);
    GameObject* SpawnStaticSprite(const std::string& texturePath, const glm::vec3& position,
                                  const glm::vec2& size = glm::vec2(100.0f, 100.0f));

    GameObject* SpawnAnimatedSprite(
        const std::string& texturePath,
        const glm::vec3& position,
        const glm::vec2& size,
        const std::vector<glm::vec4>& frames,
        float frameDuration,
        bool loop);

	// Background management
	void SetSceneBackground(const std::string& texturePath);

	// Object Lookup
    GameObject* GetGameObjectByID(int targetID);

    void SetAnimation(int objID, const std::string& newAnim);

private:
	GraphicsEngine& graphicsEngine;
	InputManager inputManager;

	std::vector<std::unique_ptr<GameObject>> sceneObjects;
	int nextID = 1; // ID counter for GameObjects
    int spriteID = -1; // default invalid ID
	int dinoID = -1; // for testing

    std::unordered_map<int, glm::vec3> spriteScales;
    std::unordered_map<int, glm::vec3> spritePositions;
    std::unordered_map<int, float> spriteRotations;
    std::unordered_map<int, Animator2D> animators; // map GameObject ID to Animator2D

    // Map from GameObject ID to map of animation name to Animator2D
    std::unordered_map<int, std::unordered_map<std::string, Animator2D>> objectAnimations;

    // Current animation name for each object
    std::unordered_map<int, std::string> currentAnimation;

	void LoadTest();

	// World collision system for walls and obstacles.
	collision::World mCollision;

	// Build level colliders (walls, gates, dividers) into mCollision.
	void BuildLevelColliders();

	// Click-to-move state
	bool hasClickTarget = false;		 // True if a target location has been clicked.
	glm::vec2 clickTarget{ 0.0f, 0.0f }; // Current click destination in world coords.

	bool playerSelected = false; // Must select/click player before issuing move.
	float playerSpeed = 260.0f;  // Player movement speed.

	// Stuck detection when pathing into walls
	int stuckFrames = 0;
	static constexpr int kStuckFramesToCancel = 12; // Cancel movement if stuck for ~0.2s at 60fps.
};
