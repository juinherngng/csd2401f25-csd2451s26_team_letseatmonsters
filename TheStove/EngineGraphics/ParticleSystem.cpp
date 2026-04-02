/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			ParticleSystem.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements the ParticleSystem class, handling particle creation, updates,
					Call Init once where ParticleSystem is created to register presets.
					lifetime management, and rendering behavior for in-game visual effects.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <algorithm>
#include <cmath>

#include "EngineCore/Logger.hpp"
#include "EngineGraphics/EntityManager.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/ParticleSystem.hpp"

// Sparkle row (3 frames) - V corrected for OpenGL (bottom-left origin)
static const std::vector<glm::vec4> kSparkleFrames = {
	glm::vec4(0.03880597f, 0.38427299f, 0.07089552f, 0.04451039f),
	glm::vec4(0.20671642f, 0.38427299f, 0.06641791f, 0.04451039f),
	glm::vec4(0.37985075f, 0.38427299f, 0.05074627f, 0.04451039f),
};

// Full texture (single-frame PNG)
static const std::vector<glm::vec4> kFullFrame = {

	/**
	 * @brief Performs vec4.
	 * @param f Parameter for f.
	 * @param f Parameter for f.
	 * @param f Parameter for f.
	 * @param f Parameter for f.
	 * @return Result produced by this operation.
	 */
	glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)
};

/**
 * @brief Performs length safe.
 * @param v Parameter for v.
 * @return Result produced by this operation.
 */
static float lengthSafe(const glm::vec2& v) {
	return glm::length(v);
}

/**
 * @brief Sets seed.
 * @param seed Parameter for seed.
 * @return Result produced by this operation.
 */
void ParticleSystem::SetSeed(uint32_t seed) {
	rng_.seed(seed);
	std::uniform_int_distribution<uint32_t> sd(0, 0xFFFFFFFFu);
	for (auto& kv : presets_) {
		kv.second.rng.seed(sd(rng_));
	}
}

/**
 * @brief Performs rand01.
 * @return Result produced by this operation.
 */
float ParticleSystem::rand01_() {
	std::uniform_real_distribution<float> d(0.0f, 1.0f);
	return d(rng_);
}

/**
 * @brief Performs rand range.
 * @param a Parameter for a.
 * @param b Parameter for b.
 * @return Result produced by this operation.
 */
float ParticleSystem::randRange_(float a, float b) {
	std::uniform_real_distribution<float> d(a, b);
	return d(rng_);
}

/**
 * @brief Performs ensure default footstep preset.
 * @param em Parameter for em.
 * @return Result produced by this operation.
 */
void ParticleSystem::EnsureDefaultFootstepPreset_(EntityManager& em) {
	if (presets_.find("FootstepDust") != presets_.end()) return;

	Preset dust;
	dust.name = "FootstepDust";
	dust.texturePath = "../assets/VFX/run_vfx.png";
	dust.frames = kFullFrame;
	dust.animateFrames = false;
	dust.frameDuration = 0.06f;
	dust.loop = false;

	dust.poolSize = 256;
	dust.lifeRange = glm::vec2(0.50f, 0.60f);
	dust.speedRange = glm::vec2(12.0f, 28.0f);
	dust.gravityY = 0.0f;
	dust.drag = 8.0f;

	// Trail-friendly velocity
	dust.useMoveDirForVelocity = true;
	dust.backwardSpeed = 28.0f;
	dust.sidewaysSpeed = 8.0f;
	dust.randomSpeedJitter = 3.0f;

	dust.spawnJitter = glm::vec2(3.0f, 0.0f);
	dust.zOffset = -0.01f;
	dust.startScaleMul = 1.0f;
	dust.endScaleMul = 0.15f;

	dust.disableColliders = true;

	RegisterPreset(dust, em);
}

/**
 * @brief Initializes pool.
 * @param preset Parameter for preset.
 * @param em Parameter for em.
 * @return Result produced by this operation.
 */
