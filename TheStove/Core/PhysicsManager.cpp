/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         PhysicsManager.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu	(45%)
 CO-AUTHORS:        Yat Chun Wee, y.chunwee@digipen.edu		(50%)
					Ng Juin Herng, juinherng.ng@digipen.edu (5%)

 DESCRIPTION:       Implements PhysicsManager. Integrates simple force-based movement
					(seek/arrive + drag), applies damping, and trims movement against
					world collisions. On impact, the active seek is cleared and the
					entity is stopped. Also clears MovementManager's click-to-move
					target so the UI/green path reflects the stop.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Graphics/SceneManager.hpp"

#include "Logger.hpp"
#include "PhysicsManager.hpp"

#include <cmath>

 // SystemInterface implementation

/**
 * @brief Initializes this object.
 * @return Result produced by this operation.
 */
void PhysicsManager::Initialize() {
	TS_LOG_INFO("PhysicsManager system initialized.");
}

/**
 * @brief Updates this object.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void PhysicsManager::Update(float dt) {
	(void)dt;
}

/**
 * @brief Returns the stable name for this object.
 * @return Requested value.
 */
std::string PhysicsManager::GetName() {
	return "PhysicsManager";
}

/**
 * @brief Sets entity manager.
 * @param entityMgr Parameter for entity mgr.
 * @return Result produced by this operation.
 */
void PhysicsManager::SetEntityManager(EntityManager* entityMgr) {
	entityManager_ = entityMgr;
}

/**
 * @brief Sets input manager.
 * @param inputMgr Parameter for input mgr.
 * @return Result produced by this operation.
 */
void PhysicsManager::SetInputManager(InputManager* inputMgr) {
	inputManager_ = inputMgr;
}

/**
 * @brief Sets collision world.
 * @param world Parameter for world.
 * @return Result produced by this operation.
 */
void PhysicsManager::SetCollisionWorld(collision::World* world) {
	world_ = world;
}

/**
 * @brief Updates physics.
 * @param physicsDt Parameter for physics dt.
 * @param entityManager Entity manager containing the active objects.
 * @param inputManager Input manager for the current frame.
 * @return Result produced by this operation.
 */
void PhysicsManager::UpdatePhysics(float physicsDt,
	EntityManager& entityManager,
	InputManager& inputManager) {
	(void)inputManager; // currently unused; kept for signature compatibility

	// Scene already decided if physics runs this frame via physicsDt.
	if (physicsDt <= 0.0f) {
		return;
	}

	// Update all entities with physics enabled.
	for (auto& [entityID, state] : physicsStates_) {
		if (scene_ && !scene_->IsObjectLayerEnabled(entityID)) {
			continue;
		}
		IntegrateEntity(entityID, physicsDt, entityManager);
	}
}

/**
 * @brief Enables physics.
 * @param entityID Parameter for entity id.
 * @param mass Parameter for mass.
 * @return Result produced by this operation.
 */
void PhysicsManager::EnablePhysics(int entityID, float mass) {
	PhysicsState state;
	state.invMass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
	state.damping = 0.98f;
	state.velocity = { 0.0f, 0.0f };
	state.forceAccum = { 0.0f, 0.0f };

	physicsStates_[entityID] = state;
}

/**
 * @brief Disables physics.
 * @param entityID Parameter for entity id.
 * @return Result produced by this operation.
 */
void PhysicsManager::DisablePhysics(int entityID) {
	physicsStates_.erase(entityID);
	seekTargets_.erase(entityID);
}

/**
 * @brief Returns whether physics.
 * @param entityID Parameter for entity id.
 * @return True when the operation succeeds or the condition is met.
 */
bool PhysicsManager::HasPhysics(int entityID) const {
	return physicsStates_.find(entityID) != physicsStates_.end();
}

/**
 * @brief Sets seek target.
 * @param entityID Parameter for entity id.
 * @param target Parameter for target.
 * @return Result produced by this operation.
 */
void PhysicsManager::SetSeekTarget(int entityID, const Math::Vector2D& target) {
	if (!HasPhysics(entityID)) {
		return;
	}

	seekTargets_[entityID] = target;
}

/**
 * @brief Clears seek target.
 * @param entityID Parameter for entity id.
 * @return Result produced by this operation.
 */
