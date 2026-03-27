/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Physics.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Definitions for physics helpers:
						- Collider construction
						- World clamping (with and without gate)
						- Fixed-timestep / step-mode controller
						- Separation responses (player vs other)
						- Lane motion with bounce
						- Equal-mass elastic collisions

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "Logger.hpp"
#include "Physics.hpp"

#include <algorithm>
#include <cmath>

namespace {

	/**
	 * @brief Performs safe normalize.
	 * @param v Parameter for v.
	 * @return Result produced by this operation.
	 */
	inline Math::Vector2D SafeNormalize(const Math::Vector2D& v) {
		const float len = v.Length();
		if (len > 1e-6f) {
			return Math::Vector2D(v.x / len, v.y / len);
		}

		return Math::Vector2D(0.f, 0.f);
	}
}

namespace physics {

	/**
	 * @brief Performs make collider box.
	 * @param gameObj Parameter for game obj.
	 * @param pos Parameter for pos.
	 * @return Result produced by this operation.
	 */
	collision::AABB MakeColliderBox(GameObject* gameObj, const Math::Vector3D& pos) {
		const Math::Vector2D offset = gameObj->GetColliderOffset();
		const Math::Vector2D size = gameObj->GetColliderSize();
		const Math::Vector3D center(pos.x + offset.x, pos.y + offset.y, pos.z);
		const Math::Vector3D scale(size.x, size.y, 1.0f);
		return collision::World::makeAABBFromCenter(center, scale);
	}

	/**
	 * @brief Performs clamp inside walk.
	 * @param walkArea Parameter for walk area.
	 * @param gameObj Parameter for game obj.
	 * @param pos Parameter for pos.
	 */
	void ClampInsideWalk(const collision::WalkArea& walkArea, GameObject* gameObj, Math::Vector3D& pos) {
		const Math::Vector2D size = gameObj->GetColliderSize();
		const Math::Vector2D offset = gameObj->GetColliderOffset();
		const Math::Vector2D half = size * 0.5f;

		pos.x = std::clamp(pos.x, walkArea.L + half.x - offset.x, walkArea.R - half.x - offset.x);
		pos.y = std::clamp(pos.y, walkArea.T + half.y - offset.y, walkArea.B - half.y - offset.y);
	}

	/**
	 * @brief Performs clamp inside walk with gate.
	 * @param walk Parameter for walk.
	 * @param gate Parameter for gate.
	 * @param obj Parameter for obj.
	 * @param pos Parameter for pos.
	 */
	void ClampInsideWalkWithGate(const collision::WalkArea& walk,
		const collision::StageEndGateVertical& gate,
		GameObject* obj, Math::Vector3D& pos) {
		const Math::Vector2D half = obj->GetColliderSize() * 0.5f;
		const Math::Vector2D off = obj->GetColliderOffset();

		pos.y = std::clamp(pos.y, walk.T + half.y - off.y, walk.B - half.y - off.y);

		float maxX = walk.R - half.x - off.x;
		const float centerY = pos.y + off.y;
		const float eps = 1.0f;
		if (centerY >= gate.gapMinY - eps && centerY <= gate.gapMaxY + eps) {
			maxX = gate.x1 - half.x - off.x;
		}

		const float minX = walk.L - off.x + half.x;
		pos.x = std::clamp(pos.x, minX, maxX);
	}

