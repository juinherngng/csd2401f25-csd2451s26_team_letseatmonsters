#include "EntityManager.hpp"
#include "GameObject.hpp"
#include "ParticleSystem.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

// Sparkle row (3 frames) - V corrected for OpenGL (bottom-left origin)
static const std::vector<glm::vec4> kSparkleFrames = {
	glm::vec4(0.03880597f, 0.38427299f, 0.07089552f, 0.04451039f),
	glm::vec4(0.20671642f, 0.38427299f, 0.06641791f, 0.04451039f),
	glm::vec4(0.37985075f, 0.38427299f, 0.05074627f, 0.04451039f),
};

// Full texture (single-frame PNG)
static const std::vector<glm::vec4> kFullFrame = {
	glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)
};

static float lengthSafe(const glm::vec2& v) {
	return std::sqrt(v.x * v.x + v.y * v.y);
}

float ParticleSystem::rand01_() {
	return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

float ParticleSystem::randRange_(float a, float b) {
	return a + (b - a) * rand01_();
}

void ParticleSystem::EnsureDefaultFootstepPreset_(EntityManager& em) {
	if (presets_.find("FootstepDust") != presets_.end()) return;

	Preset dust;
	dust.name = "FootstepDust";
	dust.texturePath = "../assets/run_vfx.png";
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

void ParticleSystem::InitPool_(const Preset& preset, EntityManager& em) {
	Pool& pool = pools_[preset.name];
	pool.p.clear();
	pool.freeList.clear();
	pool.p.resize(preset.poolSize);
	pool.freeList.reserve(preset.poolSize);

	// Pre-spawn particle GameObjects once. Keep them "inactive" by scaling to 0.
	const glm::vec3 hiddenPos(0.0f, 0.0f, -1000.0f);

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
			pool.initialized = false;
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
		pool.p[i].active = false;
		pool.p[i].preset = &presets_.at(preset.name);

		pool.freeList.push_back(i);
	}

	pool.initialized = true;
}

void ParticleSystem::RegisterPreset(const Preset& preset, EntityManager& em) {
	presets_[preset.name] = preset;
	InitPool_(presets_.at(preset.name), em);
}

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
	GameObject* obj = em.GetByID(p.id);
	if (!obj) {
		pool.initialized = false;
		pool.freeList.push_back(idx);
		return;
	}

	p.active = true;
	p.preset = &preset;
	p.age = 0.0f;
	p.life = randRange_(preset.lifeRange.x, preset.lifeRange.y);
	p.frame = 0;
	p.frameTimer = 0.0f;

	float size = randRange_(preset.sizeRange.x, preset.sizeRange.y);
	p.baseSize = size;

	glm::vec3 spawnPos = pos;
	spawnPos.x += randRange_(-preset.spawnJitter.x, preset.spawnJitter.x);
	spawnPos.y += randRange_(-preset.spawnJitter.y, preset.spawnJitter.y);
	spawnPos.z = baseZ + preset.zOffset;

	obj->SetPosition(spawnPos);
	obj->SetScale(glm::vec3(size * preset.startScaleMul, size * preset.startScaleMul, 1.0f));
	if (!preset.frames.empty()) {
		// Pick a random frame (sparkle 1/2/3) and keep it (no animation needed)
		p.frame = static_cast<int>(rand01_() * preset.frames.size());
		if (p.frame >= static_cast<int>(preset.frames.size())) {
			p.frame = static_cast<int>(preset.frames.size()) - 1;
		}
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
		v += perp * randRange_(-preset.sidewaysSpeed, preset.sidewaysSpeed);
		v.x += randRange_(-preset.randomSpeedJitter, preset.randomSpeedJitter);
		v.y += randRange_(-preset.randomSpeedJitter, preset.randomSpeedJitter);
	}
	else {
		float sp = randRange_(preset.speedRange.x, preset.speedRange.y);
		v = glm::vec2(randRange_(-1.0f, 1.0f), -1.0f);
		float l = lengthSafe(v);
		if (l > 0.0001f) {
			v /= l;
		}

		v *= sp;
	}

	p.vel = v;
}

void ParticleSystem::Update(float dt, EntityManager& em) {
	if (dt <= 0.0f) {
		return;
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

			GameObject* obj = em.GetByID(p.id);
			if (!obj) {
				p.active = false;
				pool.freeList.push_back(i);
				continue;
			}

			p.age += dt;
			if (p.age >= p.life) {
				// Return to pool
				p.active = false;
				obj->SetScale(glm::vec3(0.0f, 0.0f, 1.0f));
				obj->SetPosition(glm::vec3(0.0f, 0.0f, -1000.0f));
				pool.freeList.push_back(i);
				continue;
			}

			// Animate (only if enabled)
			if (preset.animateFrames && preset.frames.size() > 1) {
				p.frameTimer += dt;
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

void ParticleSystem::EmitFootstep(EntityManager& em, const glm::vec3& pos, float baseZ) {
	Emit("FootstepDust", em, pos, baseZ, nullptr);
}

void ParticleSystem::EmitTrail(EntityManager& em, const glm::vec3& pos, float baseZ, const glm::vec2& moveDir) {
	Emit("FootstepDust", em, pos, baseZ, &moveDir);
}
