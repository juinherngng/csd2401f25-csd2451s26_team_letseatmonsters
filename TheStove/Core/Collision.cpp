/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Collision.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implementation of AABB-based collision primitives and world resolution.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <cmath>

#include "Collision.hpp"

namespace collision {
	static constexpr float kEPS = 1e-4f;

	static inline float clampf(float v, float lo, float hi) {
		return std::max(lo, std::min(v, hi));
	}

	// Internal overlap test (AABB vs AABB).
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

	// Primitives 
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
		float penX = std::abs(left) < std::abs(right)?left:right;
		float penY = std::abs(top) < std::abs(bottom)?top:bottom;

		if (std::abs(penX) < std::abs(penY)) {
			mtvOut = Math::Vector2D(penX, 0.f);
		}
		else {
			mtvOut = Math::Vector2D(0.f, penY);
		}

		return true;
	}

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

	bool pointInsideCenterAABB(const Math::Vector2D& point, const Math::Vector3D& center, const Math::Vector3D& scale) {
		const float hx = scale.x * 0.5f;
		const float hy = scale.y * 0.5f;
		return (point.x >= center.x - hx && point.x <= center.x + hx &&
				point.y >= center.y - hy && point.y <= center.y + hy);
	}

	// World methods
	void World::clear() {
		mWalls.clear();
	}

	void World::addWall(const AABB& aabb) {
		mWalls.push_back(aabb);
	}

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

	Math::Vector2D World::resolve(const AABB& startBox, Math::Vector2D desiredDelta) const {
		// Begin with desired; trim by walls.
		Math::Vector2D allowedDelta = desiredDelta;

		// X sweep
		AABB movedX = startBox;

		movedX.min.x += allowedDelta.x;
		movedX.max.x += allowedDelta.x;

		for (const auto& wall : mWalls) {
			// We only care if we overlap on Y (otherwise no X collision possible)
			const bool yOverlap = !(movedX.min.y >= wall.max.y || movedX.max.y <= wall.min.y);
			if (!yOverlap) {
				continue;
			}

			if (allowedDelta.x > 0.0f) {
				// Moving right; we must be to the left initially and cross wall.min.x
				const bool wasLeft = (startBox.max.x <= wall.min.x + kEPS);
				const bool nowPenetrating = (movedX.max.x > wall.min.x);
				if (wasLeft && nowPenetrating) {
					const float penX = movedX.max.x - wall.min.x;
					const float corr = std::min(penX, allowedDelta.x); // don’t overshoot

					allowedDelta.x -= corr;
					movedX.min.x -= corr;
					movedX.max.x -= corr;
				}
			}
			else if (allowedDelta.x < 0.0f) {
				// Moving left; we must be to the right initially and cross wall.max.x
				const bool wasRight = (startBox.min.x >= wall.max.x - kEPS);
				const bool nowPenetrating = (movedX.min.x < wall.max.x);
				if (wasRight && nowPenetrating) {
					const float penX = wall.max.x - movedX.min.x;
					const float corr = std::min(penX, -allowedDelta.x); // don’t overshoot

					allowedDelta.x += corr;
					movedX.min.x += corr;
					movedX.max.x += corr;
				}
			}
		}

		// Y sweep
		AABB movedY = movedX;

		movedY.min.y += allowedDelta.y;
		movedY.max.y += allowedDelta.y;

		for (const auto& wall : mWalls) {
			// Only resolve if we overlap on X (otherwise no Y collision possible)
			const bool xOverlap = !(movedY.min.x >= wall.max.x || movedY.max.x <= wall.min.x);
			if (!xOverlap) {
				continue;
			}

			if (allowedDelta.y > 0.0f) {
				// Moving down; must be above initially and cross wall.min.y
				const bool wasAbove = (startBox.max.y <= wall.min.y + kEPS);
				const bool nowPenetrating = (movedY.max.y > wall.min.y);
				if (wasAbove && nowPenetrating) {
					const float penY = movedY.max.y - wall.min.y;
					const float corr = std::min(penY, allowedDelta.y); // don’t overshoot

					allowedDelta.y -= corr;
					movedY.min.y -= corr;
					movedY.max.y -= corr;
				}
			}
			else if (allowedDelta.y < 0.0f) {
				// Moving up; must be below initially and cross wall.max.y
				const bool wasBelow = (startBox.min.y >= wall.max.y - kEPS);
				const bool nowPenetrating = (movedY.min.y < wall.max.y);
				if (wasBelow && nowPenetrating) {
					const float penY = wall.max.y - movedY.min.y;
					const float corr = std::min(penY, -allowedDelta.y); // don’t overshoot

					allowedDelta.y += corr;
					movedY.min.y += corr;
					movedY.max.y += corr;
				}
			}
		}

		return allowedDelta;
	}

	AABB World::makeAABBFromCenter(const Math::Vector3D& center, const Math::Vector3D& scale) {
		const float halfW = scale.x * 0.5f;
		const float halfH = scale.y * 0.5f;

		AABB box{};
		box.min = Math::Vector2D(center.x - halfW, center.y - halfH);
		box.max = Math::Vector2D(center.x + halfW, center.y + halfH);
		return box;
	}
}