void PhysicsManager::ClearSeekTarget(int entityID) {
	seekTargets_.erase(entityID);

	// Stop the entity
	auto it = physicsStates_.find(entityID);
	if (it != physicsStates_.end()) {
		it->second.velocity = { 0.0f, 0.0f };
	}
}

/**
 * @brief Performs integrate entity.
 * @param entityID Parameter for entity id.
 * @param dt Frame delta time in seconds.
 * @param entityManager Entity manager containing the active objects.
 * @return Result produced by this operation.
 */
void PhysicsManager::IntegrateEntity(int entityID, float dt, EntityManager& entityManager) {
	GameObject* obj = entityManager.GetByID(entityID);
	if (!obj) {
		return;
	}

	auto& state = physicsStates_[entityID];

	// Clear forces from last frame
	state.forceAccum = { 0.0f, 0.0f };

	// Apply seek force if target is set
	auto seekIt = seekTargets_.find(entityID);
	if (seekIt != seekTargets_.end()) {
		Math::Vector3D pos3D = obj->GetPosition();
		Math::Vector2D currentPos(pos3D.x, pos3D.y);
		Math::Vector2D toTarget = seekIt->second - currentPos;
		float distance = toTarget.Length();

		// Arrive behavior - stop within radius
		if (distance <= ARRIVE_RADIUS) {
			state.velocity = { 0.0f, 0.0f };
			seekTargets_.erase(seekIt);
		}
		else if (distance > 1e-4f) {
			// Apply capped acceleration in the target direction
			Math::Vector2D direction = toTarget * (1.0f / distance);
			float mass = (state.invMass > 0.0f) ? (1.0f / state.invMass) : 1.0f;
			state.forceAccum = state.forceAccum + (direction * (SEEK_MAX_ACCEL * mass));
		}
	}

	// Drag force proportional to speed
	const float speed = state.velocity.Length();
	if (speed > 1e-6f) {
		const float dragMag = (dragForce_.k1 * speed) + (dragForce_.k2 * speed * speed);
		Math::Vector2D dragDir = state.velocity * (-1.0f / speed);
		state.forceAccum = state.forceAccum + (dragDir * dragMag);
	}

	// Integrate forces
	if (state.invMass > 0.0f) {
		// a = F * invMass
		Math::Vector2D accel = state.forceAccum * state.invMass;

		// v += a * dt
		state.velocity = state.velocity + (accel * dt);

		// Apply damping
		if (state.damping > 0.0f && state.damping < 1.0f) {
			float dampFactor = std::pow(state.damping, dt);
			state.velocity = state.velocity * dampFactor;
		}

		// Desired movement this frame
		Math::Vector3D pos = obj->GetPosition();
		const Math::Vector2D desiredDelta(state.velocity.x * dt, state.velocity.y * dt);

		// Trim against world, if available
		Math::Vector2D allowedDelta = desiredDelta;

		if (world_ != nullptr) {
			// Build current AABB using your helper (namespace per your codebase).
			const collision::AABB start = physics::MakeColliderBox(obj, pos);

			// Ask world how much of desiredDelta we can move.
			allowedDelta = world_->resolve(start, desiredDelta);

			// If trimmed, we hit a wall: clear seek, clear click-to-move, and stop.
			const auto impacted = [](const Math::Vector2D& d, const Math::Vector2D& a) {
				constexpr float EPS = 1e-4f;
				return (std::fabs(d.x - a.x) > EPS) || (std::fabs(d.y - a.y) > EPS);
				};

			if (impacted(desiredDelta, allowedDelta)) {
				// Clear physics seek target
				if (auto it = seekTargets_.find(entityID); it != seekTargets_.end()) {
					seekTargets_.erase(it);
				}

				// Clear click-to-move target so UI path/green line stops too
				if (movement_ != nullptr) {
					movement_->ClearMoveTarget(entityID);
				}

				// Hard stop
				state.velocity = { 0.0f, 0.0f };
			}
		}

		// Apply allowed movement
		pos.x += allowedDelta.x;
		pos.y += allowedDelta.y;
		obj->SetPosition(pos);
	}
}

/**
 * @brief Clears this object.
 * @return Result produced by this operation.
 */
void PhysicsManager::Clear() {
	physicsStates_.clear();
	seekTargets_.clear();
}

