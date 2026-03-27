/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Physics.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Helpers for collider construction, world clamping, step control,
					separation responses, lane motion with bounce, and equal-mass elastic collisions.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "EngineCore/Collision.hpp"
#include "EngineCore/InputManager.hpp"
#include "EngineCore/Math.hpp"
#include "EngineGraphics/GameObject.hpp"

namespace physics {

	/**
	 * @brief Performs make collider box.
	 * @param gameObj Parameter for game obj.
	 * @param pos Parameter for pos.
	 * @return Result produced by this operation.
	 */
	collision::AABB MakeColliderBox(GameObject* gameObj, const Math::Vector3D& pos);

	/**
	 * @brief Performs clamp inside walk.
	 * @param walkArea Parameter for walk area.
	 * @param gameObj Parameter for game obj.
	 * @param pos Parameter for pos.
	 */
	void ClampInsideWalk(const collision::WalkArea& walkArea, GameObject* gameObj, Math::Vector3D& pos);

	/**
	 * @brief Performs clamp inside walk with gate.
	 * @param walk Parameter for walk.
	 * @param gate Parameter for gate.
	 * @param obj Parameter for obj.
	 * @param pos Parameter for pos.
	 */
	void ClampInsideWalkWithGate(
		const collision::WalkArea& walk,
		const collision::StageEndGateVertical& gate,
		GameObject* obj,
		Math::Vector3D& pos);

	// Step Controller (fixed-step simulation with manual stepping)
	struct StepController {
		bool enabled = false;		  // step mode on/off
		int stepsQueued = 0;		  // steps to release
		float fixedDt = 1.0f / 60.0f; // fixed step size (s)

		// runtime accumulation (real-time mode)
		double maxCarry = (1.0 / 60.0) * 4.0; // clamp long frames
		double runtimeAccum = 0.0;

		// input edges (for toggles)
		bool prevToggle = false;
		bool prevW = false, prevA = false, prevS = false, prevD = false;

		/**
		 * @brief Resolves dt.
		 * @param input Input manager for the current frame.
		 * @param deltaTime Frame delta time in seconds.
		 * @return Result produced by this operation.
		 */
		float resolveDt(::InputManager& input, float deltaTime);
	};

	/**
	 * @brief Performs separate player vs other stop player only.
	 * @param world Parameter for world.
	 * @param player Parameter for player.
	 * @param other Parameter for other.
	 * @param playerPos Parameter for player pos.
	 * @param otherPos Parameter for other pos.
	 * @param desiredMove Parameter for desired move.
	 * @param hasClickTarget Parameter for has click target.
	 * @param splitPlayer Parameter for split player.
	 */
	void SeparatePlayerVsOther_StopPlayerOnly(
		collision::World& world,
		GameObject* player, GameObject* other,
		Math::Vector3D& playerPos, Math::Vector3D& otherPos,
		Math::Vector2D& desiredMove, bool& hasClickTarget,
		float splitPlayer);

	/**
	 * @brief Moves ylane with bounce.
	 * @param world Parameter for world.
	 * @param gameObj Parameter for game obj.
	 * @param pos Parameter for pos.
	 * @param vel Parameter for vel.
	 * @param laneX Parameter for lane x.
	 * @param physicsDt Parameter for physics dt.
	 */
	void MoveYLaneWithBounce(
		collision::World& world,
		GameObject* gameObj, Math::Vector3D& pos, Math::Vector2D& vel,
		float laneX, float physicsDt);

	/**
	 * @brief Performs elastic bounce equal mass.
	 * @param firstObj Parameter for first obj.
	 * @param secondObj Parameter for second obj.
	 * @param firstPos Parameter for first pos.
	 * @param secondPos Parameter for second pos.
	 * @param firstVel Parameter for first vel.
	 * @param secondVel Parameter for second vel.
	 */
	void ElasticBounceEqualMass(
		GameObject* firstObj, GameObject* secondObj,
		Math::Vector3D& firstPos, Math::Vector3D& secondPos,
		Math::Vector2D& firstVel, Math::Vector2D& secondVel);
}
