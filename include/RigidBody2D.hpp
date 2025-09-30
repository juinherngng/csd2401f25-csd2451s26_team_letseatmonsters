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

	void AddForce(const Math::Vector2D&);
	void SetVelocity(const Math::Vector2D&);
	void Stop();

	std::string ToString() const override;

	~RigidBody2D() override
	{
		std::cout << "Deleting RigidBody2D's component " << "\n";
	}

private:
	Math::Vector2D velocity;
	Math::Vector2D acceleration;
	//float mass;
	bool useGravity;
};