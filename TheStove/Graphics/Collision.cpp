/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Collision.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Implementation of AABB-based collision building and resolution.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "Collision.h"
#include <algorithm>
#include <cmath>

namespace collision {
	// AABB overlap test on both axes
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

	static inline float clampf(float v, float lo, float hi) {
		return std::max(lo, std::min(v, hi));
	}

	// AABB minimal translation vector (move A out of B)
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

	// Split MTV by weightA: 0.5 = equal; 1.0 = only A moves
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

	bool pointInsideCenterAABB(glm::vec2 p, glm::vec3 center, glm::vec3 scale) {
		const float hx = scale.x * 0.5f;
		const float hy = scale.y * 0.5f;
		return (p.x >= center.x - hx && p.x <= center.x + hx &&
			p.y >= center.y - hy && p.y <= center.y + hy);
	}


	// World: build walls and resolve swept motion
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
		endTop.min = { end.x0, end.topMinY };
		endTop.max = { end.x1, end.topMaxY };
		mWalls.push_back(endTop);

		AABB endBottom{};
		endBottom.min = { end.x0, end.botMinY };
		endBottom.max = { end.x1, end.botMaxY };
		mWalls.push_back(endBottom);
	}

	glm::vec2 World::resolve(const AABB& startBox, glm::vec2 desiredDelta) const {
		// Start with desired; trim by walls
		glm::vec2 out{};
		out.x = desiredDelta.x;
		out.y = desiredDelta.y;

		// X-sweep
		AABB movedX{};
		movedX.min = startBox.min;
		movedX.max = startBox.max;

		// Compute the intended shift along X
		float shiftX = out.x;

		// Apply the X shift separately for min and max
		float newMinX = movedX.min.x + shiftX;
		float newMaxX = movedX.max.x + shiftX;

		movedX.min.x = newMinX;
		movedX.max.x = newMaxX;

		// Check against each wall
		for (const auto& w : mWalls) {
			bool overlapNow = overlaps(movedX, w);

			if (overlapNow) {
				if (out.x > 0.0f) {
					// Moving right; need to push left
					float penetrationX = movedX.max.x - w.min.x;

					// Correct the output vector
					float correctedOutX = out.x - penetrationX;
					out.x = correctedOutX;

					// Adjust moved box
					float correctedMinX = movedX.min.x - penetrationX;
					float correctedMaxX = movedX.max.x - penetrationX;

					movedX.min.x = correctedMinX;
					movedX.max.x = correctedMaxX;
				}
				else if (out.x < 0.0f) {
					// Moving left; need to push right
					float penetrationX = w.max.x - movedX.min.x;

					// Correct the output vector
					float correctedOutX = out.x + penetrationX;
					out.x = correctedOutX;

					// Adjust moved box
					float correctedMinX = movedX.min.x + penetrationX;
					float correctedMaxX = movedX.max.x + penetrationX;

					movedX.min.x = correctedMinX;
					movedX.max.x = correctedMaxX;
				}
			}
		}

		// Y-sweep
		AABB movedY{};
		movedY.min = movedX.min;
		movedY.max = movedX.max;

		// Compute the intended shift along Y
		float shiftY = out.y;

		// Apply the Y shift separately
		float newMinY = movedY.min.y + shiftY;
		float newMaxY = movedY.max.y + shiftY;

		movedY.min.y = newMinY;
		movedY.max.y = newMaxY;

		// Check against each wall
		for (const auto& w : mWalls) {
			bool overlapNow = overlaps(movedY, w);

			if (overlapNow) {
				if (out.y > 0.0f) {
					// Moving down; need to push up
					float penetrationY = movedY.max.y - w.min.y;

					// Correct output vector
					float correctedOutY = out.y - penetrationY;
					out.y = correctedOutY;

					// Adjust moved box
					float correctedMinY = movedY.min.y - penetrationY;
					float correctedMaxY = movedY.max.y - penetrationY;

					movedY.min.y = correctedMinY;
					movedY.max.y = correctedMaxY;
				}
				else if (out.y < 0.0f) {
					// Moving up; need to push down
					float penetrationY = w.max.y - movedY.min.y;

					// Correct output vector
					float correctedOutY = out.y + penetrationY;
					out.y = correctedOutY;

					// Adjust moved box
					float correctedMinY = movedY.min.y + penetrationY;
					float correctedMaxY = movedY.max.y + penetrationY;

					movedY.min.y = correctedMinY;
					movedY.max.y = correctedMaxY;
				}
			}
		}

		return out;
	}

	// Center + size
	AABB World::makeAABBFromCenter(const glm::vec3& center, const glm::vec3& scale) {
		// Half extents
		float halfWidth = scale.x * 0.5f;
		float halfHeight = scale.y * 0.5f;

		glm::vec2 halfExtents{};
		halfExtents.x = halfWidth;
		halfExtents.y = halfHeight;

		// Extract 2D center
		float cx = center.x;
		float cy = center.y;

		glm::vec2 center2D{};
		center2D.x = cx;
		center2D.y = cy;

		// Compute min corner
		float minX = center2D.x - halfExtents.x;
		float minY = center2D.y - halfExtents.y;

		glm::vec2 minCorner{};
		minCorner.x = minX;
		minCorner.y = minY;

		// Compute max corner
		float maxX = center2D.x + halfExtents.x;
		float maxY = center2D.y + halfExtents.y;

		glm::vec2 maxCorner{};
		maxCorner.x = maxX;
		maxCorner.y = maxY;

		// Construct AABB
		AABB box{};
		box.min = minCorner;
		box.max = maxCorner;

		return box;
	}
}