void ParticleSystem::InitPool_(const Preset& preset, EntityManager& em) {
	Pool& pool = pools_[preset.name];
	pool.p.clear();
	pool.freeList.clear();
	pool.p.resize(preset.poolSize);
	pool.freeList.reserve(preset.poolSize);

	// Pre-spawn particle GameObjects once. Keep them "inactive" by scaling to 0.
	const glm::vec3 hiddenPos(0.0f, 0.0f, -1000.0f);
	// Keep track of created IDs so we can clean up on failure
	std::vector<int> createdIDs;

	for (size_t i = 0; i < preset.poolSize; ++i) {
		GameObject* obj = em.SpawnAnimatedSprite(
			preset.texturePath,
			hiddenPos,
			glm::vec2(1.0f, 1.0f),
			preset.frames,
			preset.frameDuration,
			preset.loop
		);

		if (!obj) {
			// Cleanup any previously created objects to avoid leaking
			for (int cid : createdIDs) {
				em.DespawnByID(cid);
			}
			pool.p.clear();
			pool.freeList.clear();
			pool.initialized = false;
			TS_LOG_ERROR("[ParticleSystem] InitPool_ failed to spawn animated sprite for preset '" << preset.name << "'");
			return;
		}

		if (preset.disableColliders) {
			obj->SetColliderSize(Math::Vector2D(0.f, 0.f));
			obj->SetColliderOffset(Math::Vector2D(0.f, 0.f));
		}

		obj->SetScale(glm::vec3(0.0f, 0.0f, 1.0f));
		if (!preset.frames.empty()) {
			obj->SetUVRect(preset.frames.front());
		}

		pool.p[i].id = obj->GetID();
		pool.p[i].obj = obj;
		pool.p[i].active = false;
		pool.p[i].preset = &presets_.at(preset.name);
		pool.p[i].inFreeList = true;

		pool.freeList.push_back(i);
		createdIDs.push_back(pool.p[i].id);
	}

	pool.initialized = true;
}

/**
 * @brief Registers preset.
 * @param preset Parameter for preset.
 * @param em Parameter for em.
 * @return Result produced by this operation.
 */
void ParticleSystem::RegisterPreset(const Preset& preset, EntityManager& em) {
	// Ensure callbacks are registered
	Init(em);

	// Copy preset and seed its RNG from the system RNG for reproducible but varied presets
	presets_[preset.name] = preset;
	// Seed the preset RNG with a random value from the system RNG
	std::uniform_int_distribution<uint32_t> sd(0, 0xFFFFFFFFu);
	presets_[preset.name].rng.seed(sd(rng_));
	InitPool_(presets_.at(preset.name), em);
}

/**
 * @brief Initializes this object.
 * @param em Parameter for em.
 * @return Result produced by this operation.
 */
void ParticleSystem::Init(EntityManager& em) {
	if (callbackRegistered_) return;
	em.RegisterDespawnCallback([this](int id) { this->OnEntityDespawned(id); });
	callbackRegistered_ = true;
}

/**
 * @brief Performs on entity despawned.
 * @param id Parameter for id.
 * @return Result produced by this operation.
 */
void ParticleSystem::OnEntityDespawned(int id) {
	// Walk all pools and clear any cached pointers matching the despawned id
	for (auto& kv : pools_) {
		Pool& pool = kv.second;
		for (auto& pi : pool.p) {
			if (pi.id == id) {
				pi.obj = nullptr;
				// Mark id as invalid so we don't try to resolve it again
				pi.id = -1;
			}
		}
	}
}

/**
 * @brief Performs rand01.
 * @param r Parameter for r.
 * @return Result produced by this operation.
 */
float ParticleSystem::rand01_(std::mt19937& r) {
	std::uniform_real_distribution<float> d(0.0f, 1.0f);
	return d(r);
}

/**
 * @brief Performs rand range.
 * @param a Parameter for a.
 * @param b Parameter for b.
 * @param r Parameter for r.
 * @return Result produced by this operation.
 */
float ParticleSystem::randRange_(float a, float b, std::mt19937& r) {
	std::uniform_real_distribution<float> d(a, b);
	return d(r);
}

/**
 * @brief Emits this object.
 * @param presetName Parameter for preset name.
 * @param em Parameter for em.
 * @param pos Parameter for pos.
 * @param baseZ Parameter for base z.
 * @param moveDir Parameter for move dir.
 * @return Result produced by this operation.
 */
