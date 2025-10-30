#pragma once

#include <vector>
#include <algorithm>
#include "Math.hpp"

class RigidBody2D;

// ---------- Base interface ----------
struct IForceGenerator {
	virtual ~IForceGenerator() = default;
	virtual void UpdateForce(RigidBody2D& body, float dt) = 0;
};

// ---------- Registry ----------
class ForceRegistry {
public:
	void Add(RigidBody2D* body, IForceGenerator* gen);
	void Remove(RigidBody2D* body, IForceGenerator* gen);
	void Clear();
	void UpdateForces(float dt);

private:
	struct Entry { RigidBody2D* body; IForceGenerator* gen; };
	std::vector<Entry> entries;
};

// ---------- Generators ----------
struct GravityForce : IForceGenerator {
	Math::Vector2D g; // e.g., (0,-9.8f)
	explicit GravityForce(Math::Vector2D gravity) : g(gravity) {}
	void UpdateForce(RigidBody2D& body, float dt) override;
};

struct DragForce : IForceGenerator {
	float k1 = 0.0f; // linear term
	float k2 = 0.0f; // quadratic term
	DragForce(float k1_, float k2_) : k1(k1_), k2(k2_) {}
	void UpdateForce(RigidBody2D& body, float dt) override;
};

struct ConstantForce : IForceGenerator {
	Math::Vector2D f;
	explicit ConstantForce(Math::Vector2D F) : f(F) {}
	void UpdateForce(RigidBody2D& body, float dt) override;
};

// Gentle steering toward a target (for point & click)
struct SeekForce : IForceGenerator {
	Math::Vector2D* target = nullptr; // external pointer you control
	float maxAccel = 600.0f;          // tune to taste
	explicit SeekForce(Math::Vector2D* tgt, float maxA = 600.f) : target(tgt), maxAccel(maxA) {}
	void UpdateForce(RigidBody2D& body, float dt) override;
};
