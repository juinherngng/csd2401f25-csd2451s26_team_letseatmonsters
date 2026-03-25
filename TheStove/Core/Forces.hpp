/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Forces.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Declares a simple force system for 2D rigid bodies:
					- IForceGenerator: interface for all force sources
					- ForceRegistry:   maps bodies <-> force generators and updates them
					- GravityForce:    constant acceleration (F = m * g)
					- DragForce:       linear + quadratic velocity drag
					- ConstantForce:   applies a fixed world-space force each step
					- SeekForce:       seek/arrive steering toward a target with deadzone

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <algorithm>
#include <vector>

#include "Math.hpp"
#include "RigidBody2D.hpp"

 // Interface: any force source implements this.
struct IForceGenerator {

	/**
	 * @brief Destroys the `IForceGenerator` instance and releases owned resources.
	 * @return Result produced by this operation.
	 */
	virtual ~IForceGenerator();

	/**
	 * @brief Updates force.
	 * @param body Parameter for body.
	 * @param dt Frame delta time in seconds.
	 * @return Result produced by this operation.
	 */
	virtual void UpdateForce(RigidBody2D& body, float dt) = 0;
};

// Registry: holds body <-> generator pairs and updates all per frame.
class ForceRegistry {
public:
	// Types
	struct Entry {
		RigidBody2D* body = nullptr;
		IForceGenerator* gen = nullptr;
	};

	/**
	 * @brief Adds this object.
	 * @param body Parameter for body.
	 * @param gen Parameter for gen.
	 */
	void Add(RigidBody2D* body, IForceGenerator* gen);

	/**
	 * @brief Removes this object.
	 * @param body Parameter for body.
	 * @param gen Parameter for gen.
	 */
	void Remove(RigidBody2D* body, IForceGenerator* gen);

	/**
	 * @brief Clears this object.
	 */
	void Clear();

	/**
	 * @brief Updates forces.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateForces(float dt);

private:
	// Data Members
	std::vector<Entry> entries;
};

// Concrete forces
struct GravityForce : IForceGenerator {
	// Data
	Math::Vector2D g{};

	/**
	 * @brief Constructs a `GravityForce` instance.
	 * @param gravity Parameter for gravity.
	 * @return Result produced by this operation.
	 */
	explicit GravityForce(Math::Vector2D gravity);

	/**
	 * @brief Updates force.
	 * @param body Parameter for body.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateForce(RigidBody2D& body, float dt) override;
};

// Drag force with linear and quadratic terms.
struct DragForce : IForceGenerator {
	// Data
	float k1 = 0.0f; // linear term
	float k2 = 0.0f; // quadratic term

	/**
	 * @brief Constructs a `DragForce` instance.
	 * @param linearK Parameter for linear k.
	 * @param quadraticK Parameter for quadratic k.
	 */
	DragForce(float linearK, float quadraticK);

	/**
	 * @brief Updates force.
	 * @param body Parameter for body.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateForce(RigidBody2D& body, float dt) override;
};

// Applies a fixed world-space force every update.
struct ConstantForce : IForceGenerator {
	// Data
	Math::Vector2D f{ 0.0f, 0.0f };

	/**
	 * @brief Constructs a `ConstantForce` instance.
	 * @param force Parameter for force.
	 * @return Result produced by this operation.
	 */
	explicit ConstantForce(Math::Vector2D force);

	/**
	 * @brief Updates force.
	 * @param body Parameter for body.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateForce(RigidBody2D& body, float dt) override;
};

// Seek / Arrive steering toward a target with max acceleration and arrive radius.
struct SeekForce : IForceGenerator {
	// Data
	Math::Vector2D* target = nullptr;                 // Destination (mutable externally)
	const Math::Vector2D* currentPos2DPtr = nullptr;  // Optional external position to use instead of body position
	float maxAccel = 600.0f;                          // Acceleration magnitude
	float arriveRadius = 6.0f;                        // Deadzone to stop and zero velocity

	/**
	 * @brief Constructs a `SeekForce` instance.
	 * @param targetPtr Parameter for target ptr.
	 * @param maxAccelIn Parameter for max accel in.
	 * @param arrive Parameter for arrive.
	 */
	SeekForce(Math::Vector2D* targetPtr, float maxAccelIn = 600.0f, float arrive = 6.0f);

	/**
	 * @brief Constructs a `SeekForce` instance.
	 * @param targetPtr Parameter for target ptr.
	 * @param currentPosPtr Parameter for current pos ptr.
	 * @param maxAccelIn Parameter for max accel in.
	 * @param arrive Parameter for arrive.
	 */
	SeekForce(Math::Vector2D* targetPtr,
		const Math::Vector2D* currentPosPtr,
		float maxAccelIn = 600.0f,
		float arrive = 6.0f);

	/**
	 * @brief Updates force.
	 * @param body Parameter for body.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateForce(RigidBody2D& body, float dt) override;
};

