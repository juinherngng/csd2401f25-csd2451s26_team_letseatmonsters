/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			RigidBody2D.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (15%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu   (85%)

 DESCRIPTION:		Implements RigidBody2D. Accumulates forces, integrates velocity with
					exponential damping, optionally applies gravity/legacy acceleration,
					and writes motion to the owner's Transform.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <cmath>

#include "EngineCore/Forces.hpp"
#include "EngineCore/GOC.hpp"
#include "EngineCore/RigidBody2D.hpp"
#include "EngineCore/Transform.hpp"

/**
 * @brief Initializes this object.
 * @return Result produced by this operation.
 */
void RigidBody2D::Initialize() {
	velocity = Math::Vector2D::ZERO;
	acceleration = Math::Vector2D::ZERO;
	invMass = 1.0f;
	damping = 0.98f;
	useGravity = false;
	forceAccum = Math::Vector2D::ZERO;
	registry = nullptr;
}

/**
 * @brief Updates this object.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
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

/**
 * @brief Adds force.
 * @param force Parameter for force.
 * @return Result produced by this operation.
 */
void RigidBody2D::AddForce(const Math::Vector2D& force) {
	// Accumulate forces for this step
	forceAccum = forceAccum + force;
}

/**
 * @brief Adds impulse.
 * @param impulse Parameter for impulse.
 * @return Result produced by this operation.
 */
void RigidBody2D::AddImpulse(const Math::Vector2D& impulse) {
	if (invMass <= 0.0f) {
		return;
	}

	// Instant velocity change: v += J * invMass
	velocity = velocity + (impulse * invMass);
}

/**
 * @brief Returns velocity.
 * @return Requested value.
 */
Math::Vector2D const RigidBody2D::GetVelocity() const {
	return velocity;
}

/**
 * @brief Returns acceleration.
 * @return Requested value.
 */
Math::Vector2D const RigidBody2D::GetAcceleration() const {
	return acceleration;
}

/**
 * @brief Returns use gravity.
 * @return Requested value.
 */
bool const RigidBody2D::GetUseGravity() const {
	return useGravity;
}

/**
 * @brief Returns position.
 * @return Requested value.
 */
Math::Vector2D RigidBody2D::GetPosition() const {
	if (GOC* owner = GetOwner()) {
		if (auto transformHandle = owner->Get<Transform>()) {
			const Transform* transform = *transformHandle;
			return transform->GetPosition();
		}
	}

	return Math::Vector2D(0.0f, 0.0f);
}

/**
 * @brief Returns mass.
 * @return Requested value.
 */
float RigidBody2D::GetMass() const {
	return (invMass > 0.0f) ? (1.0f / invMass) : 0.0f;
}

/**
 * @brief Returns inverse mass.
 * @return Requested value.
 */
float RigidBody2D::GetInverseMass() const {
	return invMass;
}

/**
 * @brief Sets velocity.
 * @param newVelocity Parameter for new velocity.
 * @return Result produced by this operation.
 */
void RigidBody2D::SetVelocity(const Math::Vector2D& newVelocity) {
	velocity = newVelocity;
}

/**
 * @brief Sets acceleration.
 * @param newAcceleration Parameter for new acceleration.
 * @return Result produced by this operation.
 */
void RigidBody2D::SetAcceleration(const Math::Vector2D& newAcceleration) {
	acceleration = newAcceleration;
}

/**
 * @brief Sets use gravity.
 * @param enable Boolean flag controlling whether the feature is enabled.
 * @return Result produced by this operation.
 */
void RigidBody2D::SetUseGravity(const bool enable) {
	useGravity = enable;
}

/**
 * @brief Performs stop.
 * @return Result produced by this operation.
 */
void RigidBody2D::Stop() {
	velocity = Math::Vector2D::ZERO;
}

/**
 * @brief Sets mass.
 * @param mass Parameter for mass.
 * @return Result produced by this operation.
 */
void RigidBody2D::SetMass(float mass) {
	invMass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
}

/**
 * @brief Sets linear damping.
 * @param value Parameter for value.
 * @return Result produced by this operation.
 */
void RigidBody2D::SetLinearDamping(float value) {
	damping = value;
}

/**
 * @brief Sets force registry.
 * @param fr Parameter for fr.
 * @return Result produced by this operation.
 */
void RigidBody2D::SetForceRegistry(ForceRegistry* fr) {
	registry = fr;
}

/**
 * @brief Performs to string.
 * @return Result produced by this operation.
 */
std::string RigidBody2D::ToString() const {
	return "Rigidbody2D (vel: " + std::to_string(velocity.x) + "," + std::to_string(velocity.y) + ")";
}

/**
 * @brief Performs clone.
 * @return Result produced by this operation.
 */
GameComponent* RigidBody2D::Clone() const {
	return new RigidBody2D(*this);
}

/**
 * @brief Performs integrate.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
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

/**
 * @brief Clears accum.
 * @return Result produced by this operation.
 */
void RigidBody2D::ClearAccum() {
	forceAccum = Math::Vector2D::ZERO;
}
