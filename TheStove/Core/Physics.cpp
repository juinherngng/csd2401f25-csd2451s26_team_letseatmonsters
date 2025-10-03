/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Physics.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Definitions for physics helpers: collider construction, clamping, step control,
					separation responses, lane motion with bounce, and equal-mass elastic collisions.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "Physics.hpp"
#include "../Core/InputManager.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace physics {
	collision::AABB MakeColliderBox(GameObject* gameObj, const glm::vec3& pos) {
		return collision::World::makeAABBFromCenter(
			pos + glm::vec3(gameObj->GetColliderOffset(), 0.0f),
			glm::vec3(gameObj->GetColliderSize(), 1.0f));
	}

	void ClampInsideWalk(const collision::WalkArea& walkArea, GameObject* gameObj, glm::vec3& pos) {
		const glm::vec2 size = gameObj->GetColliderSize();
		const glm::vec2 offset = gameObj->GetColliderOffset();
		const glm::vec2 half = size * 0.5f;

		pos.x = std::clamp(pos.x, walkArea.L + half.x - offset.x, walkArea.R - half.x - offset.x);
		pos.y = std::clamp(pos.y, walkArea.T + half.y - offset.y, walkArea.B - half.y - offset.y);
	}

	void ClampInsideWalkWithGate(
		const collision::WalkArea& walk,
		const collision::StageEndGateVertical& gate,
		GameObject* obj,
		glm::vec3& pos)
	{
		const glm::vec2 size = obj->GetColliderSize();
		const glm::vec2 offset = obj->GetColliderOffset();
		const glm::vec2 half = size * 0.5f;

		// Always clamp Y to the walk area
		pos.y = std::clamp(pos.y, walk.T + half.y - offset.y, walk.B - half.y - offset.y);

		// Default right clamp = walk.R (normal wall)
		float maxX = walk.R - half.x - offset.x;

		// Player AABB vertical span
		const float aabbMinY = pos.y - half.y + offset.y;
		const float aabbMaxY = pos.y + half.y + offset.y;

		// If fully inside the gate’s vertical gap, extend clamp to the far side (gate.x1)
		if (aabbMinY >= gate.gapMinY && aabbMaxY <= gate.gapMaxY) {
			maxX = gate.x1 - half.x - offset.x;
		}

		// Left clamp unchanged
		const float minX = walk.L + half.x - offset.x;
		pos.x = std::clamp(pos.x, minX, maxX);
	}

	float StepController::resolveDt(::InputManager& input, float deltaTime) {
		const bool pNow = input.IsKeyPressed(GLFW_KEY_P);
		const bool wNow = input.IsKeyPressed(GLFW_KEY_W);
		const bool aNow = input.IsKeyPressed(GLFW_KEY_A);
		const bool sNow = input.IsKeyPressed(GLFW_KEY_S);
		const bool dNow = input.IsKeyPressed(GLFW_KEY_D);

		// Toggle step mode on P edge.
		if (pNow && !prevToggle) {
			enabled = !enabled;
			std::cout << "[Physics] Step mode " << (enabled ? "ON" : "OFF") << "\n";
		}

		// Queue a step on left-click edge while in step mode.
		if (enabled && input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
			++stepsQueued;
		}

		// Queue one step on any WASD edge while in step mode.
		if (enabled) {
			const bool anyEdge = (wNow && !prevW) || (aNow && !prevA) || (sNow && !prevS) || (dNow && !prevD);
			if (anyEdge) {
				++stepsQueued;
			}
		}

		// Update edge trackers.
		prevToggle = pNow;
		prevW = wNow; prevA = aNow; prevS = sNow; prevD = dNow;

		// Determine physics dt:
		// - Step mode: run fixed step only when a step is queued; else 0.
		// - Real-time: pass-through.
		const float physicsDt = enabled ? (stepsQueued > 0 ? fixedDt : 0.0f) : deltaTime;

		if (enabled && physicsDt > 0.0f) {
			--stepsQueued;
		}

		return physicsDt;
	}

	void SeparatePlayerVsOther_StopPlayerOnly(
		collision::World& world,
		GameObject* player, GameObject* other,
		glm::vec3& playerPos, glm::vec3& otherPos,
		glm::vec2& desiredMove, bool& hasClickTarget,
		float splitPlayer)
	{
		const collision::AABB pBox = MakeColliderBox(player, playerPos);
		const collision::AABB oBox = MakeColliderBox(other, otherPos);

		glm::vec2 playerCorr{}, otherCorr{};
		if (!collision::separateWeighted(pBox, oBox, splitPlayer, playerCorr, otherCorr)) return;

		// Make other correction world-safe; blocked part transfers to player.
		{
			const collision::AABB start = MakeColliderBox(other, otherPos);
			const glm::vec2 allowedDelta = world.resolve(start, otherCorr);
			const glm::vec2 blockedDelta = otherCorr - allowedDelta;

			if (blockedDelta.x != 0.0f || blockedDelta.y != 0.0f) {
				playerCorr += blockedDelta;   // player absorbs the blocked portion
				otherCorr = allowedDelta;	  // only apply what's allowed to "other"
				desiredMove = { 0.0f, 0.0f }; // cancel player move target
				hasClickTarget = false;
			}
		}

		// Make player correction world-safe as well.
		{
			const collision::AABB start = MakeColliderBox(player, playerPos);
			playerCorr = world.resolve(start, playerCorr);
		}

		// Apply corrections.
		playerPos += glm::vec3(playerCorr, 0.0f);
		otherPos += glm::vec3(otherCorr, 0.0f);
		player->SetPosition(playerPos);
		other->SetPosition(otherPos);

		// Tiny nudge if still overlapping (robustness for coincident edges).
		glm::vec2 mtv{};
		if (collision::overlapMTV(MakeColliderBox(player, playerPos),
			MakeColliderBox(other, otherPos), mtv)) {
			playerPos += glm::vec3(mtv * 1.001f, 0.0f);
			player->SetPosition(playerPos);
		}
	}

	void MoveYLaneWithBounce(
		collision::World& world,
		GameObject* gameObj, glm::vec3& pos, glm::vec2& vel,
		float laneX, float physicsDt)
	{
		// Constrain to lane (fixed X).
		pos.x = laneX;

		// Move along Y and resolve against static world.
		const glm::vec2 desiredDelta = vel * physicsDt;
		const collision::AABB start = MakeColliderBox(gameObj, pos);
		const glm::vec2 allowedDelta = world.resolve(start, desiredDelta);

		pos.y += allowedDelta.y;
		gameObj->SetPosition(pos);

		// On impact, flip Y velocity (simple elastic bounce).
		if (physicsDt > 0.0f) {
			const float eps = 1e-4f;
			if (std::abs(allowedDelta.y - desiredDelta.y) > eps) vel.y = -vel.y;
		}
	}

	void ElasticBounceEqualMass(
		GameObject* firstObj, GameObject* secondObj,
		glm::vec3& firstPos, glm::vec3& secondPos,
		glm::vec2& firstVel, glm::vec2& secondVel)
	{
		collision::AABB firstBox = MakeColliderBox(firstObj, firstPos);
		collision::AABB secondBox = MakeColliderBox(secondObj, secondPos);

		glm::vec2 mtv{};
		if (!collision::overlapMTV(firstBox, secondBox, mtv)) return;

		// Separate equally along MTV.
		const glm::vec2 halfCorr = 0.5f * mtv;
		firstPos += glm::vec3(+halfCorr, 0.0f);
		secondPos += glm::vec3(-halfCorr, 0.0f);
		firstObj->SetPosition(firstPos);
		secondObj->SetPosition(secondPos);

		// Collision normal.
		glm::vec2 colNormal;
		float mtvLen = glm::length(mtv);
		if (mtvLen > 1e-6f) {
			colNormal = mtv / mtvLen;
		}
		else {
			glm::vec2 rel = { firstPos.x - secondPos.x, firstPos.y - secondPos.y };
			if (std::abs(rel.y) >= std::abs(rel.x)) {
				colNormal = { 0.0f, (rel.y >= 0 ? 1.0f : -1.0f) };
			}
			else {
				colNormal = { (rel.x >= 0 ? 1.0f : -1.0f), 0.0f };
			}
		}

		// Swap normal components; keep tangential (equal masses).
		const float firstN = glm::dot(firstVel, colNormal);
		const float secondN = glm::dot(secondVel, colNormal);
		const glm::vec2 firstT = firstVel - firstN * colNormal;
		const glm::vec2 secondT = secondVel - secondN * colNormal;

		firstVel = firstT + secondN * colNormal;
		secondVel = secondT + firstN * colNormal;

		// Safety.
		if (!std::isfinite(firstVel.x) || !std::isfinite(firstVel.y))  firstVel = { 0.f, 0.f };
		if (!std::isfinite(secondVel.x) || !std::isfinite(secondVel.y)) secondVel = { 0.f, 0.f };
	}
}
