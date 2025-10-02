/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Collision.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Implementation of AABB-based collision primitives and world resolution.

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "Collision.h"
#include <algorithm>
#include <cmath>

namespace collision {
	/**
	 * @brief Check overlap between two AABBs.
	 * @param a First AABB.
	 * @param b Second AABB.
	 * @return true if overlap exists, false otherwise.
	 */
	static inline bool overlaps(const AABB& a, const AABB& b) {
		bool aRightOfB = (a.min.x >= b.max.x);
		bool aLeftOfB = (a.max.x <= b.min.x);
		bool noOverlapX = aRightOfB || aLeftOfB;

		bool aAboveB = (a.min.y >= b.max.y);
		bool aBelowB = (a.max.y <= b.min.y);
		bool noOverlapY = aAboveB || aBelowB;

		bool anyNoOverlap = noOverlapX || noOverlapY;
		bool overlapExists = !anyNoOverlap;

		return overlapExists;
	}

	/**
	 * @brief Clamp float between two values.
	 * @param v Value to clamp.
	 * @param lo Minimum bound.
	 * @param hi Maximum bound.
	 * @return Clamped value.
	 */
	static inline float clampf(float v, float lo, float hi) {
		return std::max(lo, std::min(v, hi));
	}

	// --- Primitives ---

	// Computes signed penetration depths and selects smallest axis.
	bool overlapMTV(const AABB& a, const AABB& b, glm::vec2& mtvOut) {
		// Signed gaps (A relative to B)
		float left = b.min.x - a.max.x;
		float right = b.max.x - a.min.x;
		float top = b.min.y - a.max.y;
		float bottom = b.max.y - a.min.y;

		// Early exit if separated on any axis
		if (left > 0 || right < 0 || top > 0 || bottom < 0) {
			mtvOut = { 0.0f, 0.0f };
			return false;
		}

		// Smallest correction wins
		float penX = std::abs(left) < std::abs(right) ? left : right;
		float penY = std::abs(top) < std::abs(bottom) ? top : bottom;

		if (std::abs(penX) < std::abs(penY)) {
			mtvOut = { penX, 0.0f };
		}
		else {
			mtvOut = { 0.0f, penY };
		}

		return true;
	}

	// Splits MTV based on given weight between A and B.
	bool separateWeighted(const AABB& a, const AABB& b, float weightA, glm::vec2& moveA, glm::vec2& moveB) {
		glm::vec2 mtv;
		if (!overlapMTV(a, b, mtv)) {
			moveA = { 0.0f, 0.0f };
			moveB = { 0.0f, 0.0f };
			return false;
		}

		// Clamp weight
		weightA = glm::clamp(weightA, 0.0f, 1.0f);
		moveA = weightA * mtv;
		moveB = -(1.0f - weightA) * mtv;
		return true;
	}

	// Half-extents based point inclusion test.
	bool pointInsideCenterAABB(glm::vec2 p, glm::vec3 center, glm::vec3 scale) {
		const float hx = scale.x * 0.5f;
		const float hy = scale.y * 0.5f;
		return (p.x >= center.x - hx && p.x <= center.x + hx &&
			p.y >= center.y - hy && p.y <= center.y + hy);
	}

	// --- World methods ---

	// Reset world by removing all walls.
	void World::clear() {
		mWalls.clear();
	}

	// Push custom AABB wall into world.
	void World::addWall(const AABB& aabb) {
		mWalls.push_back(aabb);
	}

