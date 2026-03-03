/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SpatialGrid.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Uniform 2D spatial hash grid for broad-phase queries.
					- Insert() associates a GameObject with the grid cells overlapped by its AABB.
					- Query() returns unique candidates overlapping the query AABB (+ 1-cell neighbors).
					- QueryPoint() returns candidates in the cell containing a point.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "../Graphics/GameObject.hpp"

#include "Collision.hpp"
#include "Math.hpp"

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

 /**
  * @brief Spatial hash grid for broad-phase collision / picking.
  */
class SpatialGrid {
public:
	using Key = std::uint64_t;

	// Constructors / Initialization
	explicit SpatialGrid(float cellSize);
	void Clear();

	// Public Interface
	float CellSize() const;

	// Object management
	void Insert(GameObject* object, const collision::AABB& box);

	// Queries
	void Query(const collision::AABB& box, std::vector<GameObject*>& outCandidates) const;
	void QueryPoint(const Math::Vector2D& point, std::vector<GameObject*>& outCandidates) const;

private:
	// Internal Helpers
	struct TempVisited {
		bool Seen(GameObject* g) const;
		void Mark(GameObject* g);
		std::unordered_set<GameObject*> marks;
	};

	Key ToKey(int cellX, int cellY) const;

	void ForEachCell(const collision::AABB& box, const std::function<void(Key)>& visit) const;
	void ForEachCellWithNeighbors(const collision::AABB& box, const std::function<void(Key)>& visit) const;

	// Data Members
	float cellSize;
	std::unordered_map<Key, std::vector<GameObject*>> cells;
	std::vector<std::pair<GameObject*, collision::AABB>> objects;
};
