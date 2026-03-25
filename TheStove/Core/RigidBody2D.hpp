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

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "GameComponent.hpp"
#include "Math.hpp"

#include <string>

class Transform;
class ForceRegistry;

// A simple 2D rigid body component for basic physics simulation.
class RigidBody2D : public GameComponent {
public:

	/**
	 * @brief Constructs a `RigidBody2D` instance.
	 */
	RigidBody2D() : velocity(Math::Vector2D::ZERO), acceleration(Math::Vector2D::ZERO)/*, mass(0.0f)*/, useGravity(false) {
	};

	/**
	 * @brief Initializes this object.
	 */
	void Initialize() override;

	/**
	 * @brief Updates this object.
	 * @param dt Frame delta time in seconds.
	 */
	void Update(float dt) override;

	/**
	 * @brief Adds force.
	 * @param force Parameter for force.
	 */
	void AddForce(const Math::Vector2D& force);

	/**
	 * @brief Adds impulse.
	 * @param impulse Parameter for impulse.
	 */
	void AddImpulse(const Math::Vector2D& impulse);

	/**
	 * @brief Returns velocity.
	 * @return Requested value.
	 */
	Math::Vector2D const GetVelocity() const;

	/**
	 * @brief Returns acceleration.
	 * @return Requested value.
	 */
	Math::Vector2D const GetAcceleration() const;

	/**
	 * @brief Returns use gravity.
	 * @return Requested value.
	 */
	bool const GetUseGravity() const;

	/**
	 * @brief Returns position.
	 * @return Requested value.
	 */
	Math::Vector2D GetPosition() const;

	/**
	 * @brief Returns mass.
	 * @return Requested value.
	 */
	float GetMass() const;

	/**
	 * @brief Returns inverse mass.
	 * @return Requested value.
	 */
	float GetInverseMass() const;

	/**
	 * @brief Sets velocity.
	 * @param vel Parameter for vel.
	 */
	void SetVelocity(const Math::Vector2D& vel);

	/**
	 * @brief Sets acceleration.
	 * @param accel Parameter for accel.
	 */
	void SetAcceleration(const Math::Vector2D& accel);

	/**
	 * @brief Sets use gravity.
	 * @param b Parameter for b.
	 */
	void SetUseGravity(const bool b);

	/**
	 * @brief Performs stop.
	 */
	void Stop();

	/**
	 * @brief Sets mass.
	 * @param m Parameter for m.
	 */
	void SetMass(float m);

	/**
	 * @brief Sets linear damping.
	 * @param d Parameter for d.
	 */
	void SetLinearDamping(float d);

	/**
	 * @brief Sets force registry.
	 * @param fr Parameter for fr.
	 */
	void SetForceRegistry(ForceRegistry* fr);

	/**
	 * @brief Performs to string.
	 * @return Result produced by this operation.
	 */
	std::string ToString() const override;

	/**
	 * @brief Performs clone.
	 * @return Result produced by this operation.
	 */
	GameComponent* Clone() const override;

	/**
	 * @brief Destroys the `RigidBody2D` instance and releases owned resources.
	 */
	~RigidBody2D() override = default;
private:

	/**
	 * @brief Performs integrate.
	 * @param dt Frame delta time in seconds.
	 */
	void Integrate(float dt);

	/**
	 * @brief Clears accum.
	 */
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

