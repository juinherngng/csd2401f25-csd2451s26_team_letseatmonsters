#pragma once
#include <vector>
#include <glm/glm.hpp>

namespace coll {
	struct AABB {
		glm::vec2 min;
		glm::vec2 max;
	};

	// Walkable light-gray rectangle (inside the dark rim) + how thick the edge bars are
	struct WalkArea {
		float L, R, T, B;    // left/right/top/bottom of light-gray play area
		float edgeThick;     // thin blocking bars placed just inside these edges
	};

	// Your vertical wooden divider split into: TOP solid, GAP (walk-through), BOTTOM solid
	struct WoodVertical {
		float x0, x1;              // wood left/right
		float topMinY, topMaxY;    // top solid segment
		float gapMinY, gapMaxY;    // open gap (NO collider)
		float botMinY, botMaxY;    // bottom solid segment
	};

	class World {
	public:
		World() = default;

		void clear();
		void addWall(const AABB& aabb);
		void build(const WalkArea& walk, const WoodVertical& wood);

		// Resolve collision for a swept move (non-reversing correction + small skin)
		glm::vec2 resolve(const AABB& startBox, glm::vec2 desiredDelta) const;

		// Helper to build an AABB from a center-anchored sprite
		static AABB makeAABBFromCenter(const glm::vec3& center, const glm::vec3& scale);

		const std::vector<AABB>& walls() const { return mWalls; }

	private:
		std::vector<AABB> mWalls;
		static constexpr float kSkin = 0.75f; // microscopic separation to avoid “snap”
	};

}
