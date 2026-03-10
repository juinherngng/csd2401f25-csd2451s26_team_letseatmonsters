/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			RigidBody2D.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (20%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu	  (80%)

 DESCRIPTION:		Declares a lightweight 2D rigid body component that stores linear state
					(velocity, mass, damping), accumulates forces/impulses, integrates motion,
					and writes back to an attached Transform. Designed to be used with a
					ForceRegistry and simple steering forces (seek, drag, gravity).

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "GameComponent.hpp"
#include "Math.hpp"

#include <iostream>
#include <string>

class Transform;
class ForceRegistry;

// A simple 2D rigid body component for basic physics simulation.
class RigidBody2D : public GameComponent {
public:
	RigidBody2D() : velocity(Math::Vector2D::ZERO), acceleration(Math::Vector2D::ZERO)/*, mass(0.0f)*/, useGravity(false) {
	};

	// GameComponent interface
	void Initialize() override;
	void Update(float dt) override;

	// Force application
	void AddForce(const Math::Vector2D& force);
	void AddImpulse(const Math::Vector2D& impulse);

	// Getters
	Math::Vector2D const GetVelocity() const;
	Math::Vector2D const GetAcceleration() const;
	bool const GetUseGravity() const;
	Math::Vector2D GetPosition() const;
	float GetMass() const;
	float GetInverseMass() const;

	// Setters
	void SetVelocity(const Math::Vector2D& vel);
	void SetAcceleration(const Math::Vector2D& accel);
	void SetUseGravity(const bool b);

	// Hard-stop the body (velocity = 0).
	void Stop();

	// Set the mass (<= 0 becomes infinite mass).
	void SetMass(float m);

	// Set exponential linear damping (applied as damping^dt).
	void SetLinearDamping(float d);

	// Attach a ForceRegistry that updates external force generators.
	void SetForceRegistry(ForceRegistry* fr);

	// GameComponent overrides
	std::string ToString() const override;
	GameComponent* Clone() const override;

	~RigidBody2D() override {
		std::cout << "Deleting RigidBody2D's component " << "\n";
	}
private:
	// Internal integration method to update velocity and position based on accumulated forces and damping.
	void Integrate(float dt);
	void ClearAccum();

private:
	// State variables
	Math::Vector2D velocity;        // pixels/sec
	Math::Vector2D acceleration;    // legacy external accel (converted to force)
	Math::Vector2D forceAccum;      // sum of forces for this step

	// Physical parameters
	float invMass = 1.0f;
	float damping = 0.98f;
	bool useGravity = false;

	// Force system
	ForceRegistry* registry = nullptr;
};