#include "ParticleSystem.hpp"
#include "EntityManager.hpp"
#include "GameObject.hpp"
#include <cstdlib>
#include <algorithm>

// VFX sheet size (from your uploaded image)
static constexpr float TEX_W = 1340.0f;
static constexpr float TEX_H = 1348.0f;

// Bottom puff row (4 frames), UV rects are (u, v, w, h) with v measured from TOP
static const std::vector<glm::vec4> kPuffFrames = {
    // left to right
    glm::vec4(0.05597015f, 0.71810089f, 0.05223881f, 0.05341246f),
    glm::vec4(0.23208955f, 0.71587537f, 0.05223881f, 0.05341246f),
    glm::vec4(0.38805970f, 0.71068249f, 0.06268657f, 0.05934718f),
    glm::vec4(0.56268657f, 0.71216617f, 0.04925373f, 0.05118694f)
};

float ParticleSystem::rand01_() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

void ParticleSystem::EmitFootstep(EntityManager& em, const glm::vec3& pos, float baseZ) {
    const char* kTex = "../assets/VFX SpriteSheet.png";

    glm::vec3 spawnPos = pos;

    // small random offset around feet
    spawnPos.x += (rand01_() * 2.0f - 1.0f) * 3.0f;

    // behind player
    spawnPos.z = baseZ - 0.01f;

    const float s = 24.0f + rand01_() * 10.0f;

    // Spawn as animated sprite (we will manually step UV frames)
    GameObject* p = em.SpawnAnimatedSprite(
        kTex,
        spawnPos,
        glm::vec2(s, s),
        kPuffFrames,
        0.06f,
        false
    );
    if (!p) return;

    // Particles must not collide
    p->SetColliderSize(Math::Vector2D(0.f, 0.f));
    p->SetColliderOffset(Math::Vector2D(0.f, 0.f));

    // start at frame 0
    p->SetUVRect(kPuffFrames[0]);

    Particle part;
    part.id = p->GetID();
    part.vel = glm::vec2((rand01_() * 2.0f - 1.0f) * 10.0f, -20.0f - rand01_() * 15.0f);
    part.frame = 0;
    part.frameTimer = 0.0f;

    // life = number of frames * frameDuration (+ small extra)
    part.life = static_cast<float>(kPuffFrames.size()) * 0.06f + 0.02f;

    particles_.push_back(part);
}

void ParticleSystem::Update(float dt, EntityManager& em) {
    const float frameDuration = 0.06f;

    for (auto& p : particles_) {
        p.life -= dt;

        GameObject* obj = em.GetByID(p.id);
        if (!obj) {
            p.life = -1.0f;
            continue;
        }

        // move
        glm::vec3 pos = obj->GetPositionGLM();
        pos.x += p.vel.x * dt;
        pos.y += p.vel.y * dt;
        obj->SetPosition(pos);

        // animate UV frames (non-loop)
        p.frameTimer += dt;
        while (p.frameTimer >= frameDuration) {
            p.frameTimer -= frameDuration;
            p.frame++;

            if (p.frame >= static_cast<int>(kPuffFrames.size())) {
                // finished animation
                p.life = -1.0f;
                break;
            }
            else {
                obj->SetUVRect(kPuffFrames[p.frame]);
            }
        }

        // optional shrink
        glm::vec3 sc = obj->GetScaleGLM();
        float shrink = 1.0f - (dt * 2.0f);
        if (shrink < 0.0f) shrink = 0.0f;
        obj->SetScale(glm::vec3(sc.x * shrink, sc.y * shrink, sc.z));
    }

    // despawn dead
    for (auto& p : particles_) {
        if (p.life <= 0.0f) {
            em.DespawnByID(p.id);
            p.id = -1;
        }
    }

    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
            [](const Particle& p) { return p.id < 0; }),
        particles_.end()
    );
}

void ParticleSystem::EmitTrail(EntityManager& em, const glm::vec3& pos, float baseZ, const glm::vec2& moveDir) {
    EmitFootstep(em, pos, baseZ);
}