void ParticleSystem::Emit(const std::string& presetName,
	EntityManager& em,
	const glm::vec3& pos,
	float baseZ,
	const glm::vec2* moveDir) {
	EnsureDefaultFootstepPreset_(em);

	auto itPreset = presets_.find(presetName);
	if (itPreset == presets_.end()) {
		return;
	}

	Preset& preset = itPreset->second;

	Pool& pool = pools_[preset.name];
	if (!pool.initialized) {
		InitPool_(preset, em);
		if (!pool.initialized) return;
	}

	size_t idx = 0;

	if (!pool.freeList.empty()) {
		idx = pool.freeList.back();
		pool.freeList.pop_back();
		// Mark as removed from free list
		if (idx < pool.p.size()) {
			pool.p[idx].inFreeList = false;
		}
	}
	else {
		// Recycle the oldest active particle
		float bestScore = -1.0f;
		size_t bestIdx = 0;

		for (size_t i = 0; i < pool.p.size(); ++i) {
			ParticleInstance& cand = pool.p[i];
			if (!cand.active || !cand.preset) {
				continue;
			}

			float score = (cand.life > 0.0001f) ? (cand.age / cand.life) : cand.age;
			if (score > bestScore) {
				bestScore = score;
				bestIdx = i;
			}
		}

		idx = bestIdx;
	}

	ParticleInstance& p = pool.p[idx];
	GameObject* obj = p.obj ? p.obj : em.GetByID(p.id);
	if (!obj) {
		// If the cached pointer is invalid, mark pool for re-init and recycle
		pool.initialized = false;
		if (idx < pool.p.size() && !pool.p[idx].inFreeList) {
			pool.freeList.push_back(idx);
			pool.p[idx].inFreeList = true;
		}
		return;
	}

	p.active = true;
	p.preset = &preset;
	p.age = 0.0f;
	// Ensure this instance is not marked free
	p.inFreeList = false;

	// Use per-preset RNG if available to make preset behavior reproducible
	std::mt19937& prng = preset.rng;
	p.life = randRange_(preset.lifeRange.x, preset.lifeRange.y, prng);
	p.frame = 0;
	p.frameTimer = 0.0f;

	float size = randRange_(preset.sizeRange.x, preset.sizeRange.y, prng);
	p.baseSize = size;

	glm::vec3 spawnPos = pos;
	spawnPos.x += randRange_(-preset.spawnJitter.x, preset.spawnJitter.x, prng);
	spawnPos.y += randRange_(-preset.spawnJitter.y, preset.spawnJitter.y, prng);
	spawnPos.z = baseZ + preset.zOffset;

	// Update cached pointer in case we resolved via GetByID
	p.obj = obj;

	obj->SetPosition(spawnPos);
	obj->SetScale(glm::vec3(size * preset.startScaleMul, size * preset.startScaleMul, 1.0f));
	if (!preset.frames.empty()) {
		// Pick a random frame (sparkle 1/2/3) and keep it (no animation needed)
		std::uniform_int_distribution<int> fd(0, static_cast<int>(preset.frames.size()) - 1);
		p.frame = fd(prng);
		obj->SetUVRect(preset.frames[static_cast<size_t>(p.frame)]);
	}

	// Velocity
	glm::vec2 v(0.0f, 0.0f);

	if (preset.useMoveDirForVelocity && moveDir) {
		glm::vec2 dir = *moveDir;
		float len = lengthSafe(dir);
		if (len > 0.0001f) {
			dir /= len;
		}

		glm::vec2 perp(-dir.y, dir.x);
		v += (-dir) * preset.backwardSpeed;
		v += perp * randRange_(-preset.sidewaysSpeed, preset.sidewaysSpeed, prng);
		v.x += randRange_(-preset.randomSpeedJitter, preset.randomSpeedJitter, prng);
		v.y += randRange_(-preset.randomSpeedJitter, preset.randomSpeedJitter, prng);
	}
	else {
		float sp = randRange_(preset.speedRange.x, preset.speedRange.y, prng);
		v = glm::vec2(randRange_(-1.0f, 1.0f, prng), -1.0f);
		float l = lengthSafe(v);
		if (l > 0.0001f) {
			v /= l;
		}

		v *= sp;
	}

	p.vel = v;
}

