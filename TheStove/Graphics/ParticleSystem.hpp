/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			ParticleSystem.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Declares the ParticleSystem class responsible for managing particle effects
					such as footsteps, trails, and visual effects. Provides interfaces for
					emitting and updating particles within the scene.

		All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class EntityManager;

class ParticleSystem {
public:
	// Data-driven description of an effect
	struct Preset {
		std::string name;

		// Visual
		std::string texturePath;
		std::vector<glm::vec4> frames; // UV rects
		float frameDuration = 0.06f;
		bool loop = false;
		bool animateFrames = false;

		// Pooling
		size_t poolSize = 256;

		// Spawn / motion
		glm::vec2 sizeRange{ 24.0f, 34.0f };   // pixels (min, max)
		glm::vec2 lifeRange{ 0.20f, 0.35f };   // seconds (min, max)
		glm::vec2 speedRange{ 20.0f, 35.0f };  // pixels/sec (min, max)
		float gravityY = 0.0f;                 // pixels/sec^2
		float drag = 6.0f;                     // 0 = no drag

		// Direction shaping
		bool useMoveDirForVelocity = false;  // trail behavior
		float backwardSpeed = 25.0f;         // used if useMoveDirForVelocity
		float sidewaysSpeed = 8.0f;          // used if useMoveDirForVelocity
		float randomSpeedJitter = 8.0f;

		// Placement jitter
		glm::vec2 spawnJitter{ 3.0f, 0.0f };  // pixels (x,y)
		float zOffset = -0.01f;

		// Fade-out (engine-agnostic): scale from start->end over life
		float startScaleMul = 1.0f;
		float endScaleMul = 0.0f;

		// Physics/collision
		bool disableColliders = true;
	};

	void RegisterPreset(const Preset& preset, EntityManager& em);
	void Update(float dt, EntityManager& em);

	void Emit(const std::string& presetName,
		EntityManager& em,
		const glm::vec3& pos,
		float baseZ,
		const glm::vec2* moveDir = nullptr);

	// Backwards-compatible helpers
	void EmitFootstep(EntityManager& em, const glm::vec3& pos, float baseZ);
	void EmitTrail(EntityManager& em, const glm::vec3& pos, float baseZ, const glm::vec2& moveDir);

private:
	struct ParticleInstance {
		int id = -1;
		bool active = false;
		const Preset* preset = nullptr;

		glm::vec2 vel{ 0.0f, 0.0f };
		float age = 0.0f;
		float life = 0.0f;

		int frame = 0;
		float frameTimer = 0.0f;

		float baseSize = 1.0f; // pixels
	};

	struct Pool {
		bool initialized = false;
		std::vector<ParticleInstance> p;
		std::vector<size_t> freeList; // indices into p
	};

	std::unordered_map<std::string, Preset> presets_;
	std::unordered_map<std::string, Pool> pools_;

	void EnsureDefaultFootstepPreset_(EntityManager& em);
	void InitPool_(const Preset& preset, EntityManager& em);

	float rand01_();
	float randRange_(float a, float b);
};
