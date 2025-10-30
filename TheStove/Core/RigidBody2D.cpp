/*
----------------------------------------------------------------------------------------------------
FILE NAME:			RigidBody2D.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Vu Phan Hung, phanhung.vu@digipen.edu

DESCRIPTION:
	Physics component representing a 2D rigid body with velocity.
	Designed to work with a Transform to simulate simple motion.

	Responsibilities:
	- Store and update linear velocity.
	- Apply velocity to the attached Transform each frame (in Update).
	- Provide simple physics behavior (e.g., movement, collision stubs).

	Intended as a starting point for the engine’s physics system.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <cmath>

#include "RigidBody2D.hpp"
#include "GOC.hpp"
#include "Transform.hpp"
#include "Forces.hpp"

void RigidBody2D::Initialize() {
	velocity = Math::Vector2D::ZERO;
	acceleration = Math::Vector2D::ZERO;
	invMass = 1.0f;     // mass = 1 by default
	damping = 0.98f;
	useGravity = false;
	forceAccum = Math::Vector2D::ZERO;
}

void RigidBody2D::Update(float dt)
{
	if (!IsEnabled() || dt <= 0.f) return;

	if (registry) registry->UpdateForces(dt);

	if (useGravity) {
		AddForce(Math::Vector2D(0.0f, -9.8f) * GetMass());
	}

	// velocity = velocity + acceleration * dt;

	// External acceleration field is still supported (legacy)
	// Total force = accum + (acceleration * m)
	AddForce(acceleration * GetMass());

	Integrate(dt);

	// Reset per-frame contributors
	acceleration = Math::Vector2D::ZERO;
	ClearAccum();
}

void RigidBody2D::Integrate(float dt) {
	if (invMass <= 0.f) return; // infinite mass = static

	// a = F * invM
	Math::Vector2D a = forceAccum * invMass;

	// Semi-implicit Euler: v += a * dt; v *= damping^dt; x += v * dt;
	velocity = velocity + a * dt;

	// simple exponential damping
	if (damping > 0.f && damping < 1.0f) {
		const float k = std::pow(damping, dt);
		velocity = velocity * k;
	}

	if (auto _transform = GetOwner()->Get<Transform>()) {
		Transform* transform = *_transform;
		transform->SetPosition(transform->GetPosition() + velocity * dt);
	}
}

void RigidBody2D::AddForce(const Math::Vector2D& force) {
	forceAccum = forceAccum + force; // no more assuming mass=1
}

void RigidBody2D::AddImpulse(const Math::Vector2D& impulse) {
	if (invMass <= 0.f) return;
	velocity = velocity + impulse * invMass;
}

Math::Vector2D const RigidBody2D::GetVelocity() const
{
	return velocity;
}

Math::Vector2D const RigidBody2D::GetAcceleration() const
{
	return acceleration;
}

bool const RigidBody2D::GetUseGravity() const
{
	return useGravity;
}

void RigidBody2D::SetVelocity(const Math::Vector2D& vel) { velocity = vel; }
void RigidBody2D::SetAcceleration(const Math::Vector2D& accel) { acceleration = accel; }
void RigidBody2D::SetUseGravity(const bool b) { useGravity = b; }

void RigidBody2D::Stop() { velocity = Math::Vector2D::ZERO; }

void RigidBody2D::SetMass(float m) { invMass = (m > 0.f) ? (1.0f / m) : 0.f; }

Math::Vector2D RigidBody2D::GetPosition() const {
	if (auto _transform = GetOwner()->Get<Transform>()) {
		const Transform* t = *_transform;
		return t->GetPosition(); // returns Vector2D already
	}
	return Math::Vector2D(0, 0);
}

std::string RigidBody2D::ToString() const
{
	return "Rigidbody2D (vel: " + std::to_string(velocity.x) + "," + std::to_string(velocity.y) + ")";
}

GameComponent* RigidBody2D::Clone() const
{
	return new RigidBody2D(*this);
}