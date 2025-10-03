/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Collision.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Provides Axis-Aligned Bounding Box (AABB) primitives and a lightweight
					collision world with support for static walls, walk areas, dividers, and gates.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace collision {
	/**
	 * @struct AABB
	 * @brief Axis-Aligned Bounding Box primitive.
	 *
	 * Represents a rectangular area aligned with the axes.
	 */
	struct AABB {
		glm::vec2 min;
		glm::vec2 max;
	};

	/**
	 * @struct WalkArea
	 * @brief Walkable rectangle with thin blocking edges.
	 *
	 * Defines the playable region and the thickness of its blocking walls.
	 */
	struct WalkArea {
		float L, R, T, B;
		float edgeThick;
	};

	/**
	 * @struct WoodVertical
	 * @brief Vertical divider composed of solid and open regions.
	 *
	 * Defines a vertical wooden divider with a top solid segment,
	 * a central gap (walkthrough), and a bottom solid segment.
	 */
	struct WoodVertical {
		float x0, x1;
		float topMinY, topMaxY;
		float gapMinY, gapMaxY;
		float botMinY, botMaxY;
	};

	/**
	 * @struct StageEndGateVertical
	 * @brief Vertical gate marking the end of a stage.
	 *
	 * Defines an end gate with top/bottom solid parts and a central open gap.
	 */
	struct StageEndGateVertical {
		float x0, x1;
		float topMinY, topMaxY;
		float gapMinY, gapMaxY;
		float botMinY, botMaxY;
	};

	/**
	 * @brief Compute minimal translation vector to separate overlapping AABBs.
	 *
	 * @param a First AABB.
	 * @param b Second AABB.
	 * @param mtvOut Output minimal translation vector for A.
	 * @return true if AABBs overlap, false otherwise.
	 */
	bool overlapMTV(const AABB& a, const AABB& b, glm::vec2& mtvOut);

	/**
	 * @brief Split the MTV between two AABBs based on weighting.
	 *
	 * @param a First AABB.
	 * @param b Second AABB.
	 * @param weightA Weight factor for A (0.5 = equal, 1.0 = A only).
	 * @param moveA Output movement for A.
	 * @param moveB Output movement for B.
	 * @return true if overlap occurred, false otherwise.
	 */
	bool separateWeighted(const AABB& a, const AABB& b, float weightA, glm::vec2& moveA, glm::vec2& moveB);

	/**
	 * @brief Check if a 2D point lies inside a center-based AABB.
	 *
	 * @param p The 2D point.
	 * @param center Center of the AABB.
	 * @param scale Full width/height of the AABB.
	 * @return true if inside, false otherwise.
	 */
	bool pointInsideCenterAABB(glm::vec2 p, glm::vec3 center, glm::vec3 scale);

	/**
	 * @class World
	 * @brief Represents a static collision world.
	 *
	 * Stores static walls built from walk areas, dividers, and gates, and provides
	 * collision resolution for moving AABBs.
	 */
	class World {
	public:
		World() = default;

		/** @brief Clear all walls from the world. */
		void clear();

		/**
		 * @brief Add a custom wall AABB.
		 * @param aabb Wall to add.
		 */
		void addWall(const AABB& aabb);

		/**
		 * @brief Build the collision world from primitives.
		 * @param walk Walkable area.
		 * @param wood Wooden divider.
		 * @param end End gate.
		 */
		void build(const WalkArea& walk, const WoodVertical& wood, const StageEndGateVertical& end);

		/**
		 * @brief Resolve desired motion against world walls.
		 * @param startBox Starting AABB.
		 * @param desiredDelta Desired translation.
		 * @return Adjusted translation that avoids penetration.
		 */
		glm::vec2 resolve(const AABB& startBox, glm::vec2 desiredDelta) const;

		/**
		 * @brief Construct an AABB from a center and size.
		 * @param center Object center (X/Y used).
		 * @param scale Full size (X/Y used).
		 * @return Constructed AABB.
		 */
		static AABB makeAABBFromCenter(const glm::vec3& center, const glm::vec3& scale);

		/**
		 * @brief Get read-only access to walls for debugging.
		 * @return Vector of AABBs representing walls.
		 */
		const std::vector<AABB>& walls() const { return mWalls; }

	private:
		std::vector<AABB> mWalls;			  // Internal wall list
		static constexpr float kSkin = 0.75f; // Small offset to prevent sticking
	};
}