	/**
	 * @brief Resolves dt.
	 * @param input Input manager for the current frame.
	 * @param deltaTime Frame delta time in seconds.
	 * @return Result produced by this operation.
	 */
	float StepController::resolveDt(::InputManager& input, float deltaTime) {
		// Toggle step mode with P (edge)
		if (input.IsKeyJustPressed(GLFW_KEY_P)) {
			enabled = !enabled;
			TS_LOG_INFO("[Physics] Step mode " << (enabled ? "ON" : "OFF"));
		}

		// While in step mode, queue steps on input edges
		if (enabled) {
			// Left click in step mode = one physics step
			if (input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
				++stepsQueued;
			}

			// Any WASD key *edge* in step mode = one physics step
			const bool anyEdge =
				input.IsKeyJustPressed(GLFW_KEY_W) ||
				input.IsKeyJustPressed(GLFW_KEY_A) ||
				input.IsKeyJustPressed(GLFW_KEY_S) ||
				input.IsKeyJustPressed(GLFW_KEY_D);

			if (anyEdge) {
				++stepsQueued;
			}
		}

		// Decide physics dt
		if (enabled) {
			// Step mode: fixed slice only when a step is queued
			if (stepsQueued > 0) {
				--stepsQueued;
				return fixedDt;
			}
			return 0.0f;
		}
		else {
			// Real-time mode: fixed timestep at 60 Hz using accumulator
			deltaTime = std::min(deltaTime, static_cast<float>(maxCarry));
			runtimeAccum += deltaTime;

			if (runtimeAccum >= fixedDt) {
				runtimeAccum -= fixedDt;
				return fixedDt;
			}
			return 0.0f;
		}
	}

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
		float splitPlayer) {
		const collision::AABB pBox = MakeColliderBox(player, playerPos);
		const collision::AABB oBox = MakeColliderBox(other, otherPos);

		Math::Vector2D playerCorr{}, otherCorr{};
		if (!collision::separateWeighted(pBox, oBox, splitPlayer, playerCorr, otherCorr)) return;

		// Make other correction world-safe; blocked part transfers to player.
		{
			const collision::AABB start = MakeColliderBox(other, otherPos);
			const Math::Vector2D allowedDelta = world.resolve(start, otherCorr);
			const Math::Vector2D blockedDelta = otherCorr - allowedDelta;

			if (blockedDelta.x != 0.0f || blockedDelta.y != 0.0f) {
				// player absorbs the blocked portion
				playerCorr = Math::Vector2D(playerCorr.x + blockedDelta.x, playerCorr.y + blockedDelta.y);
				otherCorr = allowedDelta;				// only apply what's allowed to "other"
				desiredMove = Math::Vector2D(0.f, 0.f); // cancel player move target
				hasClickTarget = false;
			}
		}

		// Make player correction world-safe as well.
		{
			const collision::AABB start = MakeColliderBox(player, playerPos);
			const Math::Vector2D safeDelta = world.resolve(start, playerCorr);
			playerCorr = safeDelta;
		}

		// Apply corrections.
		playerPos.x += playerCorr.x;
		playerPos.y += playerCorr.y;

		otherPos.x += otherCorr.x;
		otherPos.y += otherCorr.y;

		player->SetPosition(playerPos);
		other->SetPosition(otherPos);

		// Tiny nudge if still overlapping (robustness for coincident edges).
		Math::Vector2D mtv{};
		if (collision::overlapMTV(MakeColliderBox(player, playerPos),
			MakeColliderBox(other, otherPos), mtv)) {
			playerPos.x += mtv.x * 1.001f;
			playerPos.y += mtv.y * 1.001f;
			player->SetPosition(playerPos);
		}
	}

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
		float laneX, float physicsDt) {
		// Constrain to lane (fixed X).
		pos.x = laneX;

		// Move along Y and resolve against static world.
		const Math::Vector2D desiredDelta = vel * physicsDt;
		const collision::AABB start = MakeColliderBox(gameObj, pos);
		const Math::Vector2D allowedDelta = world.resolve(start, desiredDelta);

		pos.y += allowedDelta.y;
		gameObj->SetPosition(pos);

		// On impact, flip Y velocity (simple elastic bounce).
		if (physicsDt > 0.0f) {
			const float eps = 1e-4f;
			if (std::abs(allowedDelta.y - desiredDelta.y) > eps) vel.y = -vel.y;
		}
	}

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
		Math::Vector2D& firstVel, Math::Vector2D& secondVel) {
		collision::AABB firstBox = MakeColliderBox(firstObj, firstPos);
		collision::AABB secondBox = MakeColliderBox(secondObj, secondPos);

		Math::Vector2D mtv{};
		if (!collision::overlapMTV(firstBox, secondBox, mtv)) return;

		// Separate equally along MTV.
		const Math::Vector2D halfCorr(mtv.x * 0.5f, mtv.y * 0.5f);
		firstPos.x += halfCorr.x;
		firstPos.y += halfCorr.y;

		secondPos.x -= halfCorr.x;
		secondPos.y -= halfCorr.y;

		firstObj->SetPosition(firstPos);
		secondObj->SetPosition(secondPos);

		// Collision normal.
		Math::Vector2D colNormal = SafeNormalize(mtv);
		if (colNormal.Length() < 1e-6f) {
			Math::Vector2D rel(firstPos.x - secondPos.x,
				firstPos.y - secondPos.y);
			colNormal = SafeNormalize(rel);
			if (colNormal.Length() < 1e-6f) colNormal = Math::Vector2D(1.f, 0.f);
		}

		// Swap normal components; keep tangential (equal masses).
		const Math::Vector2D v1 = firstVel;
		const Math::Vector2D v2 = secondVel;

		const float v1n = v1.Dot(colNormal);
		const float v2n = v2.Dot(colNormal);

		const Math::Vector2D v1t(v1.x - v1n * colNormal.x, v1.y - v1n * colNormal.y);
		const Math::Vector2D v2t(v2.x - v2n * colNormal.x, v2.y - v2n * colNormal.y);

		const Math::Vector2D v1_out(v1t.x + v2n * colNormal.x, v1t.y + v2n * colNormal.y);
		const Math::Vector2D v2_out(v2t.x + v1n * colNormal.x, v2t.y + v1n * colNormal.y);

		firstVel = v1_out;
		secondVel = v2_out;

		firstObj->SetVelocity(firstVel);
		secondObj->SetVelocity(secondVel);

		// Safety.
		if (!std::isfinite(firstVel.x) || !std::isfinite(firstVel.y))  firstVel = { 0.f, 0.f };
		if (!std::isfinite(secondVel.x) || !std::isfinite(secondVel.y)) secondVel = { 0.f, 0.f };
	}
}

