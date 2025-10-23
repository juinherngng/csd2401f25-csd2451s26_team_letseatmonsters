/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Collision.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Implementation of AABB-based collision primitives and world resolution.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "Collision.hpp"
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
	bool overlapMTV(const AABB& a, const AABB& b, Math::Vector2D& mtvOut) {
		// Signed gaps (A relative to B)
		float left = b.min.x - a.max.x;
		float right = b.max.x - a.min.x;
		float top = b.min.y - a.max.y;
		float bottom = b.max.y - a.min.y;

		// Early exit if separated on any axis
		if (left > 0 || right < 0 || top > 0 || bottom < 0) {
			mtvOut = Math::Vector2D(0.f, 0.f);
			return false;
		}

		// Smallest correction wins
		float penX = std::abs(left) < std::abs(right) ? left : right;
		float penY = std::abs(top) < std::abs(bottom) ? top : bottom;

		if (std::abs(penX) < std::abs(penY)) {
			mtvOut = Math::Vector2D(penX, 0.f);
		}
		else {
			mtvOut = Math::Vector2D(0.f, penY);
		}

		return true;
	}

	// Splits MTV based on given weight between A and B.
	bool separateWeighted(const AABB& a, const AABB& b, float weightA, Math::Vector2D& moveA, Math::Vector2D& moveB) {
		Math::Vector2D mtv;
		if (!overlapMTV(a, b, mtv)) {
			moveA = Math::Vector2D(0.f, 0.f);
			moveB = Math::Vector2D(0.f, 0.f);
			return false;
		}

		// Clamp weight
		weightA = clampf(weightA, 0.f, 1.f);
		moveA = Math::Vector2D(mtv.x * weightA, mtv.y * weightA);
		moveB = Math::Vector2D(-mtv.x * (1.f - weightA), -mtv.y * (1.f - weightA));
		return true;
	}

	// Half-extents based point inclusion test.
	bool pointInsideCenterAABB(const Math::Vector2D& point, const Math::Vector3D& center, const Math::Vector3D& scale) {
		const float hx = scale.x * 0.5f;
		const float hy = scale.y * 0.5f;
		return (point.x >= center.x - hx && point.x <= center.x + hx &&
			point.y >= center.y - hy && point.y <= center.y + hy);
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
		leftWall.min = Math::Vector2D(w.L - w.edgeThick, w.T);
		leftWall.max = Math::Vector2D(w.L, w.B);
		mWalls.push_back(leftWall);

		// RIGHT edge wall (TOP segment)
		AABB rightTopWall{};
		rightTopWall.min = Math::Vector2D(w.R, w.T);
		rightTopWall.max = Math::Vector2D(w.R + w.edgeThick, end.gapMinY);
		mWalls.push_back(rightTopWall);

		// RIGHT edge wall (BOTTOM segment)
		AABB rightBottomWall{};
		rightBottomWall.min = Math::Vector2D(w.R, end.gapMaxY);
		rightBottomWall.max = Math::Vector2D(w.R + w.edgeThick, w.B);
		mWalls.push_back(rightBottomWall);

		// TOP edge wall
		AABB topWall{};
		topWall.min = Math::Vector2D(w.L, w.T - w.edgeThick);
		topWall.max = Math::Vector2D(w.R, w.T);
		mWalls.push_back(topWall);

		// BOTTOM edge wall
		AABB bottomWall{};
		bottomWall.min = Math::Vector2D(w.L, w.B);
		bottomWall.max = Math::Vector2D(w.R, w.B + w.edgeThick);
		mWalls.push_back(bottomWall);

		// Wooden divider: TOP solid, middle GAP (skipped), BOTTOM solid
		AABB woodTop{};
		woodTop.min = Math::Vector2D(wood.x0, wood.topMinY);
		woodTop.max = Math::Vector2D(wood.x1, wood.topMaxY);
		mWalls.push_back(woodTop);

		AABB woodBottom{};
		woodBottom.min = Math::Vector2D(wood.x0, wood.botMinY);
		woodBottom.max = Math::Vector2D(wood.x1, wood.botMaxY);
		mWalls.push_back(woodBottom);

		// End gate: TOP solid, middle GAP (skipped), BOTTOM solid
		AABB endTop{};
		endTop.min = Math::Vector2D(end.x0, end.topMinY);
		endTop.max = Math::Vector2D(end.x1, end.topMaxY);
		mWalls.push_back(endTop);

		AABB endBottom{};
		endBottom.min = Math::Vector2D(end.x0, end.botMinY);
		endBottom.max = Math::Vector2D(end.x1, end.botMaxY);
		mWalls.push_back(endBottom);
	}

	// Axis-separable sweep test: move along X then Y, correcting overlaps.
	Math::Vector2D World::resolve(const AABB& startBox, Math::Vector2D desiredDelta) const {
		// Begin with desired; trim by walls.
		Math::Vector2D allowedDelta = desiredDelta;

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
	AABB World::makeAABBFromCenter(const Math::Vector3D& center, const Math::Vector3D& scale) {
		const float halfW = scale.x * 0.5f;
		const float halfH = scale.y * 0.5f;

		AABB box{};
		box.min = Math::Vector2D(center.x - halfW, center.y - halfH);
		box.max = Math::Vector2D(center.x + halfW, center.y + halfH);
		return box;
	}
}
