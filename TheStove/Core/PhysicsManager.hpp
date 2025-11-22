/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         PhysicsManager.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu
					Ng Juin Herng, juinherng.ng@digipen.edu

 DESCRIPTION:       Declares PhysicsManager, a lightweight force-based integrator that updates
					per-entity velocity/position, supports seek/arrive targets, simple drag, and
					world-aware movement trimming (stop on wall impact). Works alongside the
					MovementManager for click-to-move UX.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <unordered_map>

#include "../Graphics/EntityManager.hpp"

#include "CollisionManager.hpp"
#include "Forces.hpp"
#include "InputManager.hpp"
#include "Math.hpp"
#include "MovementManager.hpp"
#include "Physics.hpp"
#include "System.hpp"

 /**
  * @brief Lightweight force-based physics for entities.
  *
  * Manages per-entity physics states (mass, velocity, damping) and integrates them
  * under forces such as seek/arrive and drag. Movement is trimmed against the world
  * using CollisionManager::resolve(); on impact, the active seek is cleared and
  * velocity is zeroed (hard stop).
  */
class PhysicsManager : public CoreFramework::SystemInterface {
public:
	PhysicsManager() = default;
	~PhysicsManager() override = default;

	// SystemInterface implementation
	void Initialize() override;
	void Update(float dt) override;
	std::string GetName() override;

	// Set EntityManager and InputManager references (must be called after construction)
	void SetEntityManager(EntityManager* entityMgr);
	void SetInputManager(InputManager* inputMgr);

	// World collision/trim resolver used to clamp movement each step.
	void SetCollisionWorld(collision::World* world);

	// Core physics update (original signature - now called internally)
	void UpdatePhysics(float deltaTime,
					   EntityManager& entityManager,
					   InputManager& inputManager);

	// Enable physics on an entity and initialize its state.
	void EnablePhysics(int entityID, float mass = 1.0f);

	// Disable physics on an entity(removes state and any active seek).
	void DisablePhysics(int entityID);

	// Returns true if an entity has a physics state.
	bool HasPhysics(int entityID) const;

	// Set/replace a seek target for an entity (arrive behavior inside).
	void SetSeekTarget(int entityID, const Math::Vector2D& target);

	// Clear an entity's seek target and stop the body immediately.
	void ClearSeekTarget(int entityID);

	// Clear all physics states and seek targets.
	void Clear();

	// Access to step controller
	physics::StepController& GetStepController() {
		return physicsStep_;
	}

	// Movement system (used to clear click-to-move targets on impact).
	void SetMovementManager(MovementManager* m) {
		movement_ = m;
	}

private:
	// Internal state & constants
	struct PhysicsState {
		float invMass = 0.0f;  // 0 => infinite mass (immovable)
		float damping = 0.98f; // Exponential damping factor per second
		Math::Vector2D velocity{ 0.0f, 0.0f };
		Math::Vector2D forceAccum{ 0.0f, 0.0f };
	};

	// Per-entity physics state
	std::unordered_map<int, PhysicsState> physicsStates_;

	// Per-entity seek targets
	std::unordered_map<int, Math::Vector2D> seekTargets_;

	// Force generators (shared across all entities)
	DragForce dragForce_{ 0.9f, 0.1f };     // Linear & quadratic drag coefficients

	// Fixed - step resolver
	physics::StepController physicsStep_;

	// External systems
	const collision::World* world_ = nullptr;
	MovementManager* movement_ = nullptr;
	EntityManager* entityManager_ = nullptr;
	InputManager* inputManager_ = nullptr;

	// Constants
	static constexpr float SEEK_MAX_ACCEL = 600.0f;
	static constexpr float ARRIVE_RADIUS = 10.0f;

	// Integrate one entity by dt, applying seek/arrive and drag, then trimming movement against world; hard - stop on impact.
	void IntegrateEntity(int entityID, float dt, EntityManager& entityManager);
};
