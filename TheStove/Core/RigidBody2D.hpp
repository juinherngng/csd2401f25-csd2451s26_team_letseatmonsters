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

class RigidBody2D : public GameComponent
{
public:
	RigidBody2D() : velocity(Math::Vector2D::ZERO), acceleration(Math::Vector2D::ZERO)/*, mass(0.0f)*/, useGravity(false) {};
	void Initialize() override;
	void Update(float dt) override;

	Math::Vector2D const GetVelocity() const;
	Math::Vector2D const GetAcceleration() const;
	bool const GetUseGravity() const;

	void AddForce(const Math::Vector2D&);
	void SetVelocity(const Math::Vector2D&);
	void SetAcceleration(const Math::Vector2D&);
	void SetUseGravity(const bool);
	void Stop();

	std::string ToString() const override;

	~RigidBody2D() override
	{
		std::cout << "Deleting RigidBody2D's component " << "\n";
	}

	GameComponent* Clone() const override;

private:
	Math::Vector2D velocity;
	Math::Vector2D acceleration;
	//float mass;
	bool useGravity;
};