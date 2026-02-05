/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Physics.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Helpers for collider construction, world clamping, step control,
					separation responses, lane motion with bounce, and equal-mass elastic collisions.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "../Graphics/GameObject.hpp"

#include "Collision.hpp"
#include "InputManager.hpp"
#include "Math.hpp"

namespace physics {
	// Colliders / Clamp
	collision::AABB MakeColliderBox(GameObject* gameObj, const Math::Vector3D& pos);
	void ClampInsideWalk(const collision::WalkArea& walkArea, GameObject* gameObj, Math::Vector3D& pos);
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

		// returns the physics dt to simulate this frame (0 if none)
		float resolveDt(::InputManager& input, float deltaTime);
	};

	// Separation / Movement helpers
	void SeparatePlayerVsOther_StopPlayerOnly(
		collision::World& world,
		GameObject* player, GameObject* other,
		Math::Vector3D& playerPos, Math::Vector3D& otherPos,
		Math::Vector2D& desiredMove, bool& hasClickTarget,
		float splitPlayer);

	void MoveYLaneWithBounce(
		collision::World& world,
		GameObject* gameObj, Math::Vector3D& pos, Math::Vector2D& vel,
		float laneX, float physicsDt);

	void ElasticBounceEqualMass(
		GameObject* firstObj, GameObject* secondObj,
		Math::Vector3D& firstPos, Math::Vector3D& secondPos,
		Math::Vector2D& firstVel, Math::Vector2D& secondVel);
}
