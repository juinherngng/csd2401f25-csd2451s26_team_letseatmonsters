#pragma once

#include "../Graphics/Collision.h"
#include "./Graphics/GameObject.h"
#include <glm/glm.hpp>

// Forward declare the real class in the global namespace
class InputManager;

namespace physics {

	struct StepController {
		bool  enabled = false;
		int   stepsQueued = 0;
		float fixedDt = 1.0f / 60.0f;

		bool prevToggle = false;
		bool prevW = false, prevA = false, prevS = false, prevD = false;

		// Note the leading :: to bind to the global InputManager
		float resolveDt(::InputManager& input, float deltaTime);
		inline void QueueOneStep() { ++stepsQueued; }
	};

	void SeparatePlayerVsOther_StopPlayerOnly(
		collision::World& world,
		GameObject* player, GameObject* other,
		glm::vec3& pPos, glm::vec3& oPos,
		glm::vec2& desiredMove, bool& hasClickTarget,
		float weightPlayerSplit);

	void MoveYLaneWithBounce(
		collision::World& world,
		GameObject* obj, glm::vec3& pos, glm::vec2& vel,
		float laneX, float physicsDt);

	void ElasticBounceEqualMass(
		GameObject* aObj, GameObject* bObj,
		glm::vec3& aPos, glm::vec3& bPos,
		glm::vec2& aVel, glm::vec2& bVel);

	collision::AABB MakeColliderBox(GameObject* obj, const glm::vec3& pos);
	void ClampInsideWalk(const collision::WalkArea& w, GameObject* obj, glm::vec3& pos);

} // namespace physics
