#pragma once

#include "GraphicsEngine.h"
#include "../Core/InputManager.h"
#include "Animator.h"
#include "Collision.h"
#include "../Core/Physics.hpp"

#include <string>
#include <vector>
#include <unordered_map>

class Scene {
public:
	Scene(GraphicsEngine& engine);

	// Lifecycle
	void LoadScene(const std::string& sceneName);
	void Update(float deltaTime, GLFWwindow* window);

    // Scene-specific object creation
    GameObject* SpawnTriangle(const glm::vec3 position, const glm::vec3 scale, float rotation = 0.0f);
    GameObject* SpawnStaticSprite(const std::string& texturePath, const glm::vec3 position, const glm::vec2 size = glm::vec2(100.0f, 100.0f));
    GameObject* SpawnAnimatedSprite(const std::string& texturePath, const glm::vec3 position,
									const glm::vec2 size, const std::vector<glm::vec4> frames,
									float frameDuration, bool loop);

	// Background management
	void SetSceneBackground(const std::string& texturePath);

	// Object Lookup
	GameObject* GetGameObjectByID(int targetID);

	// Optional: despawn API
	void DespawnByID(int targetID);

    void SetAnimation(int objID, const std::string& newAnim);

	// Collect raw pointers for rendering
	void CollectRenderablePointers(std::vector<GameObject*>& out) const;

private:
	// Engine/input
	GraphicsEngine& graphicsEngine;
	InputManager inputManager;

	//Scene-owned objects
	std::vector<std::unique_ptr<GameObject>> sceneObjects;
	int nextID = 1; // ID counter for SceneObjects

    int spriteID = -1; // default invalid ID
	int dinoID = -1; // for testing

    std::unordered_map<int, glm::vec3> spriteScales;
    std::unordered_map<int, glm::vec3> spritePositions;
    std::unordered_map<int, float> spriteRotations;
    std::unordered_map<int, Animator2D> animators; // map GameObject ID to Animator2D

	// Map of object ID to their animations (name to Animator2D)
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
};
