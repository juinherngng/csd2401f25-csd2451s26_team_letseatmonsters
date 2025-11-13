/*
----------------------------------------------------------------------------------------------------
FILE NAME:			RigidBody2D.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Vu Phan Hung, phanhung.vu@digipen.edu
CO-AUTHORS:			Yat Chun Wee, y.chunwee@digipen.edu

DESCRIPTION:		Implements RigidBody2D. Accumulates forces, integrates velocity with
					exponential damping, optionally applies gravity/legacy acceleration,
					and writes motion to the owner's Transform.

All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <cmath>

#include "Forces.hpp"
#include "GOC.hpp"
#include "RigidBody2D.hpp"
#include "Transform.hpp"

// Lifecycle
void RigidBody2D::Initialize() {
	velocity = Math::Vector2D::ZERO;
	acceleration = Math::Vector2D::ZERO;
	invMass = 1.0f;
	damping = 0.98f;
	useGravity = false;
	forceAccum = Math::Vector2D::ZERO;
	registry = nullptr;
}

void RigidBody2D::Update(float dt) {
	// Disabled or no time passed
	if (!IsEnabled()) {
		return;
	}

	if (dt <= 0.0f) {
		return;
	}

	// External force generators
	if (registry) {
		registry->UpdateForces(dt);
	}

	// Optional global gravity (kept for compatibility)
	if (useGravity) {
		AddForce(Math::Vector2D(0.0f, -9.8f) * GetMass());
	}

	// Legacy external acceleration still supported (converted to force)
	AddForce(acceleration * GetMass());

	// Integrate state
	Integrate(dt);

	// Clear per frame contributors
	acceleration = Math::Vector2D::ZERO;
	ClearAccum();
}

// Forces / Impulses
void RigidBody2D::AddForce(const Math::Vector2D& force) {
	// Accumulate forces for this step
	forceAccum = forceAccum + force;
}

void RigidBody2D::AddImpulse(const Math::Vector2D& impulse) {
	if (invMass <= 0.0f) {
		return;
	}

	// Instant velocity change: v += J * invMass
	velocity = velocity + (impulse * invMass);
}

// Queries
Math::Vector2D const RigidBody2D::GetVelocity() const {
	return velocity;
}

Math::Vector2D const RigidBody2D::GetAcceleration() const {
	return acceleration;
}

bool const RigidBody2D::GetUseGravity() const {
	return useGravity;
}

Math::Vector2D RigidBody2D::GetPosition() const {
	if (GOC* owner = GetOwner()) {
		if (auto transformHandle = owner->Get<Transform>()) {
			const Transform* transform = *transformHandle;
			return transform->GetPosition();
		}
	}

	return Math::Vector2D(0.0f, 0.0f);
}

float RigidBody2D::GetMass() const {
	return (invMass > 0.0f) ? (1.0f / invMass) : 0.0f;
}

float RigidBody2D::GetInverseMass() const {
	return invMass;
}

// Setters
void RigidBody2D::SetVelocity(const Math::Vector2D& newVelocity) {
	velocity = newVelocity;
}

void RigidBody2D::SetAcceleration(const Math::Vector2D& newAcceleration) {
	acceleration = newAcceleration;
}

void RigidBody2D::SetUseGravity(const bool enable) {
	useGravity = enable;
}

void RigidBody2D::Stop() {
	velocity = Math::Vector2D::ZERO;
}

void RigidBody2D::SetMass(float mass) {
	invMass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
}

void RigidBody2D::SetLinearDamping(float value) {
	damping = value;
}

void RigidBody2D::SetForceRegistry(ForceRegistry* fr) {
	registry = fr;
}

// Utilities
std::string RigidBody2D::ToString() const {
	return "Rigidbody2D (vel: " + std::to_string(velocity.x) + "," + std::to_string(velocity.y) + ")";
}

GameComponent* RigidBody2D::Clone() const {
	return new RigidBody2D(*this);
}

// Internal
void RigidBody2D::Integrate(float dt) {
	// Infinite mass - static body
	if (invMass <= 0.0f) {
		return;
	}

	// a = F * invMass
	const Math::Vector2D accelFromForces = forceAccum * invMass;

	// v += a * dt
	velocity = velocity + (accelFromForces * dt);

	// Exponential linear damping (velocity *= damping^dt)
	if (damping > 0.0f && damping < 1.0f) {
		const float dampingFactor = std::pow(damping, dt);
		velocity = velocity * dampingFactor;
	}

	// Move Transform if we actually have one
	if (GOC* owner = GetOwner()) {
		if (auto transformHandle = owner->Get<Transform>()) {
			Transform* transform = *transformHandle;
			transform->SetPosition(transform->GetPosition() + (velocity * dt));
		}
	}
}

void RigidBody2D::ClearAccum() {
	forceAccum = Math::Vector2D::ZERO;
}
