/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Forces.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <vector>
#include <algorithm>

#include "Math.hpp"

class RigidBody2D;

// Base interface
struct IForceGenerator {
	virtual ~IForceGenerator();
	virtual void UpdateForce(RigidBody2D& body, float dt) = 0;
};

// Registry
class ForceRegistry {
public:
	void Add(RigidBody2D* body, IForceGenerator* gen);
	void Remove(RigidBody2D* body, IForceGenerator* gen);
	void Clear();
	void UpdateForces(float dt);

private:
	struct Entry {
		RigidBody2D* body = nullptr;
		IForceGenerator* gen = nullptr;
	};

	std::vector<Entry> entries;
};

// Generators
struct GravityForce : IForceGenerator {
	Math::Vector2D g;

	explicit GravityForce(Math::Vector2D gravity);
	void UpdateForce(RigidBody2D& body, float dt) override;
};

struct DragForce : IForceGenerator {
	float k1 = 0.0f; // linear term
	float k2 = 0.0f; // quadratic term

	DragForce(float linearK, float quadraticK);
	void UpdateForce(RigidBody2D& body, float dt) override;
};

struct ConstantForce : IForceGenerator {
	Math::Vector2D f{ 0.0f, 0.0f };

	explicit ConstantForce(Math::Vector2D force);
	void UpdateForce(RigidBody2D& body, float dt) override;
};

// Gentle steering toward a target (for point & click)
// Uses an arrival radius to stop pushing & zero velocity near the goal.
struct SeekForce : IForceGenerator {
	Math::Vector2D* target = nullptr;  // destination
	const Math::Vector2D* currentPos2DPtr = nullptr;
	float maxAccel = 600.0f; // acceleration magnitude (tunable)
	float arriveRadius = 6.0f; // deadzone to stop pushing

	SeekForce(Math::Vector2D* tgt, float maxA = 600.0f, float arrive = 6.0f);
	SeekForce(Math::Vector2D* tgt,
		const Math::Vector2D* cur,
		float maxA = 600.0f,
		float arrive = 6.0f);

	void UpdateForce(RigidBody2D& body, float dt) override;
};
