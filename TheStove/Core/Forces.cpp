/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Forces.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Implements the force system (registry + generators). Each generator applies
					its force to a body during UpdateForce; the registry iterates all pairs.

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <iostream> 

#include "Forces.hpp"
#include "RigidBody2D.hpp"

#define FORCE_DEBUG // uncomment to show per-frame force application logs

 // IForceGenerator 
IForceGenerator::~IForceGenerator() = default;

// ForceRegistry
void ForceRegistry::Add(RigidBody2D* bodyPtr, IForceGenerator* generator) {
	entries.push_back({ bodyPtr, generator });
}

void ForceRegistry::Remove(RigidBody2D* bodyPtr, IForceGenerator* generator) {
	entries.erase(
		std::remove_if(entries.begin(), entries.end(),
					   [&](const Entry& e) {
		return (e.body == bodyPtr) && (e.gen == generator);
	}),
		entries.end()
	);
}

void ForceRegistry::Clear() {
	entries.clear();
}

void ForceRegistry::UpdateForces(float dt) {
	for (Entry& entry : entries) {
		if (entry.body && entry.gen) {
#ifdef FORCE_DEBUG
			std::cout << "[Force] Applying " << typeid(*entry.gen).name() << " to body\n";
#endif
			entry.gen->UpdateForce(*entry.body, dt);
		}
	}
}

// GravityForce
GravityForce::GravityForce(Math::Vector2D gravity)
	: g(gravity) {
}

void GravityForce::UpdateForce(RigidBody2D& body, float) {
	// Static bodies have inverse mass 0 (or less) � ignore gravity
	if (body.GetInverseMass() <= 0.0f) {
		return;
	}

	// F = m * a
	body.AddForce(g * body.GetMass());
}

// DragForce
DragForce::DragForce(float linearK, float quadraticK)
	: k1(linearK), k2(quadraticK) {
}

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

// ConstantForce
ConstantForce::ConstantForce(Math::Vector2D force)
	: f(force) {
}

void ConstantForce::UpdateForce(RigidBody2D& body, float) {
	body.AddForce(f);
}

// SeekForce
SeekForce::SeekForce(Math::Vector2D* targetPtr, float maxAccelIn, float arrive)
	: target(targetPtr), currentPos2DPtr(nullptr), maxAccel(maxAccelIn), arriveRadius(arrive) {
}

SeekForce::SeekForce(Math::Vector2D* targetPtr,
					 const Math::Vector2D* cur,
					 float maxAccelIn,
					 float arrive)
	: target(targetPtr), currentPos2DPtr(cur), maxAccel(maxAccelIn), arriveRadius(arrive) {
}

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
		const float accel = (maxAccel > 0.0f)?maxAccel:0.0f;
		body.AddForce(direction * (accel * body.GetMass()));
	}
}
