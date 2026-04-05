/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			PlayerLogicShared.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Shared internal helpers and constants for the split PlayerLogic implementation.
					- Defines movement, interaction, and click-indicator tuning constants
					- Provides reusable geometry and vector helper functions
					- Centralizes small utility routines shared across PlayerLogic source files
					- Supports the split implementation without changing the public class API

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/PlayerLogic.hpp"
#include "GameCore/TableLogic.hpp"

namespace PlayerLogicDetail {
	// Interaction and movement tuning constants shared across the split PlayerLogic implementation.
	inline constexpr float kPlayerInteractRadius = 48.0f;
	inline constexpr float kMoveRetargetDeadzone = 6.0f;
	inline constexpr float kDragRetargetDistance = 12.0f;
	inline constexpr float kDragRetargetInterval = 0.035f;
	inline constexpr float kArriveRadius = 10.0f;
	inline constexpr int kBlockedFramesBeforeCancel = 6;

	inline constexpr float kKeyboardMoveSpeed = 200.0f;
	inline constexpr float kTrailJitterEpsilon = 0.25f;
	inline constexpr float kFootstepInterval = 0.3f;
	inline constexpr float kClickIndicatorLifetime = 0.45f;
	inline constexpr float kClickIndicatorBaseSize = 37.0f;
	inline constexpr float kMaxPlayerUpdateDt = 1.0f / 30.0f;

	/**
	 * @brief Returns whether a world-space point lies within an object's visual bounds.
	 * @param point World-space point to test.
	 * @param obj Object whose bounds should be checked.
	 * @return True when the point lies inside the resolved object rectangle.
	 */
	inline bool PointInsideObjectVisualRect(const glm::vec2& point, GameObject* obj) {
		// Fall back to scale when no collider size is authored so hover checks still work.
		if (!obj) {
			return false;
		}

		const Math::Vector2D colSize = obj->GetColliderSize();
		const Math::Vector2D colOffset = obj->GetColliderOffset();
		const glm::vec3 scale = obj->GetScaleGLM();
		const float width = (colSize.x > 0.0f) ? colSize.x : scale.x;
		const float height = (colSize.y > 0.0f) ? colSize.y : scale.y;
		if (width <= 0.0f || height <= 0.0f) {
			return false;
		}

		const glm::vec3 pos = obj->GetPositionGLM();
		const glm::vec2 center(pos.x + colOffset.x, pos.y + colOffset.y);
		const float halfW = width * 0.5f;
		const float halfH = height * 0.5f;

		return
			point.x >= center.x - halfW && point.x <= center.x + halfW &&
			point.y >= center.y - halfH && point.y <= center.y + halfH;
	}

	/**
	 * @brief Returns the squared distance from a point to an object's center.
	 * @param point World-space point to compare from.
	 * @param obj Object whose center should be measured.
	 * @return Squared distance, or a large sentinel when the object is invalid.
	 */
	inline float DistanceSqToObjectCenter(const glm::vec2& point, GameObject* obj) {
		// Use squared distance to avoid an unnecessary square root in selection code.
		if (!obj) {
			return std::numeric_limits<float>::max();
		}

		const Math::Vector2D colOffset = obj->GetColliderOffset();
		const glm::vec3 pos = obj->GetPositionGLM();
		const glm::vec2 center(pos.x + colOffset.x, pos.y + colOffset.y);
		const glm::vec2 d = point - center;
		return d.x * d.x + d.y * d.y;
	}

	/**
	 * @brief Resolves an object's center point and half extents.
	 * @param obj Object whose rectangle should be extracted.
	 * @param outCenter Receives the rectangle center in world space.
	 * @param outHalfExtents Receives the rectangle half extents.
	 * @return True when a valid rectangle could be built for the object.
	 */
	inline bool GetObjectRect(GameObject* obj, glm::vec2& outCenter, glm::vec2& outHalfExtents) {
		// Reuse the same collider-or-scale logic everywhere we need simple bounds tests.
		if (!obj) {
			return false;
		}

		const Math::Vector2D colSize = obj->GetColliderSize();
		const Math::Vector2D colOffset = obj->GetColliderOffset();
		const glm::vec3 scale = obj->GetScaleGLM();
		const float width = (colSize.x > 0.0f) ? colSize.x : scale.x;
		const float height = (colSize.y > 0.0f) ? colSize.y : scale.y;
		if (width <= 0.0f || height <= 0.0f) {
			return false;
		}

		const glm::vec3 pos = obj->GetPositionGLM();
		outCenter = glm::vec2(pos.x + colOffset.x, pos.y + colOffset.y);
		outHalfExtents = glm::vec2(width * 0.5f, height * 0.5f);
		return true;
	}

	/**
	 * @brief Returns the squared distance from a point to an expanded axis-aligned rectangle.
	 * @param point World-space point being tested.
	 * @param rectCenter Center of the expanded rectangle.
	 * @param rectHalfExtents Half extents of the expanded rectangle.
	 * @return Squared distance from the point to the rectangle boundary.
	 */
	inline float DistanceSqPointToExpandedRect(
		const glm::vec2& point,
		const glm::vec2& rectCenter,
		const glm::vec2& rectHalfExtents) {
		// Clamp each axis independently so overlapping points report zero distance.
		const float dx = std::max(std::abs(point.x - rectCenter.x) - rectHalfExtents.x, 0.0f);
		const float dy = std::max(std::abs(point.y - rectCenter.y) - rectHalfExtents.y, 0.0f);
		return dx * dx + dy * dy;
	}

	/**
	 * @brief Returns the squared distance between two 2D points.
	 * @param a First point.
	 * @param b Second point.
	 * @return Squared Euclidean distance.
	 */
	inline float DistanceSquared(const glm::vec2& a, const glm::vec2& b) {
		// Keep this helper tiny so drag retarget checks stay readable at the call-site.
		const glm::vec2 delta = a - b;
		return delta.x * delta.x + delta.y * delta.y;
	}

	/**
	 * @brief Drops the Z component from a 3D vector.
	 * @param value Source 3D vector.
	 * @return Equivalent 2D vector containing X and Y only.
	 */
	inline glm::vec2 ToVec2(const glm::vec3& value) {
		// Player navigation is performed in the XY plane only.
		return { value.x, value.y };
	}

	/**
	 * @brief Normalizes a vector when it has meaningful length.
	 * @param v Vector to normalize.
	 * @return Unit-length vector, or zero when the input is too small.
	 */
	inline glm::vec2 NormalizeOrZero(const glm::vec2& v) {
		// Guard tiny vectors so callers can skip epsilon checks after normalization.
		const float len = std::sqrt(v.x * v.x + v.y * v.y);
		if (len <= 0.0001f) {
			return glm::vec2(0.0f, 0.0f);
		}

		return glm::vec2(v.x / len, v.y / len);
	}
}
