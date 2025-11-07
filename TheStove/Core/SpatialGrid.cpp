/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SpatialGrid.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Definitions for a uniform 2D spatial hash grid used for broad-phase queries.

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <cmath>
#include <algorithm>

#include "SpatialGrid.hpp"

 // Constructors / Clear
SpatialGrid::SpatialGrid(float cellSize)
	: cellSize(cellSize) {
}

void SpatialGrid::Clear() {
	cells.clear();
	objects.clear();
}

// TempVisited (uniqueness helper during queries)
bool SpatialGrid::TempVisited::Seen(GameObject* g) const {
	return marks.find(g) != marks.end();
}

void SpatialGrid::TempVisited::Mark(GameObject* g) {
	marks.insert(g);
}

// Internal helpers
SpatialGrid::Key SpatialGrid::ToKey(int cellX, int cellY) const {
	return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cellX)) << 32)
		| static_cast<std::uint64_t>(static_cast<std::uint32_t>(cellY));
}

void SpatialGrid::ForEachCell(const collision::AABB& box, const std::function<void(Key)>& visit) const {
	const int minCellX = static_cast<int>(std::floor(box.min.x / cellSize));
	const int maxCellX = static_cast<int>(std::floor(box.max.x / cellSize));
	const int minCellY = static_cast<int>(std::floor(box.min.y / cellSize));
	const int maxCellY = static_cast<int>(std::floor(box.max.y / cellSize));

	for (int cy = minCellY; cy <= maxCellY; ++cy) {
		for (int cx = minCellX; cx <= maxCellX; ++cx) {
			visit(ToKey(cx, cy));
		}
	}
}

void SpatialGrid::ForEachCellWithNeighbors(const collision::AABB& box, const std::function<void(Key)>& visit) const {
	const int minCellX = static_cast<int>(std::floor(box.min.x / cellSize)) - 1;
	const int maxCellX = static_cast<int>(std::floor(box.max.x / cellSize)) + 1;
	const int minCellY = static_cast<int>(std::floor(box.min.y / cellSize)) - 1;
	const int maxCellY = static_cast<int>(std::floor(box.max.y / cellSize)) + 1;

	for (int cy = minCellY; cy <= maxCellY; ++cy) {
		for (int cx = minCellX; cx <= maxCellX; ++cx) {
			visit(ToKey(cx, cy));
		}
	}
}

// Public Interface
float SpatialGrid::CellSize() const {
	return cellSize;
}

void SpatialGrid::Insert(GameObject* object, const collision::AABB& box) {
	if (object == nullptr) {
		return;
	}

	objects.push_back({ object, box });

	ForEachCell(box, [&](Key key) {
		cells[key].push_back(object);
		});
}

void SpatialGrid::Query(const collision::AABB& box, std::vector<GameObject*>& outCandidates) const {
	outCandidates.clear();
	TempVisited visited;

	ForEachCellWithNeighbors(box, [&](Key key) {
		auto it = cells.find(key);
		if (it == cells.end()) {
			return;
		}

		for (GameObject* obj : it->second) {
			if (obj == nullptr) {
				continue;
			}

			if (!visited.Seen(obj)) {
				visited.Mark(obj);
				outCandidates.push_back(obj);
			}
		}
		});
}

void SpatialGrid::QueryPoint(const Math::Vector2D& point, std::vector<GameObject*>& outCandidates) const {
	outCandidates.clear();

	const int cellX = static_cast<int>(std::floor(point.x / cellSize));
	const int cellY = static_cast<int>(std::floor(point.y / cellSize));
	const Key key = ToKey(cellX, cellY);

	auto it = cells.find(key);
	if (it != cells.end()) {
		outCandidates = it->second;
	}
}
