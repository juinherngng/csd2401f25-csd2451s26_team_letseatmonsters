/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Physics.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Physics helpers for AABB construction, movement clamping, step-mode control,
					simple collision response, and elastic bounces.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "../Core/Collision.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Core/Math.hpp"

 // Forward declarations to avoid heavy includes in headers.
class InputManager;

namespace physics {

	/**
	 * @brief Build an AABB collider for a given object and position.
	 *
	 * Uses the GameObject's collider offset and collider size to construct an
	 * AABB centered at (pos + offset), with full width/height from GetColliderSize().
	 *
	 * @param obj Pointer to the game object providing collider info.
	 * @param pos World position used as the object center before offset.
	 * @return collision::AABB The constructed axis-aligned bounding box.
	 */
	collision::AABB MakeColliderBox(GameObject* gameObj, const Math::Vector3D& pos);

	/**
	 * @brief Clamp an object's position so its collider stays inside a walkable area.
	 *
	 * Adjusts @p pos in-place such that the object’s collider (size + offset) remains within
	 * the WalkArea bounds.
	 *
	 * @param w   Walkable area definition (L/R/T/B plus edge thickness).
	 * @param obj Game object providing collider size/offset.
	 * @param pos [in,out] World position to be clamped.
	 */
	void ClampInsideWalk(const collision::WalkArea& walkArea, GameObject* gameObj, Math::Vector3D& pos);

	/**
	 * @brief Clamp an object's position inside the walkable area, with special handling for an end gate.
	 *
	 * Ensures the object’s collider (size + offset) stays within the WalkArea bounds, while allowing
	 * horizontal extension past the walk area's right edge if the object is fully within the vertical
	 * gap of the StageEndGateVertical.
	 *
	 * @param walk Walkable area definition (L/R/T/B plus edge thickness).
	 * @param gate End-of-stage vertical gate definition (solid top/bottom, central gap).
	 * @param obj  Game object providing collider size/offset.
	 * @param pos  [in,out] World position to be clamped and possibly extended at the gate.
	 */
	void ClampInsideWalkWithGate(
		const collision::WalkArea& walk,
		const collision::StageEndGateVertical& gate,
		GameObject* obj,
		Math::Vector3D& pos);

	/**
	 * @brief A tiny controller to toggle between real-time deltaTime and discrete fixed steps.
	 *
	 * - Press **P** to toggle step mode.
	 * - In step mode, a single left-click queues one physics step.
	 * - In step mode, pressing any of **W/A/S/D** (edge) also queues a step.
	 *
	 * Useful for deterministic debugging of physics/collision.
	 */
	struct StepController {
		bool enabled = false;
		float fixedDt = 1.0f / 60.0f;

		// Internal key-edge tracking
		int stepsQueued = 0;
		bool prevToggle = false;
		bool prevW = false, prevA = false, prevS = false, prevD = false;

		/**
		 * @brief Decide which dt to use this frame based on input and mode.
		 *
		 * - When disabled, returns the given @p deltaTime unchanged.
		 * - When enabled and a step is queued (via click or WASD edge), returns @c fixedDt
		 *   and consumes one queued step; otherwise returns 0.0f (no simulation).
		 *
		 * @param input     Input manager (reads P, WASD, and left mouse edges).
		 * @param deltaTime Real-time frame delta.
		 * @return float The physics delta to use this frame (fixedDt, deltaTime, or 0).
		 */
		float resolveDt(::InputManager& input, float deltaTime);
	};

	/**
	 * @brief Separate a player from another dynamic object, stopping the player if necessary.
	 *
	 * Computes an MTV between player and other, splits it by @p weightPlayerSplit, and then
	 * filters both corrections through the world (so blocked portions are reassigned).
	 * If the other’s correction is blocked by the world, the player absorbs the blocked part,
	 * the player's pending click/command is canceled, and @p desiredMove is zeroed.
	 *
	 * @param world            Collision world used to resolve against static walls.
	 * @param player           Player object.
	 * @param other            Other dynamic object.
	 * @param pPos             [in,out] Player position to correct.
	 * @param oPos             [in,out] Other object position to correct.
	 * @param desiredMove      [in,out] Player's desired movement vector (may be cleared).
	 * @param hasClickTarget   [in,out] Whether the player has an active click target (may be cleared).
	 * @param weightPlayerSplit MTV split factor for the player (0..1), e.g., 0.5 = equal.
	 */
	void SeparatePlayerVsOther_StopPlayerOnly(
		collision::World& world,
		GameObject* player, GameObject* other,
		Math::Vector3D& playerPos, Math::Vector3D& otherPos,
		Math::Vector2D& desiredMove, bool& hasClickTarget,
		float splitPlayer);

	/**
	 * @brief Constrain an object to a vertical “lane” (fixed X) and bounce in Y on wall impact.
	 *
	 * Applies velocity * physicsDt along Y, resolves against the world, moves the object,
	 * and flips the Y velocity when a collision is detected (simple elastic bounce on Y).
	 *
	 * @param world     Collision world for static walls.
	 * @param obj       The object to move.
	 * @param pos       [in,out] Current object position.
	 * @param vel       [in,out] Current object velocity (Y may flip on bounce).
	 * @param laneX     The fixed X coordinate of the lane.
	 * @param physicsDt Physics delta time to advance.
	 */
	void MoveYLaneWithBounce(
		collision::World& world,
		GameObject* gameObj, Math::Vector3D& pos, Math::Vector2D& vel,
		float laneX, float physicsDt);

	/**
	 * @brief Equal-mass elastic collision response for two objects.
	 *
	 * Separates overlapping colliders equally along the MTV and swaps the normal
	 * components of their velocities, preserving tangential components (perfectly elastic,
	 * equal mass). Includes guards for degenerate/near-zero MTV cases.
	 *
	 * @param aObj  First object.
	 * @param bObj  Second object.
	 * @param aPos  [in,out] First object's position (may be nudged).
	 * @param bPos  [in,out] Second object's position (may be nudged).
	 * @param aVel  [in,out] First object's velocity.
	 * @param bVel  [in,out] Second object's velocity.
	 */
	void ElasticBounceEqualMass(
		GameObject* firstObj, GameObject* secondObj,
		Math::Vector3D& firstPos, Math::Vector3D& secondPos,
		Math::Vector2D& firstVel, Math::Vector2D& secondVel);
}
