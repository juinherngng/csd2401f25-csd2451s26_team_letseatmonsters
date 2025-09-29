#include "Collision.h"
#include <algorithm>
#include <cmath>

namespace coll {
	static inline bool overlaps(const AABB& a, const AABB& b) {
		return !(a.max.x <= b.min.x || a.min.x >= b.max.x ||
			a.max.y <= b.min.y || a.min.y >= b.max.y);
	}

	void World::clear() {
		mWalls.clear();
	}

	void World::addWall(const AABB& aabb) {
		mWalls.push_back(aabb);
	}

	void World::build(const WalkArea& w, const WoodVertical& wood) {
		mWalls.clear();

		// 4 inside-edge bars: stop exactly at the light-gray boundary
		mWalls.push_back({ { w.L - w.edgeThick, w.T }, { w.L, w.B } });           // left
		mWalls.push_back({ { w.R, w.T },         { w.R + w.edgeThick, w.B } });   // right
		mWalls.push_back({ { w.L, w.T - w.edgeThick }, { w.R, w.T } });           // top
		mWalls.push_back({ { w.L, w.B },         { w.R, w.B + w.edgeThick } });   // bottom

		// Wooden divider: TOP solid + BOTTOM solid (middle gap is pass-through)
		mWalls.push_back({ { wood.x0, wood.topMinY }, { wood.x1, wood.topMaxY } });
		mWalls.push_back({ { wood.x0, wood.botMinY }, { wood.x1, wood.botMaxY } });
	}

	glm::vec2 World::resolve(const AABB& startBox, glm::vec2 desiredDelta) const {
		// Move along X then Y, resolving penetration at each step
		glm::vec2 out = desiredDelta;

		// X
		AABB movedX = startBox;
		movedX.min.x += out.x; movedX.max.x += out.x;
		for (const auto& w : mWalls) {
			if (overlaps(movedX, w)) {
				if (out.x > 0.0f) { // moving right; push left
					float pen = movedX.max.x - w.min.x;
					out.x -= pen;
					movedX.min.x -= pen; movedX.max.x -= pen;
				}
				else if (out.x < 0.0f) { // moving left; push right
					float pen = w.max.x - movedX.min.x;
					out.x += pen;
					movedX.min.x += pen; movedX.max.x += pen;
				}
			}
		}

		// Y
		AABB movedY = movedX;
		movedY.min.y += out.y; movedY.max.y += out.y;
		for (const auto& w : mWalls) {
			if (overlaps(movedY, w)) {
				if (out.y > 0.0f) { // moving down; push up
					float pen = movedY.max.y - w.min.y;
					out.y -= pen;
					movedY.min.y -= pen; movedY.max.y -= pen;
				}
				else if (out.y < 0.0f) { // moving up; push down
					float pen = w.max.y - movedY.min.y;
					out.y += pen;
					movedY.min.y += pen; movedY.max.y += pen;
				}
			}
		}

		return out;
	}

	AABB World::makeAABBFromCenter(const glm::vec3& center, const glm::vec3& scale) {
		glm::vec2 he = { scale.x * 0.5f, scale.y * 0.5f };
		glm::vec2 c = { center.x, center.y };
		return { c - he, c + he };
	}
}
