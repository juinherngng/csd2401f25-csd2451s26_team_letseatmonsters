/*
----------------------------------------------------------------------------------------------------
FILE NAME:			RigidBody2D.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Vu Phan Hung, phanhung.vu@digipen.edu
CO-AUTHORS:			Yat Chun Wee, y.chunwee@digipen.edu

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

#include <iostream>
#include <string>

#include "GameComponent.hpp"
#include "Math.hpp"

class Transform;
class ForceRegistry;

class RigidBody2D : public GameComponent
{
public:
	RigidBody2D() : velocity(Math::Vector2D::ZERO), acceleration(Math::Vector2D::ZERO)/*, mass(0.0f)*/, useGravity(false) {};

	// Lifecycle
	void Initialize() override;
	void Update(float dt) override;

	// Public Physics API
	void AddForce(const Math::Vector2D& force);
	void AddImpulse(const Math::Vector2D& impulse);

	// Queries
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

	void Stop();
	void SetMass(float m);
	void SetLinearDamping(float d);
	void SetForceRegistry(ForceRegistry* fr);

	// Debug / Utility
	std::string ToString() const override;
	GameComponent* Clone() const override;

	~RigidBody2D() override
	{
		std::cout << "Deleting RigidBody2D's component " << "\n";
	}
private:
	// Integrator helpers
	void Integrate(float dt);
	void ClearAccum();

	// State
	Math::Vector2D velocity{ 0,0 };
	Math::Vector2D acceleration{ 0,0 };
	Math::Vector2D forceAccum{ 0,0 };

	float invMass = 1.0f;
	float damping = 0.98f;
	bool useGravity = false;

	ForceRegistry* registry = nullptr; // not owned
};