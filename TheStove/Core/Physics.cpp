#include "Physics.hpp"
#include "../Core/InputManager.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace physics {

	collision::AABB MakeColliderBox(GameObject* obj, const glm::vec3& pos) {
		return collision::World::makeAABBFromCenter(
			pos + glm::vec3(obj->GetColliderOffset(), 0.0f),
			glm::vec3(obj->GetColliderSize(), 1.0f));
	}

	void ClampInsideWalk(const collision::WalkArea& w, GameObject* obj, glm::vec3& pos) {
		const glm::vec2 sz = obj->GetColliderSize();
		const glm::vec2 off = obj->GetColliderOffset();
		const glm::vec2 half = sz * 0.5f;
		pos.x = std::clamp(pos.x, w.L + half.x - off.x, w.R - half.x - off.x);
		pos.y = std::clamp(pos.y, w.T + half.y - off.y, w.B - half.y - off.y);
	}

	float StepController::resolveDt(::InputManager& input, float deltaTime) {
		const bool toggleNow = input.IsKeyPressed(GLFW_KEY_P);
		const bool wNow = input.IsKeyPressed(GLFW_KEY_W);
		const bool aNow = input.IsKeyPressed(GLFW_KEY_A);
		const bool sNow = input.IsKeyPressed(GLFW_KEY_S);
		const bool dNow = input.IsKeyPressed(GLFW_KEY_D);

		if (toggleNow && !prevToggle) {
			enabled = !enabled;
			std::cout << "[Physics] Step mode " << (enabled ? "ON" : "OFF") << "\n";
		}

		// queue a step on left click (edge)
		if (enabled && input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
			++stepsQueued;
		}

		if (enabled) {
			const bool anyDownEdge =
				(wNow && !prevW) || (aNow && !prevA) || (sNow && !prevS) || (dNow && !prevD);
			if (anyDownEdge) ++stepsQueued;
		}

		prevToggle = toggleNow;
		prevW = wNow; prevA = aNow; prevS = sNow; prevD = dNow;

		const float physicsDt =
			enabled ? (stepsQueued > 0 ? fixedDt : 0.0f) : deltaTime;

		if (enabled && physicsDt > 0.0f) --stepsQueued;
		return physicsDt;
	}

	void SeparatePlayerVsOther_StopPlayerOnly(
		collision::World& world,
		GameObject* player, GameObject* other,
		glm::vec3& pPos, glm::vec3& oPos,
		glm::vec2& desiredMove, bool& hasClickTarget,
		float weightPlayerSplit)
	{
		const collision::AABB a = MakeColliderBox(player, pPos);
		const collision::AABB b = MakeColliderBox(other, oPos);

		glm::vec2 pushA{}, pushB{};
		if (!collision::separateWeighted(a, b, weightPlayerSplit, pushA, pushB)) return;

		// Other: make correction world-safe; blocked part goes to player
		{
			const collision::AABB otherStart = MakeColliderBox(other, oPos);
			const glm::vec2 allowedB = world.resolve(otherStart, pushB);
			const glm::vec2 blockedB = pushB - allowedB;
			if (blockedB.x != 0.0f || blockedB.y != 0.0f) {
				pushA += blockedB;
				pushB = allowedB;
				desiredMove = { 0.0f, 0.0f };
				hasClickTarget = false;
			}
		}

		// Player: world-safe
		{
			const collision::AABB playerStart = MakeColliderBox(player, pPos);
			pushA = world.resolve(playerStart, pushA);
		}

		// Apply
		pPos += glm::vec3(pushA, 0.0f);
		oPos += glm::vec3(pushB, 0.0f);
		player->SetPosition(pPos);
		other->SetPosition(oPos);

		// tiny nudge if still overlapping
		glm::vec2 mtv{};
		if (collision::overlapMTV(MakeColliderBox(player, pPos),
			MakeColliderBox(other, oPos), mtv)) {
			pPos += glm::vec3(mtv * 1.001f, 0.0f);
			player->SetPosition(pPos);
		}
	}

	void MoveYLaneWithBounce(
		collision::World& world,
		GameObject* obj, glm::vec3& pos, glm::vec2& vel,
		float laneX, float physicsDt)
	{
		pos.x = laneX;

		const glm::vec2 desired = vel * physicsDt;
		const collision::AABB start = MakeColliderBox(obj, pos);
		const glm::vec2 allowed = world.resolve(start, desired);

		pos.y += allowed.y;
		obj->SetPosition(pos);

		if (physicsDt > 0.0f) {
			const float eps = 1e-4f;
			if (std::abs(allowed.y - desired.y) > eps) vel.y = -vel.y; // bounce Y
		}
	}

	void ElasticBounceEqualMass(
		GameObject* aObj, GameObject* bObj,
		glm::vec3& aPos, glm::vec3& bPos,
		glm::vec2& aVel, glm::vec2& bVel)
	{
		collision::AABB a = MakeColliderBox(aObj, aPos);
		collision::AABB b = MakeColliderBox(bObj, bPos);
		glm::vec2 mtv{};
		if (!collision::overlapMTV(a, b, mtv)) return;

		// split separation
		const glm::vec2 half = 0.5f * mtv;
		aPos += glm::vec3(+half, 0.0f);
		bPos += glm::vec3(-half, 0.0f);
		aObj->SetPosition(aPos);
		bObj->SetPosition(bPos);

		// robust normal
		glm::vec2 n;
		float len = glm::length(mtv);
		if (len > 1e-6f) n = mtv / len;
		else {
			glm::vec2 rel = { aPos.x - bPos.x, aPos.y - bPos.y };
			if (std::abs(rel.y) >= std::abs(rel.x)) n = { 0.0f, (rel.y >= 0 ? 1.0f : -1.0f) };
			else                                    n = { (rel.x >= 0 ? 1.0f : -1.0f), 0.0f };
		}

		// exchange normal components (equal mass)
		const float aN = glm::dot(aVel, n);
		const float bN = glm::dot(bVel, n);
		const glm::vec2 aT = aVel - aN * n;
		const glm::vec2 bT = bVel - bN * n;

		aVel = aT + bN * n;
		bVel = bT + aN * n;

		// safety
		if (!std::isfinite(aVel.x) || !std::isfinite(aVel.y)) aVel = { 0.f,0.f };
		if (!std::isfinite(bVel.x) || !std::isfinite(bVel.y)) bVel = { 0.f,0.f };
	}

}
