#include "RigidBody2D.hpp"
#include "GOC.hpp"
#include "Transform.hpp"

void RigidBody2D::Initialize()
{
	velocity = Math::Vector2D::ZERO;
	acceleration = Math::Vector2D::ZERO;
}

void RigidBody2D::Update(float dt)
{
    if (!IsEnabled()) return;

    if (useGravity) {
        acceleration = acceleration + Math::Vector2D(0.0f, -9.8f);
    }

    velocity = velocity + acceleration * dt;

    // Move Transform
    if (auto _transform = GetOwner()->Get<Transform>())
    {
        Transform* transform = *_transform;
        transform->SetPosition(transform->GetPosition() + velocity * dt);
    }

    // Reset acceleration (forces applied each frame only)
    acceleration = Math::Vector2D::ZERO;
}

Math::Vector2D const RigidBody2D::GetVelocity() const
{
    return velocity;
}

Math::Vector2D const RigidBody2D::GetAcceleration() const
{
    return acceleration;
}

void RigidBody2D::AddForce(const Math::Vector2D& force)
{
    // F = ma -> a = F/m 
    //mass is 1 right now
    acceleration = acceleration + force;
}

void RigidBody2D::SetVelocity(const Math::Vector2D& vel)
{
    velocity = vel;
}

void RigidBody2D::Stop()
{
    velocity = Math::Vector2D::ZERO;
}

std::string RigidBody2D::ToString() const
{
    return "Rigidbody2D (vel: " + std::to_string(velocity.x) + "," + std::to_string(velocity.y) + ")";
}