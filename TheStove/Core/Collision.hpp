/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Collision.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Declares basic collision primitives (AABB, MTV helpers) and a simple
					2D collision world that trims desired motion against static walls.

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <cfloat>
#include <vector>

#include "Math.hpp"

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

	struct WoodVertical {
		float x0, x1;			// slab x-range
		float topMinY, topMaxY; // top solid segment
		float gapMinY, gapMaxY; // middle gap (non-solid)
		float botMinY, botMaxY; // bottom solid segment
	};

	struct StageEndGateVertical {
		float x0, x1;			// slab x-range
		float topMinY, topMaxY; // top solid segment
		float gapMinY, gapMaxY; // middle gap (non-solid)
		float botMinY, botMaxY; // bottom solid segment
	};

	// Primitive Queries

	// Returns true and writes MTV if overlapping (A relative to B). False if separated.
	bool overlapMTV(const AABB& a, const AABB& b, Math::Vector2D& mtvOut);

	// Splits MTV using weightA in [0..1] between A and B (moveA/moveB are outputs).
	bool separateWeighted(const AABB& a, const AABB& b, float weightA, Math::Vector2D& moveA, Math::Vector2D& moveB);

	// Half-extent check: is 2D point inside AABB defined by center/scale (x,y used)?
	bool pointInsideCenterAABB(const Math::Vector2D& point, const Math::Vector3D& center, const Math::Vector3D& scale);

	// World (static walls + resolve)
	class World {
	public:
		// Walls management
		void clear();
		void addWall(const AABB& aabb);

		// Build boundary/obstacles from editor primitives.
		void build(const WalkArea& walk, const WoodVertical& wood, const StageEndGateVertical& end);

		// Axis-separable sweep: tries desiredDelta, trims by walls, returns allowed delta.
		Math::Vector2D resolve(const AABB& startBox, Math::Vector2D desiredDelta) const;

		// Utility: construct AABB from 2D center & 2D size (z ignored).
		static AABB makeAABBFromCenter(const Math::Vector3D& center, const Math::Vector3D& scale);

	private:
		std::vector<AABB> mWalls;
	};
}
