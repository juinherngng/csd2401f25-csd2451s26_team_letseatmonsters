/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Forces.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Declares a simple force system for 2D rigid bodies:
					- IForceGenerator: interface for all force sources
					- ForceRegistry:   maps bodies <-> force generators and updates them
					- GravityForce:    constant acceleration (F = m * g)
					- DragForce:       linear + quadratic velocity drag
					- ConstantForce:   applies a fixed world-space force each step
					- SeekForce:       seek/arrive steering toward a target with deadzone

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <algorithm>
#include <vector>

#include "Math.hpp"
#include "RigidBody2D.hpp"

 // Interface: any force source implements this.
struct IForceGenerator {
	virtual ~IForceGenerator();

	// Apply force to a body for this timestep.
	virtual void UpdateForce(RigidBody2D& body, float dt) = 0;
};

// Registry: holds body <-> generator pairs and updates all per frame.
class ForceRegistry {
public:
	// Types
	struct Entry {
		RigidBody2D* body = nullptr;
		IForceGenerator* gen = nullptr;
	};

	// Register a body with a generator.
	void Add(RigidBody2D* body, IForceGenerator* gen);

	// Unregister a body from a generator.
	void Remove(RigidBody2D* body, IForceGenerator* gen);

	// Remove all entries.
	void Clear();

	// Update all registered generators (applies forces).
	void UpdateForces(float dt);

private:
	// Data Members
	std::vector<Entry> entries;
};

// Concrete forces
struct GravityForce : IForceGenerator {
	// Data
	Math::Vector2D g{};

	// Ctors / Interface
	explicit GravityForce(Math::Vector2D gravity);
	void UpdateForce(RigidBody2D& body, float dt) override;
};

// Drag force with linear and quadratic terms.
struct DragForce : IForceGenerator {
	// Data
	float k1 = 0.0f; // linear term
	float k2 = 0.0f; // quadratic term

	// Ctors / Interface
	DragForce(float linearK, float quadraticK);
	void UpdateForce(RigidBody2D& body, float dt) override;
};

// Applies a fixed world-space force every update.
struct ConstantForce : IForceGenerator {
	// Data
	Math::Vector2D f{ 0.0f, 0.0f };

	// Ctors / Interface
	explicit ConstantForce(Math::Vector2D force);
	void UpdateForce(RigidBody2D& body, float dt) override;
};

// Seek / Arrive steering toward a target with max acceleration and arrive radius.
struct SeekForce : IForceGenerator {
	// Data
	Math::Vector2D* target = nullptr;                 // Destination (mutable externally)
	const Math::Vector2D* currentPos2DPtr = nullptr;  // Optional external position to use instead of body position
	float maxAccel = 600.0f;                          // Acceleration magnitude
	float arriveRadius = 6.0f;                        // Deadzone to stop and zero velocity

	// Ctors / Interface
	SeekForce(Math::Vector2D* targetPtr, float maxAccelIn = 600.0f, float arrive = 6.0f);

	SeekForce(Math::Vector2D* targetPtr,
		const Math::Vector2D* currentPosPtr,
		float maxAccelIn = 600.0f,
		float arrive = 6.0f);

	void UpdateForce(RigidBody2D& body, float dt) override;
};
