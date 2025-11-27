/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			RigidBody2D.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Declares a lightweight 2D rigid body component that stores linear state
					(velocity, mass, damping), accumulates forces/impulses, integrates motion,
					and writes back to an attached Transform. Designed to be used with a
					ForceRegistry and simple steering forces (seek, drag, gravity).

		All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <iostream>
#include <string>

#include "GameComponent.hpp"
#include "Math.hpp"

class Transform;
class ForceRegistry;

/**
 * @class RigidBody2D
 * @brief Physics component representing a simple 2D rigid body with linear motion.
 *
 * Responsibilities:
 *  - Accumulate forces & impulses each frame and integrate velocity/position.
 *  - Optionally apply gravity or externally supplied acceleration.
 *  - Write motion to the owner's Transform (if present).
 */
class RigidBody2D : public GameComponent {
public:
	RigidBody2D() : velocity(Math::Vector2D::ZERO), acceleration(Math::Vector2D::ZERO)/*, mass(0.0f)*/, useGravity(false) {
	};

	// ----- Lifecycle -----

	// One-time setup to initialize defaults.
	void Initialize() override;

	// Per-frame update: apply forces, integrate, and move Transform.
	void Update(float dt) override;

	// Add a force to be applied this frame (accumulates until integration).
	void AddForce(const Math::Vector2D& force);

	// Add an instantaneous impulse (changes velocity immediately).
	void AddImpulse(const Math::Vector2D& impulse);

	// ----- Queries -----

	// Current linear velocity.
	Math::Vector2D const GetVelocity() const;

	// Current linear acceleration (legacy external accel).
	Math::Vector2D const GetAcceleration() const;

	// Whether gravity is applied in Update().
	bool const GetUseGravity() const;

	// Current world position read from the Transform (if any).
	Math::Vector2D GetPosition() const;

	// Mass in kg. Returns 0 for infinite mass.
	float GetMass() const;

	// Inverse mass (0 for infinite mass / immovable).
	float GetInverseMass() const;

	// ----- Setters -----

	// Replace current velocity.
	void SetVelocity(const Math::Vector2D& vel);

	// Replace current (legacy) acceleration; converted to force during Update().
	void SetAcceleration(const Math::Vector2D& accel);

	// Enable/disable simple gravity in Update().
	void SetUseGravity(const bool b);

	// Hard-stop the body (velocity = 0).
	void Stop();

	// Set the mass (<= 0 becomes infinite mass).
	void SetMass(float m);

	// Set exponential linear damping (applied as damping^dt).
	void SetLinearDamping(float d);

	// Attach a ForceRegistry that updates external force generators.
	void SetForceRegistry(ForceRegistry* fr);

	// ----- Utilities -----

	// Human-readable state summary.
	std::string ToString() const override;

	// Clone this component (kept to match engine conventions).
	GameComponent* Clone() const override;

	~RigidBody2D() override {
		std::cout << "Deleting RigidBody2D's component " << "\n";
	}
private:
	// ----- Internal helpers -----

	// Integrate velocity and write back to Transform.
	void Integrate(float dt);

	// Clear per-frame accumulated forces.
	void ClearAccum();

private:
	// ----- Linear state -----
	Math::Vector2D velocity;        // pixels/sec
	Math::Vector2D acceleration;    // legacy external accel (converted to force)
	Math::Vector2D forceAccum;      // sum of forces for this step

	// ----- Physical parameters -----
	float invMass = 1.0f;
	float damping = 0.98f;
	bool useGravity = false;

	// ----- Force system -----
	ForceRegistry* registry = nullptr;
};