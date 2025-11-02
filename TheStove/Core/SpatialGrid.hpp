/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SpatialGrid.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Spatial hash grid for broad-phase collision queries.
					Stores cells keyed by packed (cx, cy) and supports box/point queries.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "Math.hpp"
#include "Collision.hpp"
#include "../Graphics/GameObject.hpp"

 // Broad-phase uniform grid for 2D AABBs.
class SpatialGrid {
public:
	using Key = std::uint64_t;

	explicit SpatialGrid(float cellSize);

	void Clear();

	void Insert(GameObject* object, const collision::AABB& box);

	void Query(const collision::AABB& box, std::vector<GameObject*>& outCandidates) const;

	void QueryPoint(const Math::Vector2D& point, std::vector<GameObject*>& outCandidates) const;

	float CellSize() const;

private:
	Key ToKey(int cellX, int cellY) const;

	void ForEachCell(const collision::AABB& box, const std::function<void(Key)>& visit) const;
	void ForEachCellWithNeighbors(const collision::AABB& box, const std::function<void(Key)>& visit) const;

	struct TempVisited {
		bool Seen(GameObject* g) const;
		void Mark(GameObject* g);
		std::unordered_set<GameObject*> marks;
	};

	struct Tracked {
		GameObject* object{ nullptr };
		collision::AABB  bounds{};
	};

	float cellSize{ 128.0f };

	std::unordered_map<Key, std::vector<GameObject*>> cells;
	std::vector<Tracked> objects;
};
