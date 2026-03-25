/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         SceneDebug.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu (50%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(50%)

 DESCRIPTION:       Implements debug-only Scene helpers:
						- Stress test object generator
						- Animation hotkey controls for dinos

	 All content  2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "SceneManager.hpp"

#include <imgui.h>
#include <iostream>
#include <random>

/**
 * @brief Generates stress test.
 * @param objectCount Parameter for object count.
 * @return Result produced by this operation.
 */
void Scene::GenerateStressTest(int objectCount) {
	std::cout << "[Scene] Generating stress test with " << objectCount << " objects...\n";

	std::random_device rd;
	std::mt19937 gen(rd());

	// Random position ranges
	std::uniform_real_distribution<float> posX(50.0f, 1150.0f);
	std::uniform_real_distribution<float> posY(50.0f, 750.0f);

	// Random velocity ranges
	std::uniform_real_distribution<float> velX(-100.0f, 100.0f);
	std::uniform_real_distribution<float> velY(-100.0f, 100.0f);

	// Random size range
	std::uniform_real_distribution<float> sizeRand(24.0f, 64.0f);

	// Textures to render
	std::vector<std::string> texturePaths = {
		"../assets/goat_sprite_front.png",
		"../assets/mc_sprite_front.png"
	};

	std::uniform_int_distribution<size_t> texIndex(0, texturePaths.size() - 1);

	for (int i = 0; i < objectCount; ++i) {
		glm::vec3 randomPos(posX(gen), posY(gen), 0.0f);
		float size = sizeRand(gen);
		glm::vec2 randomSize(size, size);

		std::string texPath = texturePaths[texIndex(gen)];

		GameObject* obj = entityManager.SpawnStaticSprite(texPath, randomPos, randomSize);
		if (obj) {
			Math::Vector2D randomVel(velX(gen), velY(gen));
			obj->SetVelocity(randomVel);
		}
	}

	std::cout << "[Scene] Stress test loaded\n";
}

/**
 * @brief Updates animation controls.
 * @return Result produced by this operation.
 */
void Scene::UpdateAnimationControls() {
	// Do nothing when simulation is paused
	if (!simulationActive) {
		return;
	}

	// If UI is capturing keyboard, do not change gameplay animation state
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard) {
		return;
	}

	// Edge detection for single key presses
	const bool k1 = inputManager.IsKeyJustPressed(GLFW_KEY_1);
	const bool k2 = inputManager.IsKeyJustPressed(GLFW_KEY_2);
	const bool k3 = inputManager.IsKeyJustPressed(GLFW_KEY_3);

	if (!k1 && !k2 && !k3) {
		return;
	}

	const std::string anim = k1 ? "IDLE" : (k2 ? "WALK" : "ATTACK");

	// Make these known dino IDs use the chosen animation (skip missing objects)
	const int dinoIDs[] = { 1, 2, 3 };
	for (int id : dinoIDs) {
		GameObject* obj = GetGameObjectByID(id);
		if (!obj) {
			continue;
		}

		// Ensure animator is attached
		if (!animationManager.HasAnimator(id)) {
			animationManager.AttachDinoAnimations(id);
		}

		animationManager.SetAnimation(id, anim);
		std::cout << "[Scene] Playing animation " << anim
			<< " for dino id=" << id << "\n";
	}
}
