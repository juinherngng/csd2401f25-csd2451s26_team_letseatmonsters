/**
 * @file   Collision.h
 * @author Yat Chun Wee, y.chunwee
 * @date   30 Sep 2025
 * @brief  Axis-aligned bounding box (AABB) primitives and a lightweight collision world.
 *
 * @details
 * Provides:
 *  - `AABB` primitive for overlap tests
 *  - Level geometry descriptors (`WalkArea`, `WoodVertical`, `StageEndGateVertical`)
 *  - `World` to store wall colliders and resolve swept movement
 */

#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace collision {
	/**
	 * @struct AABB
	 * @brief 2D axis-aligned bounding box used for collision tests.
	 *
	 * The box spans [min, max) on each axis in world units.
	 */
	struct AABB {
		glm::vec2 min;
		glm::vec2 max;
	};

	/**
	 * @struct WalkArea
	 * @brief Defines the inner walkable rectangle and the thickness of blocking edge bars.
	 */
	struct WalkArea {
		float L, R, T, B; // Left, Right, Top, Bottom of the light-gray play area.
		float edgeThick;  // Thickness of the thin blocking bars placed just inside edges.
	};

	/**
	 * @struct WoodVertical
	 * @brief Vertical wooden divider split into TOP solid, GAP (pass-through), BOTTOM solid.
	 */
	struct WoodVertical {
		float x0, x1;           // Left/right x of the wood.
		float topMinY, topMaxY; // Top solid segment Y-range.
		float gapMinY, gapMaxY; // Open gap Y-range (no collider).
		float botMinY, botMaxY; // Bottom solid segment Y-range.
	};

	/**
	 * @struct StageEndGateVertical
	 * @brief End-of-stage gate (vertical) with TOP solid, middle GAP, and BOTTOM solid.
	 */
	struct StageEndGateVertical {
		float x0, x1;           // Left/right x of the gate.
		float topMinY, topMaxY; // Top solid segment Y-range.
		float gapMinY, gapMaxY; // Open gap Y-range (no collider).
		float botMinY, botMaxY; // Bottom solid segment Y-range.
	};

	/**
	 * @class World
	 * @brief Stores wall AABBs and resolves swept movement against them.
	 */
	class World {
	public:
		World() = default;

		void clear();
		void addWall(const AABB& aabb);
		void build(const WalkArea& walk, const WoodVertical& wood, const StageEndGateVertical& end);

		// Resolve a desired movement vector against walls using axis-separable sweep.
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
