#pragma once

#include <glm/glm.hpp>
#include <vector>

class EntityManager;
class GameObject;

class ParticleSystem {
public:
	struct Particle {
		int id = -1;
		glm::vec2 vel{ 0.0f, 0.0f };
		float life = 0.0f;

		// animation
		int frame = 0;
		float frameTimer = 0.0f;
	};

	void Update(float dt, EntityManager& em);

	void EmitFootstep(EntityManager& em, const glm::vec3& pos, float baseZ);
	void EmitTrail(EntityManager& em, const glm::vec3& pos, float baseZ, const glm::vec2& moveDir);

private:
	std::vector<Particle> particles_;

	float rand01_();
};