	// Builds world boundary and obstacles from primitives.
	void World::build(const WalkArea& w, const WoodVertical& wood, const StageEndGateVertical& end) {
		mWalls.clear();

		// LEFT edge wall
		AABB leftWall{};
		leftWall.min = { w.L - w.edgeThick, w.T };
		leftWall.max = { w.L, w.B };
		mWalls.push_back(leftWall);

		// RIGHT edge wall (TOP segment)
		AABB rightTopWall{};
		rightTopWall.min = { w.R, w.T };
		rightTopWall.max = { w.R + w.edgeThick, end.gapMinY };
		mWalls.push_back(rightTopWall);

		// RIGHT edge wall (BOTTOM segment)
		AABB rightBottomWall{};
		rightBottomWall.min = { w.R, end.gapMaxY };
		rightBottomWall.max = { w.R + w.edgeThick, w.B };
		mWalls.push_back(rightBottomWall);

		// TOP edge wall
		AABB topWall{};
		topWall.min = { w.L, w.T - w.edgeThick };
		topWall.max = { w.R, w.T };
		mWalls.push_back(topWall);

		// BOTTOM edge wall
		AABB bottomWall{};
		bottomWall.min = { w.L, w.B };
		bottomWall.max = { w.R, w.B + w.edgeThick };
		mWalls.push_back(bottomWall);

		// Wooden divider: TOP solid, middle GAP (skipped), BOTTOM solid
		AABB woodTop{};
		woodTop.min = { wood.x0, wood.topMinY };
		woodTop.max = { wood.x1, wood.topMaxY };
		mWalls.push_back(woodTop);

		AABB woodBottom{};
		woodBottom.min = { wood.x0, wood.botMinY };
		woodBottom.max = { wood.x1, wood.botMaxY };
		mWalls.push_back(woodBottom);

		// End gate: TOP solid, middle GAP (skipped), BOTTOM solid
		AABB endTop{};
		endTop.min = { end.x0 + kSkin, end.topMinY };
		endTop.max = { end.x1, end.topMaxY };
		mWalls.push_back(endTop);

		AABB endBottom{};
		endBottom.min = { end.x0 + kSkin, end.botMinY };
		endBottom.max = { end.x1, end.botMaxY };
		mWalls.push_back(endBottom);
	}

	// Axis-separable sweep test: move along X then Y, correcting overlaps.
	glm::vec2 World::resolve(const AABB& startBox, glm::vec2 desiredDelta) const {
		// Begin with desired; trim by walls.
		glm::vec2 allowedDelta = desiredDelta;

		// X sweep
		AABB movedX = startBox;

		// Apply X shift
		movedX.min.x += allowedDelta.x;
		movedX.max.x += allowedDelta.x;

		// Check against each wall
		for (const auto& wall : mWalls) {
			const bool hit = overlaps(movedX, wall);
			if (!hit) continue;

			if (allowedDelta.x > 0.0f) {
				// Moving right; push left
				const float penX = movedX.max.x - wall.min.x;

				allowedDelta.x -= penX; // trim
				movedX.min.x -= penX;   // shift box back
				movedX.max.x -= penX;

			}
			else if (allowedDelta.x < 0.0f) {
				// Moving left; push right
				const float penX = wall.max.x - movedX.min.x;

				allowedDelta.x += penX; // trim
				movedX.min.x += penX;   // shift box forward
				movedX.max.x += penX;
			}
		}

		// Y sweep
		AABB movedY = movedX;

		// Apply Y shift
		movedY.min.y += allowedDelta.y;
		movedY.max.y += allowedDelta.y;

		for (const auto& wall : mWalls) {
			const bool hit = overlaps(movedY, wall);
			if (!hit) continue;

			if (allowedDelta.y > 0.0f) {
				// Moving down; push up
				const float penY = movedY.max.y - wall.min.y;

				allowedDelta.y -= penY;
				movedY.min.y -= penY;
				movedY.max.y -= penY;

			}
			else if (allowedDelta.y < 0.0f) {
				// Moving up; push down
				const float penY = wall.max.y - movedY.min.y;

				allowedDelta.y += penY;
				movedY.min.y += penY;
				movedY.max.y += penY;
			}
		}

		return allowedDelta;
	}

	// Construct AABB from 2D center and full size.
	AABB World::makeAABBFromCenter(const glm::vec3& center, const glm::vec3& scale) {
		const float halfW = scale.x * 0.5f;
		const float halfH = scale.y * 0.5f;

		const glm::vec2 c2{ center.x, center.y };

		AABB box{};
		box.min = { c2.x - halfW, c2.y - halfH };
		box.max = { c2.x + halfW, c2.y + halfH };
		return box;
	}
}