/**
 * @brief Updates this object.
 * @param dt Frame delta time in seconds.
 * @param em Parameter for em.
 * @return Result produced by this operation.
 */
void ParticleSystem::Update(float dt, EntityManager& em) {
	if (dt <= 0.0f) {
		return;
	}

	// Clamp very large dt (e.g. when debugging or after a hitch) to avoid
	// particles moving or animating too far in a single frame.
	const float kMaxDt = 1.0f / 30.0f;
	if (dt > kMaxDt) {
		dt = kMaxDt;
	}

	for (auto& kv : pools_) {
		Pool& pool = kv.second;
		if (!pool.initialized) continue;

		for (size_t i = 0; i < pool.p.size(); ++i) {
			ParticleInstance& p = pool.p[i];
			if (!p.active || !p.preset) {
				continue;
			}

			const Preset& preset = *p.preset;

			GameObject* obj = p.obj ? p.obj : em.GetByID(p.id);
			if (!obj) {
				p.active = false;
				if (!p.inFreeList) {
					pool.freeList.push_back(i);
					p.inFreeList = true;
				}
				continue;
			}
			// Cache resolved pointer
			p.obj = obj;

			p.age += dt;
			if (p.age >= p.life) {
				// Return to pool
				p.active = false;
				obj->SetScale(glm::vec3(0.0f, 0.0f, 1.0f));
				obj->SetPosition(glm::vec3(0.0f, 0.0f, -1000.0f));
				if (!p.inFreeList) {
					pool.freeList.push_back(i);
					p.inFreeList = true;
				}
				continue;
			}

			// Animate (only if enabled)
			if (preset.animateFrames && preset.frames.size() > 1) {
				p.frameTimer += dt;
				if (preset.frameDuration > 1e-6f) {
					while (p.frameTimer >= preset.frameDuration) {
						p.frameTimer -= preset.frameDuration;
						p.frame++;

						if (preset.loop) {
							if (p.frame >= (int)preset.frames.size()) {
								p.frame = 0;
							}
						}
						else {
							if (p.frame >= (int)preset.frames.size()) {
								p.frame = (int)preset.frames.size() - 1;
							}
						}

						obj->SetUVRect(preset.frames[(size_t)p.frame]);
						if (!preset.loop && p.frame == (int)preset.frames.size() - 1) {
							break;
						}
					}
				}
			}

			// Drag + gravity
			float dragFactor = 1.0f / (1.0f + preset.drag * dt);
			p.vel *= dragFactor;
			p.vel.y += preset.gravityY * dt;

			glm::vec3 pos = obj->GetPositionGLM();
			pos.x += p.vel.x * dt;
			pos.y += p.vel.y * dt;
			obj->SetPosition(pos);

			// Fade-out via scale curve
			float t = (p.life > 0.0001f) ? (p.age / p.life) : 1.0f;
			t = std::clamp(t, 0.0f, 1.0f);
			float sMul = preset.startScaleMul + (preset.endScaleMul - preset.startScaleMul) * t;

			float s = p.baseSize * sMul;
			if (s < 0.0f) {
				s = 0.0f;
			}

			obj->SetScale(glm::vec3(s, s, 1.0f));
		}
	}
}

/**
 * @brief Emits footstep.
 * @param em Parameter for em.
 * @param pos Parameter for pos.
 * @param baseZ Parameter for base z.
 * @return Result produced by this operation.
 */
void ParticleSystem::EmitFootstep(EntityManager& em, const glm::vec3& pos, float baseZ) {
	Emit("FootstepDust", em, pos, baseZ, nullptr);
}

/**
 * @brief Emits trail.
 * @param em Parameter for em.
 * @param pos Parameter for pos.
 * @param baseZ Parameter for base z.
 * @param moveDir Parameter for move dir.
 * @return Result produced by this operation.
 */
void ParticleSystem::EmitTrail(EntityManager& em, const glm::vec3& pos, float baseZ, const glm::vec2& moveDir) {
	Emit("FootstepDust", em, pos, baseZ, &moveDir);
}
