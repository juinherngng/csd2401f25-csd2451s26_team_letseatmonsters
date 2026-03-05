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

SpatialGrid::SpatialGrid(float cellSize)
	: cellSize(std::max(cellSize, kMinCellSize)) {
}

void SpatialGrid::Clear() {
	cells.clear();
	objectCells_.clear();
	objectAABBs_.clear();
	queryVisitStamp_.clear();
	queryCellCache_.clear();
	queryStamp_ = 1;
}

SpatialGrid::Key SpatialGrid::ToKey(int cellX, int cellY) const {
	return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cellX)) << 32)
		| static_cast<std::uint64_t>(static_cast<std::uint32_t>(cellY));
}

// Visit each cell overlapped by the box, calling the provided function with the cell's key.
bool SpatialGrid::IsSameAABB(const collision::AABB& a, const collision::AABB& b) {
	return a.min.x == b.min.x && a.min.y == b.min.y && a.max.x == b.max.x && a.max.y == b.max.y;
}

// Visit each cell overlapped by the box, calling the provided function with the cell's key. Does not track visited objects.
void SpatialGrid::ReserveBucketIfNeeded(std::vector<GameObject*>& bucket) {
	if (bucket.size() == bucket.capacity()) {
		const size_t nextCapacity = bucket.capacity() > 0 ? bucket.capacity() * 2 : 4;
		bucket.reserve(nextCapacity);
	}
}

// Visit each cell overlapped by the box, calling the provided function with the cell's key. Also visits 1-cell neighbors for broader queries.
void SpatialGrid::CollectCells(const collision::AABB& box, int expandByCells, std::vector<Key>& outKeys) const {
	outKeys.clear();

	const CellRange range = BuildCellRange(box, cellSize, expandByCells);
	const int width = (range.maxX - range.minX + 1);
	const int height = (range.maxY - range.minY + 1);
	outKeys.reserve(static_cast<size_t>(std::max(0, width * height)));

	for (int cy = range.minY; cy <= range.maxY; ++cy) {
		for (int cx = range.minX; cx <= range.maxX; ++cx) {
			outKeys.push_back(ToKey(cx, cy));
		}
	}
}

float SpatialGrid::CellSize() const {
	return cellSize;
}

void SpatialGrid::Insert(GameObject* object, const collision::AABB& box) {
	if (object == nullptr) {
		return;
	}

	auto& touchedCells = objectCells_[object];
	CollectCells(box, 0, touchedCells);
	objectAABBs_[object] = box;

	for (const Key key : touchedCells) {
		auto& bucket = cells[key];
		ReserveBucketIfNeeded(bucket);
		bucket.push_back(object);
	}
}

void SpatialGrid::Remove(GameObject* object) {
	if (object == nullptr) {
		return;
	}

	auto it = objectCells_.find(object);
	if (it == objectCells_.end()) {
		return;
	}

	for (const Key key : it->second) {
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
	objectAABBs_.erase(object);
	queryVisitStamp_.erase(object);
}

// Update an object's position in the grid by first removing it from its old cells and then inserting it into the new cells based on the updated AABB.
void SpatialGrid::Update(GameObject* object, const collision::AABB& box) {
	if (object == nullptr) {
		return;
	}

	++profile_.updateCalls;

	auto aabbIt = objectAABBs_.find(object);
	if (aabbIt != objectAABBs_.end() && IsSameAABB(aabbIt->second, box)) {
		++profile_.updateSkippedSameAABB;
		return;
	}

	std::vector<Key> nextCells;
	CollectCells(box, 0, nextCells);

	auto existing = objectCells_.find(object);
	if (existing != objectCells_.end() && existing->second == nextCells) {
		existing->second = std::move(nextCells);
		objectAABBs_[object] = box;
		++profile_.updateSkippedSameCells;
		return;
	}

	Remove(object);
	auto& touchedCells = objectCells_[object];
	touchedCells = std::move(nextCells);
	objectAABBs_[object] = box;

	for (const Key key : touchedCells) {
		auto& bucket = cells[key];
		ReserveBucketIfNeeded(bucket);
		bucket.push_back(object);
	}
}

void SpatialGrid::Query(const collision::AABB& box, std::vector<GameObject*>& outCandidates) const {
	outCandidates.clear();
	++profile_.queryCalls;

	if (++queryStamp_ == 0) {
		queryStamp_ = 1;
		queryVisitStamp_.clear();
	}

	CollectCells(box, 1, queryCellCache_);
	profile_.queryCellsVisited += queryCellCache_.size();

	for (const Key key : queryCellCache_) {
		auto it = cells.find(key);
		if (it == cells.end()) {
			continue;
		}

		for (GameObject* obj : it->second) {
			if (obj == nullptr) {
				continue;
			}

			auto [markIt, inserted] = queryVisitStamp_.emplace(obj, queryStamp_);
			if (!inserted && markIt->second == queryStamp_) {
				continue;
			}

			markIt->second = queryStamp_;
			outCandidates.push_back(obj);
		}
	}

	profile_.queryCandidatesReturned += outCandidates.size();
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

const SpatialGrid::ProfileCounters& SpatialGrid::GetProfileCounters() const {
	return profile_;
}

void SpatialGrid::ResetProfileCounters() {
	profile_ = ProfileCounters{};
}
