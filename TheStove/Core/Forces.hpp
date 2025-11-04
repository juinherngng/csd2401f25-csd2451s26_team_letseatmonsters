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

/**
 * @brief Interface for all force generators.
 */
struct IForceGenerator {
	virtual ~IForceGenerator();
	virtual void UpdateForce(RigidBody2D& body, float dt) = 0;
};

/**
 * @brief Associates bodies with force generators and applies them each frame.
 */
class ForceRegistry {
public:
	struct Entry {
		RigidBody2D* body = nullptr;
		IForceGenerator* gen = nullptr;
	};

	void Add(RigidBody2D* body, IForceGenerator* gen);
	void Remove(RigidBody2D* body, IForceGenerator* gen);
	void Clear();
	void UpdateForces(float dt);

private:
	std::vector<Entry> entries;
};

/**
 * @brief Applies constant gravitational acceleration (scaled by mass).
 */
struct GravityForce : IForceGenerator {
	Math::Vector2D g{};

	explicit GravityForce(Math::Vector2D gravity);
	void UpdateForce(RigidBody2D& body, float dt) override;
};

/**
 * @brief Applies quadratic/linear drag opposite to velocity.
 */
struct DragForce : IForceGenerator {
	float k1 = 0.0f; // linear term
	float k2 = 0.0f; // quadratic term

	DragForce(float linearK, float quadraticK);
	void UpdateForce(RigidBody2D& body, float dt) override;
};

/**
 * @brief Applies a constant world-space force vector every update.
 */
struct ConstantForce : IForceGenerator {
	Math::Vector2D f{ 0.0f, 0.0f };

	explicit ConstantForce(Math::Vector2D force);
	void UpdateForce(RigidBody2D& body, float dt) override;
};

/**
 * @brief Gentle steering toward a target with arrive behavior.
 *        Pushes toward target up to maxAccel and hard-stops inside arriveRadius.
 */
struct SeekForce : IForceGenerator {
	Math::Vector2D* target = nullptr;                 // Destination (mutable externally)
	const Math::Vector2D* currentPos2DPtr = nullptr;  // Optional external position to use instead of body position
	float maxAccel = 600.0f;                          // Acceleration magnitude
	float arriveRadius = 6.0f;                        // Deadzone to stop and zero velocity

	SeekForce(Math::Vector2D* targetPtr, float maxAccelIn = 600.0f, float arrive = 6.0f);
	SeekForce(Math::Vector2D* targetPtr,
		const Math::Vector2D* currentPosPtr,
		float maxAccelIn = 600.0f,
		float arrive = 6.0f);

	void UpdateForce(RigidBody2D& body, float dt) override;
};
