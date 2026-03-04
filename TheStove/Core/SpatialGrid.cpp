/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SpatialGrid.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Definitions for a uniform 2D spatial hash grid used for broad-phase queries.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "SpatialGrid.hpp"

#include <algorithm>
#include <cmath>

 // Minimum cell size to prevent excessive memory usage or precision issues
namespace {
	constexpr float kMinCellSize = 1.0f;

	struct CellRange {
		int minX;
		int maxX;
		int minY;
		int maxY;
	};

	CellRange BuildCellRange(const collision::AABB& box, float cellSize, int expandByCells) {
		const int minCellX = static_cast<int>(std::floor(box.min.x / cellSize)) - expandByCells;
		const int maxCellX = static_cast<int>(std::floor(box.max.x / cellSize)) + expandByCells;
		const int minCellY = static_cast<int>(std::floor(box.min.y / cellSize)) - expandByCells;
		const int maxCellY = static_cast<int>(std::floor(box.max.y / cellSize)) + expandByCells;
		return { minCellX, maxCellX, minCellY, maxCellY };
	}
}

// Constructors / Initialization
SpatialGrid::SpatialGrid(float cellSize)
	: cellSize(std::max(cellSize, kMinCellSize)) {
}
void SpatialGrid::Clear() {
	cells.clear();
	objectCells_.clear();
}

// TempVisited (uniqueness helper during queries)
bool SpatialGrid::TempVisited::Seen(GameObject* g) const {
	return marks.find(g) != marks.end();
}
void SpatialGrid::TempVisited::Mark(GameObject* g) {
	marks.insert(g);
}

// Convert cell coordinates to a unique 64-bit key (using 32 bits for each coordinate).
SpatialGrid::Key SpatialGrid::ToKey(int cellX, int cellY) const {
	return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cellX)) << 32)
		| static_cast<std::uint64_t>(static_cast<std::uint32_t>(cellY));
}

// Visit each cell overlapped by the box, calling the provided function with the cell's key.
void SpatialGrid::ForEachCell(const collision::AABB& box, const std::function<void(Key)>& visit) const {
	const CellRange range = BuildCellRange(box, cellSize, 0);

	for (int cy = range.minY; cy <= range.maxY; ++cy) {
		for (int cx = range.minX; cx <= range.maxX; ++cx) {
			visit(ToKey(cx, cy));
		}
	}
}

// Visit each cell overlapped by the box and its immediate neighbors (1-cell expansion), calling the provided function with the cell's key.
void SpatialGrid::ForEachCellWithNeighbors(const collision::AABB& box, const std::function<void(Key)>& visit) const {
	const CellRange range = BuildCellRange(box, cellSize, 1);

	for (int cy = range.minY; cy <= range.maxY; ++cy) {
		for (int cx = range.minX; cx <= range.maxX; ++cx) {
			visit(ToKey(cx, cy));
		}
	}
}

// Public Interface
float SpatialGrid::CellSize() const {
	return cellSize;
}

// Insert an object into the grid, associating it with all cells overlapped by its AABB. Also store the object and its AABB for potential future use (e.g., clearing).
void SpatialGrid::Insert(GameObject* object, const collision::AABB& box) {
	if (object == nullptr) {
		return;
	}

	auto& touchedCells = objectCells_[object];
	touchedCells.clear();

	ForEachCell(box, [&](Key key) {
		cells[key].push_back(object);
		touchedCells.push_back(key);
		});

}

// Remove an object from the grid, disassociating it from all cells it was previously associated with.
void SpatialGrid::Remove(GameObject* object) {
	if (object == nullptr) {
		return;
	}

	auto it = objectCells_.find(object);
	if (it == objectCells_.end()) {
		return;
	}

	for (Key key : it->second) {
		auto cellIt = cells.find(key);
		if (cellIt == cells.end()) {
			continue;
		}
		auto& bucket = cellIt->second;
		bucket.erase(std::remove(bucket.begin(), bucket.end(), object), bucket.end());
		if (bucket.empty()) {
			cells.erase(cellIt);
		}
	}

	objectCells_.erase(it);
}

// Update an object's position in the grid by first removing it from its old cells and then inserting it into the new cells based on the updated AABB.
void SpatialGrid::Update(GameObject* object, const collision::AABB& box) {
	if (object == nullptr) {
		return;
	}
	Remove(object);
	Insert(object, box);
}

// Query for candidates overlapping the box (including 1-cell neighbors). Uses TempVisited to ensure each object is only returned once, even if it appears in multiple cells.
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

// Query for candidates in the cell containing the point. This is a simpler query that does not consider neighbors and does not require uniqueness checks since it's only one cell.
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
