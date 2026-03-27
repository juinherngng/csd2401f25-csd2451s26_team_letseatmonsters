/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Collision.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Declares basic collision primitives (AABB, MTV helpers) and a simple
					2D collision world that trims desired motion against static walls.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <cfloat>
#include <vector>

#include "EngineCore/Math.hpp"

namespace collision {
	// Primitives
	struct AABB {
		Math::Vector2D min; // lower-left
		Math::Vector2D max; // upper-right
	};

	// Axis-aligned walkable region & obstacle pieces used to build walls.
	struct WalkArea {
		float L, R, T, B; // bounds (left/right/top/bottom)
		float edgeThick;  // boundary wall thickness
	};

	// Vertical slabs with a gap in the middle, used for wood and end gate obstacles.
	struct WoodVertical {
		float x0, x1;			// slab x-range
		float topMinY, topMaxY; // top solid segment
		float gapMinY, gapMaxY; // middle gap (non-solid)
		float botMinY, botMaxY; // bottom solid segment
	};

	// Similar to WoodVertical but with different dimensions, used for the stage end gate.
	struct StageEndGateVertical {
		float x0, x1;			// slab x-range
		float topMinY, topMaxY; // top solid segment
		float gapMinY, gapMaxY; // middle gap (non-solid)
		float botMinY, botMaxY; // bottom solid segment
	};

	/**
	 * @brief Returns whether overlapmtv.
	 * @param a Parameter for a.
	 * @param b Parameter for b.
	 * @param mtvOut Parameter for mtv out.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool overlapMTV(const AABB& a, const AABB& b, Math::Vector2D& mtvOut);

	/**
	 * @brief Returns whether separateweighted.
	 * @param a Parameter for a.
	 * @param b Parameter for b.
	 * @param weightA Parameter for weight a.
	 * @param moveA Parameter for move a.
	 * @param moveB Parameter for move b.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool separateWeighted(const AABB& a, const AABB& b, float weightA, Math::Vector2D& moveA, Math::Vector2D& moveB);

	/**
	 * @brief Returns whether pointinsidecenteraabb.
	 * @param point Parameter for point.
	 * @param center Parameter for center.
	 * @param scale Parameter for scale.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool pointInsideCenterAABB(const Math::Vector2D& point, const Math::Vector3D& center, const Math::Vector3D& scale);

	// World (static walls + resolve)
	class World {
	public:

		/**
		 * @brief Clears this object.
		 */
		void clear();

		/**
		 * @brief Adds wall.
		 * @param aabb Parameter for aabb.
		 */
		void addWall(const AABB& aabb);

		/**
		 * @brief Builds this object.
		 * @param walk Parameter for walk.
		 * @param wood Parameter for wood.
		 * @param end Parameter for end.
		 */
		void build(const WalkArea& walk, const WoodVertical& wood, const StageEndGateVertical& end);

		/**
		 * @brief Resolves this object.
		 * @param startBox Parameter for start box.
		 * @param desiredDelta Parameter for desired delta.
		 * @return Result produced by this operation.
		 */
		Math::Vector2D resolve(const AABB& startBox, Math::Vector2D desiredDelta) const;

		/**
		 * @brief Performs make aabbfrom center.
		 * @param center Parameter for center.
		 * @param scale Parameter for scale.
		 * @return Result produced by this operation.
		 */
		static AABB makeAABBFromCenter(const Math::Vector3D& center, const Math::Vector3D& scale);

		/**
		 * @brief Returns whether overlapsanywall.
		 * @param box Parameter for box.
		 * @return True when the operation succeeds or the condition is met.
		 */
		bool overlapsAnyWall(const AABB& box) const;

	private:
		// Internal helper methods and state
		std::vector<AABB> mWalls;
	};
}
