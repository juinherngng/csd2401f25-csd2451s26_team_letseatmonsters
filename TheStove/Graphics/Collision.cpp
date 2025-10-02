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
	// Test strict AABB overlap on X and Y
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

	void World::clear() {
		mWalls.clear();
	}

	void World::addWall(const AABB& aabb) {
		mWalls.push_back(aabb);
	}

	void World::build(const WalkArea& w,
		const WoodVertical& wood,
		const StageEndGateVertical& end) {
		mWalls.clear();

		// LEFT edge wall
		AABB leftWall{};
		glm::vec2 leftWallMin{};
		glm::vec2 leftWallMax{};

		leftWallMin.x = w.L - w.edgeThick;
		leftWallMin.y = w.T;

		leftWallMax.x = w.L;
		leftWallMax.y = w.B;

		leftWall.min = leftWallMin;
		leftWall.max = leftWallMax;

		mWalls.push_back(leftWall);

		// RIGHT edge wall (TOP segment)
		AABB rightTopWall{};
		glm::vec2 rightTopMin{};
		glm::vec2 rightTopMax{};

		rightTopMin.x = w.R;
		rightTopMin.y = w.T;

		rightTopMax.x = w.R + w.edgeThick;
		rightTopMax.y = end.gapMinY;

		rightTopWall.min = rightTopMin;
		rightTopWall.max = rightTopMax;

		mWalls.push_back(rightTopWall);

		// RIGHT edge wall (BOTTOM segment)
		AABB rightBottomWall{};
		glm::vec2 rightBottomMin{};
		glm::vec2 rightBottomMax{};

		rightBottomMin.x = w.R;
		rightBottomMin.y = end.gapMaxY;

		rightBottomMax.x = w.R + w.edgeThick;
		rightBottomMax.y = w.B;

		rightBottomWall.min = rightBottomMin;
		rightBottomWall.max = rightBottomMax;

		mWalls.push_back(rightBottomWall);

		// TOP edge wall
		AABB topWall{};
		glm::vec2 topMin{};
		glm::vec2 topMax{};

		topMin.x = w.L;
		topMin.y = w.T - w.edgeThick;

		topMax.x = w.R;
		topMax.y = w.T;

		topWall.min = topMin;
		topWall.max = topMax;

		mWalls.push_back(topWall);

		// BOTTOM edge wall
		AABB bottomWall{};
		glm::vec2 bottomMin{};
		glm::vec2 bottomMax{};

		bottomMin.x = w.L;
		bottomMin.y = w.B;

		bottomMax.x = w.R;
		bottomMax.y = w.B + w.edgeThick;

		bottomWall.min = bottomMin;
		bottomWall.max = bottomMax;

		mWalls.push_back(bottomWall);

		// WOOD divider (TOP segment)
		AABB woodTop{};
		glm::vec2 woodTopMin{};
		glm::vec2 woodTopMax{};

		woodTopMin.x = wood.x0;
		woodTopMin.y = wood.topMinY;

		woodTopMax.x = wood.x1;
		woodTopMax.y = wood.topMaxY;

		woodTop.min = woodTopMin;
		woodTop.max = woodTopMax;

		mWalls.push_back(woodTop);

		// WOOD divider (BOTTOM segment)
		AABB woodBottom{};
		glm::vec2 woodBottomMin{};
		glm::vec2 woodBottomMax{};

		woodBottomMin.x = wood.x0;
		woodBottomMin.y = wood.botMinY;

		woodBottomMax.x = wood.x1;
		woodBottomMax.y = wood.botMaxY;

		woodBottom.min = woodBottomMin;
		woodBottom.max = woodBottomMax;

		mWalls.push_back(woodBottom);

		// END gate (TOP segment)
		AABB endTop{};
		glm::vec2 endTopMin{};
		glm::vec2 endTopMax{};

		endTopMin.x = end.x0;
		endTopMin.y = end.topMinY;

		endTopMax.x = end.x1;
		endTopMax.y = end.topMaxY;

		endTop.min = endTopMin;
		endTop.max = endTopMax;

		mWalls.push_back(endTop);

		// END gate (BOTTOM segment)
		AABB endBottom{};
		glm::vec2 endBottomMin{};
		glm::vec2 endBottomMax{};

		endBottomMin.x = end.x0;
		endBottomMin.y = end.botMinY;

		endBottomMax.x = end.x1;
		endBottomMax.y = end.botMaxY;

		endBottom.min = endBottomMin;
		endBottom.max = endBottomMax;

		mWalls.push_back(endBottom);
	}

	glm::vec2 World::resolve(const AABB& startBox, glm::vec2 desiredDelta) const {
		// Output vector starts as the desired motion
		glm::vec2 out{};
		out.x = desiredDelta.x;
		out.y = desiredDelta.y;

		// Handle X-axis motion
		// Copy the adjusted box
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

		// Handle Y-axis motion
		// Copy the adjusted box
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
