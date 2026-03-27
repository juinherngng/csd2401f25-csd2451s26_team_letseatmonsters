/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         PhysicsManager.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu	(30%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(65%)
					Ng Juin Herng, juinherng.ng@digipen.edu (5%)

 DESCRIPTION:       Declares PhysicsManager, a lightweight force-based integrator that updates
					per-entity velocity/position, supports seek/arrive targets, simple drag, and
					world-aware movement trimming (stop on wall impact). Works alongside the
					MovementManager for click-to-move UX.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <unordered_map>

#include "EngineCore/CollisionManager.hpp"
#include "EngineCore/Forces.hpp"
#include "EngineCore/InputManager.hpp"
#include "EngineCore/Math.hpp"
#include "EngineCore/MovementManager.hpp"
#include "EngineCore/Physics.hpp"
#include "EngineCore/System.hpp"
#include "EngineGraphics/EntityManager.hpp"

class Scene;

// Forward declare GameObject to avoid circular dependency.
class PhysicsManager : public CoreFramework::SystemInterface {
public:

	/**
	 * @brief Constructs a `PhysicsManager` instance.
	 */
	PhysicsManager() = default;

	/**
	 * @brief Destroys the `PhysicsManager` instance and releases owned resources.
	 */
	~PhysicsManager() override = default;

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
	 * @brief Returns the stable name for this object.
	 * @return Requested value.
	 */
	std::string GetName() override;

	/**
	 * @brief Sets entity manager.
	 * @param entityMgr Parameter for entity mgr.
	 */
	void SetEntityManager(EntityManager* entityMgr);

	/**
	 * @brief Sets input manager.
	 * @param inputMgr Parameter for input mgr.
	 */
	void SetInputManager(InputManager* inputMgr);

	/**
	 * @brief Sets scene.
	 * @param scene Scene being processed.
	 */
	void SetScene(Scene* scene) {
		scene_ = scene;
	}

	/**
	 * @brief Sets collision world.
	 * @param world Parameter for world.
	 */
	void SetCollisionWorld(collision::World* world);

	/**
	 * @brief Updates physics.
	 * @param deltaTime Frame delta time in seconds.
	 * @param entityManager Entity manager containing the active objects.
	 * @param inputManager Input manager for the current frame.
	 */
	void UpdatePhysics(float deltaTime,
		EntityManager& entityManager,
		InputManager& inputManager);

	/**
	 * @brief Enables physics.
	 * @param entityID Parameter for entity id.
	 * @param mass Parameter for mass.
	 */
	void EnablePhysics(int entityID, float mass = 1.0f);

	/**
	 * @brief Disables physics.
	 * @param entityID Parameter for entity id.
	 */
	void DisablePhysics(int entityID);

	/**
	 * @brief Returns whether physics.
	 * @param entityID Parameter for entity id.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasPhysics(int entityID) const;

	/**
	 * @brief Sets seek target.
	 * @param entityID Parameter for entity id.
	 * @param target Parameter for target.
	 */
	void SetSeekTarget(int entityID, const Math::Vector2D& target);

	/**
	 * @brief Clears seek target.
	 * @param entityID Parameter for entity id.
	 */
	void ClearSeekTarget(int entityID);

	/**
	 * @brief Clears this object.
	 */
	void Clear();

	/**
	 * @brief Returns step controller.
	 * @return Requested value.
	 */
	physics::StepController& GetStepController() {
		return physicsStep_;
	}

	/**
	 * @brief Sets movement manager.
	 * @param m Parameter for m.
	 */
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
	Scene* scene_ = nullptr;

	// Constants
	static constexpr float SEEK_MAX_ACCEL = 600.0f;
	static constexpr float ARRIVE_RADIUS = 10.0f;

	/**
	 * @brief Performs integrate entity.
	 * @param entityID Parameter for entity id.
	 * @param dt Frame delta time in seconds.
	 * @param entityManager Entity manager containing the active objects.
	 */
	void IntegrateEntity(int entityID, float dt, EntityManager& entityManager);
};
