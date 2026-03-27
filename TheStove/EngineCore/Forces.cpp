/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Forces.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements the force system (registry + generators). Each generator applies
					its force to a body during UpdateForce; the registry iterates all pairs.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>

#include "EngineCore/Forces.hpp"
#include "EngineCore/RigidBody2D.hpp"

/**
 * @brief Performs ~iforce generator.
 * @return Result produced by this operation.
 */
IForceGenerator::~IForceGenerator() = default;

/**
 * @brief Adds this object.
 * @param bodyPtr Parameter for body ptr.
 * @param generator Parameter for generator.
 * @return Result produced by this operation.
 */
void ForceRegistry::Add(RigidBody2D* bodyPtr, IForceGenerator* generator) {
	entries.push_back({ bodyPtr, generator });
}

/**
 * @brief Removes this object.
 * @param bodyPtr Parameter for body ptr.
 * @param generator Parameter for generator.
 * @return Result produced by this operation.
 */
void ForceRegistry::Remove(RigidBody2D* bodyPtr, IForceGenerator* generator) {
	entries.erase(
		std::remove_if(entries.begin(), entries.end(),
			[&](const Entry& e) {
				return (e.body == bodyPtr) && (e.gen == generator);
			}),
		entries.end()
	);
}

/**
 * @brief Clears this object.
 * @return Result produced by this operation.
 */
void ForceRegistry::Clear() {
	entries.clear();
}

/**
 * @brief Updates forces.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void ForceRegistry::UpdateForces(float dt) {
	for (Entry& entry : entries) {
		if (entry.body && entry.gen) {
			entry.gen->UpdateForce(*entry.body, dt);
		}
	}
}

/**
 * @brief Performs gravity force.
 * @param gravity Parameter for gravity.
 * @return Result produced by this operation.
 */
GravityForce::GravityForce(Math::Vector2D gravity)
	: g(gravity) {
}

/**
 * @brief Updates force.
 * @param body Parameter for body.
 * @param float Parameter for float.
 * @return Result produced by this operation.
 */
void GravityForce::UpdateForce(RigidBody2D& body, float) {
	// Static bodies have inverse mass 0 (or less) ignore gravity
	if (body.GetInverseMass() <= 0.0f) {
		return;
	}

	// F = m * a
	body.AddForce(g * body.GetMass());
}

/**
 * @brief Performs drag force.
 * @param linearK Parameter for linear k.
 * @param quadraticK Parameter for quadratic k.
 * @return Result produced by this operation.
 */
DragForce::DragForce(float linearK, float quadraticK)
	: k1(linearK), k2(quadraticK) {
}

/**
 * @brief Updates force.
 * @param body Parameter for body.
 * @param float Parameter for float.
 * @return Result produced by this operation.
 */
void DragForce::UpdateForce(RigidBody2D& body, float) {
	const Math::Vector2D velocity = body.GetVelocity();
	const float speed = velocity.Length();

	// No drag if not moving.
	if (speed <= 1e-6f) {
		return;
	}

	const float dragMagnitude = (k1 * speed) + (k2 * speed * speed);

	// Direction opposite velocity; guard divide by zero with small epsilon.
	body.AddForce(velocity * (-dragMagnitude / (speed + 1e-6f)));
}

/**
 * @brief Performs constant force.
 * @param force Parameter for force.
 * @return Result produced by this operation.
 */
ConstantForce::ConstantForce(Math::Vector2D force)
	: f(force) {
}

/**
 * @brief Updates force.
 * @param body Parameter for body.
 * @param float Parameter for float.
 * @return Result produced by this operation.
 */
void ConstantForce::UpdateForce(RigidBody2D& body, float) {
	body.AddForce(f);
}

/**
 * @brief Performs seek force.
 * @param targetPtr Parameter for target ptr.
 * @param maxAccelIn Parameter for max accel in.
 * @param arrive Parameter for arrive.
 * @return Result produced by this operation.
 */
SeekForce::SeekForce(Math::Vector2D* targetPtr, float maxAccelIn, float arrive)
	: target(targetPtr), currentPos2DPtr(nullptr), maxAccel(maxAccelIn), arriveRadius(arrive) {
}

/**
 * @brief Performs seek force.
 * @param targetPtr Parameter for target ptr.
 * @param cur Parameter for cur.
 * @param maxAccelIn Parameter for max accel in.
 * @param arrive Parameter for arrive.
 * @return Result produced by this operation.
 */
SeekForce::SeekForce(Math::Vector2D* targetPtr,
	const Math::Vector2D* cur,
	float maxAccelIn,
	float arrive)
	: target(targetPtr), currentPos2DPtr(cur), maxAccel(maxAccelIn), arriveRadius(arrive) {
}

/**
 * @brief Updates force.
 * @param body Parameter for body.
 * @param float Parameter for float.
 * @return Result produced by this operation.
 */
void SeekForce::UpdateForce(RigidBody2D& body, float) {
	if (!target) {
		return;
	}

	// Use externally-supplied position if provided; otherwise, read from body.
	Math::Vector2D currentPos2D;
	if (currentPos2DPtr) {
		currentPos2D = *currentPos2DPtr;
	}
	else {
		const Math::Vector2D bodyPos = body.GetPosition();
		currentPos2D = bodyPos;
	}

	Math::Vector2D toTarget = (*target - currentPos2D);
	const float distance = toTarget.Length();

	// Arrive behavior: stop fully when within the deadzone
	if (distance <= arriveRadius) {
		body.Stop(); // hard-zero velocity to prevent creeping
		return;
	}

	if (distance > 1e-4f) {
		// Normalize and push with fixed acceleration (scaled by mass)
		const Math::Vector2D direction = toTarget * (1.0f / distance);
		const float accel = (maxAccel > 0.0f) ? maxAccel : 0.0f;
		body.AddForce(direction * (accel * body.GetMass()));
	}
}
