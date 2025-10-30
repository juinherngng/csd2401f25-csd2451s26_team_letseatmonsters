/*
----------------------------------------------------------------------------------------------------
FILE NAME:			RigidBody2D.hpp
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

#pragma once
#include "GameComponent.hpp"
#include "Math.hpp"
#include <iostream>
#include <string>

class Transform;
class ForceRegistry;

class RigidBody2D : public GameComponent
{
public:
	RigidBody2D() : velocity(Math::Vector2D::ZERO), acceleration(Math::Vector2D::ZERO)/*, mass(0.0f)*/, useGravity(false) {};
	void Initialize() override;
	void Update(float dt) override;

	Math::Vector2D const GetVelocity() const;
	Math::Vector2D const GetAcceleration() const;
	bool const GetUseGravity() const;

	void AddForce(const Math::Vector2D& force);
	void AddImpulse(const Math::Vector2D& impulse);

	void SetVelocity(const Math::Vector2D& vel);
	void SetAcceleration(const Math::Vector2D& accel); // optional direct accel
	void SetUseGravity(const bool b);
	void Stop();

	void SetMass(float m);
	float GetMass() const { return invMass > 0.f ? 1.0f / invMass : 0.f; }
	float GetInverseMass() const { return invMass; }
	void SetLinearDamping(float d) { damping = d; }

	Math::Vector2D GetPosition() const;

	std::string ToString() const override;

	void SetForceRegistry(ForceRegistry* fr) { registry = fr; }

	~RigidBody2D() override
	{
		std::cout << "Deleting RigidBody2D's component " << "\n";
	}

	GameComponent* Clone() const override;

private:
	// Integrator helpers
	void Integrate(float dt);
	void ClearAccum() { forceAccum = Math::Vector2D::ZERO; }

	Math::Vector2D velocity{ 0,0 };
	Math::Vector2D acceleration{ 0,0 }; // external (optional)
	Math::Vector2D forceAccum{ 0,0 };   // NEW: sum of forces this step
	float invMass = 1.0f;             // default mass = 1
	float damping = 0.98f;            // simple exponential damping per second
	bool useGravity = false;

	ForceRegistry* registry = nullptr; // not owned
};