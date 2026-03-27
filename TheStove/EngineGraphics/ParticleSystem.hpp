/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			ParticleSystem.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Declares the ParticleSystem class responsible for managing particle effects
					such as footsteps, trails, and visual effects. Provides interfaces for
					emitting and updating particles within the scene.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

class EntityManager;
class GameObject;

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

		// Per-preset RNG (seeded at registration time for reproducible behavior)
		std::mt19937 rng{ std::random_device{}() };
	};

	/**
	 * @brief Registers preset.
	 * @param preset Parameter for preset.
	 * @param em Parameter for em.
	 */
	void RegisterPreset(const Preset& preset, EntityManager& em);

	/**
	 * @brief Updates this object.
	 * @param dt Frame delta time in seconds.
	 * @param em Parameter for em.
	 */
	void Update(float dt, EntityManager& em);

	/**
	 * @brief Emits this object.
	 * @param presetName Parameter for preset name.
	 * @param em Parameter for em.
	 * @param pos Parameter for pos.
	 * @param baseZ Parameter for base z.
	 * @param moveDir Parameter for move dir.
	 */
	void Emit(const std::string& presetName,
		EntityManager& em,
		const glm::vec3& pos,
		float baseZ,
		const glm::vec2* moveDir = nullptr);

	/**
	 * @brief Emits footstep.
	 * @param em Parameter for em.
	 * @param pos Parameter for pos.
	 * @param baseZ Parameter for base z.
	 */
	void EmitFootstep(EntityManager& em, const glm::vec3& pos, float baseZ);

	/**
	 * @brief Emits trail.
	 * @param em Parameter for em.
	 * @param pos Parameter for pos.
	 * @param baseZ Parameter for base z.
	 * @param moveDir Parameter for move dir.
	 */
	void EmitTrail(EntityManager& em, const glm::vec3& pos, float baseZ, const glm::vec2& moveDir);

private:
	// Internal struct to track active particle instances and their state
	struct ParticleInstance {
		int id = -1;
		GameObject* obj = nullptr;
		bool active = false;
		bool inFreeList = false;
		const Preset* preset = nullptr;

		glm::vec2 vel{ 0.0f, 0.0f };
		float age = 0.0f;
		float life = 0.0f;

		int frame = 0;
		float frameTimer = 0.0f;

		float baseSize = 1.0f; // pixels
	};

	// Internal struct to manage a pool of particle instances for a given preset
	struct Pool {
		bool initialized = false;
		std::vector<ParticleInstance> p;
		std::vector<size_t> freeList; // indices into p
	};

	// Registered presets and their associated pools
	std::unordered_map<std::string, Preset> presets_;
	std::unordered_map<std::string, Pool> pools_;
	std::mt19937 rng_{ std::random_device{}() };

	// Whether we've registered callbacks with the EntityManager
	bool callbackRegistered_ = false;

	/**
	 * @brief Performs ensure default footstep preset.
	 * @param em Parameter for em.
	 */
	void EnsureDefaultFootstepPreset_(EntityManager& em);

	/**
	 * @brief Initializes pool.
	 * @param preset Parameter for preset.
	 * @param em Parameter for em.
	 */
	void InitPool_(const Preset& preset, EntityManager& em);

	// Initialize the particle system with the EntityManager. Registers callbacks
	/**
	 * @brief Initializes this object.
	 * @param em Parameter for em.
	 */
	void Init(EntityManager& em);

	/**
	 * @brief Performs on entity despawned.
	 * @param id Parameter for id.
	 */
	void OnEntityDespawned(int id);

	/**
	 * @brief Sets seed.
	 * @param seed Parameter for seed.
	 */
	void SetSeed(uint32_t seed);

	/**
	 * @brief Performs rand01.
	 * @return Result produced by this operation.
	 */
	float rand01_();

	/**
	 * @brief Performs rand01.
	 * @param r Parameter for r.
	 * @return Result produced by this operation.
	 */
	float rand01_(std::mt19937& r);

	/**
	 * @brief Performs rand range.
	 * @param a Parameter for a.
	 * @param b Parameter for b.
	 * @return Result produced by this operation.
	 */
	float randRange_(float a, float b);

	/**
	 * @brief Performs rand range.
	 * @param a Parameter for a.
	 * @param b Parameter for b.
	 * @param r Parameter for r.
	 * @return Result produced by this operation.
	 */
	float randRange_(float a, float b, std::mt19937& r);
};
