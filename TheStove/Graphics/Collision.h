/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Collision.h
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Axis-aligned bounding box (AABB) primitives and a lightweight collision world.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace collision {
	// Primitives
	struct AABB {
		glm::vec2 min;
		glm::vec2 max;
	};

	// Walkable rectangle with thin blocking edges
	struct WalkArea {
		float L;
		float R;
		float T;
		float B;

		float edgeThick;
	};

	// Vertical divider: TOP (solid), GAP (open), BOTTOM (solid)
	struct WoodVertical {
		float x0, x1;
		float topMinY, topMaxY;
		float gapMinY, gapMaxY;
		float botMinY, botMaxY;
	};

	// End gate: TOP (solid), GAP (open), BOTTOM (solid).
	struct StageEndGateVertical {
		float x0, x1;
		float topMinY, topMaxY;
		float gapMinY, gapMaxY;
		float botMinY, botMaxY;
	};

	// If A and B overlap, mtvOut is the minimal vector to move A out of B.
	bool overlapMTV(const AABB& a, const AABB& b, glm::vec2& mtvOut);

	// Split MTV between A and B with weightA in [0..1] (0.5 = equal, 1.0 = only A moves).
	bool separateWeighted(const AABB& a, const AABB& b, float weightA, glm::vec2& moveA, glm::vec2& moveB);

	// Static world
	class World {
	public:
		World() = default;

		void clear();
		void addWall(const AABB& aabb);
		void build(const WalkArea& walk, const WoodVertical& wood, const StageEndGateVertical& end);

		// Axis-separable sweep: returns allowedDelta that doesn’t penetrate walls
		glm::vec2 resolve(const AABB& startBox, glm::vec2 desiredDelta) const;

		// Create an AABB from a center and scale (X/Y used; Z ignored).
		static AABB makeAABBFromCenter(const glm::vec3& center, const glm::vec3& scale);

		// Read-only access to internal wall list (for debugging/visualization).
		const std::vector<AABB>& walls() const { return mWalls; }

	private:
		std::vector<AABB> mWalls;
		static constexpr float kSkin = 0.75f;
	};
}
