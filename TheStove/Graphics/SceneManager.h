#pragma once

#include "GraphicsEngine.h"
#include "../Core/InputManager.h"
#include "Collision.h"
#include "../Core/Physics.hpp"

#include <string>
#include <vector>

class Scene {
public:
	Scene(GraphicsEngine& engine);

	// Lifecycle
	void LoadScene(const std::string& sceneName);
	void Update(float deltaTime, GLFWwindow* window);

	// Scene-specific object creation
	GameObject* SpawnTriangle(const glm::vec3& position, const glm::vec3& scale, float rotation = 0.0f);
	GameObject* SpawnSprite(const std::string& texturePath, const glm::vec3& position, const glm::vec2& size = glm::vec2(100.0f, 100.0f));

	// Background management
	void SetSceneBackground(const std::string& texturePath);

	// Object Lookup
	GameObject* GetGameObjectByID(int targetID);

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
	int nextID = 1;	   // ID counter for GameObjects
	int spriteID = -1; // Default invalid ID
	int otherID = -1;
	int otherID2 = -1;

	// Per-object transforms
	std::unordered_map<int, glm::vec3> spriteScales;
	std::unordered_map<int, glm::vec3> spritePositions;
	std::unordered_map<int, float> spriteRotations;

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
