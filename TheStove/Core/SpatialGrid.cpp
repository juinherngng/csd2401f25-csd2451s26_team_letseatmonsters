/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SpatialGrid.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Definitions for a uniform 2D spatial hash grid used for broad-phase queries.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
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

	/**
	 * @brief Builds cell range.
	 * @param box Parameter for box.
	 * @param cellSize Parameter for cell size.
	 * @param expandByCells Parameter for expand by cells.
	 * @return Result produced by this operation.
	 */
	CellRange BuildCellRange(const collision::AABB& box, float cellSize, int expandByCells) {
		const int minCellX = static_cast<int>(std::floor(box.min.x / cellSize)) - expandByCells;
		const int maxCellX = static_cast<int>(std::floor(box.max.x / cellSize)) + expandByCells;
		const int minCellY = static_cast<int>(std::floor(box.min.y / cellSize)) - expandByCells;
		const int maxCellY = static_cast<int>(std::floor(box.max.y / cellSize)) + expandByCells;
		return { minCellX, maxCellX, minCellY, maxCellY };
	}

	/**
	 * @brief Performs erase object from bucket.
	 * @param bucket Parameter for bucket.
	 * @param object Parameter for object.
	 */
	void EraseObjectFromBucket(std::vector<GameObject*>& bucket, GameObject* object) {
		auto foundIt = std::find(bucket.begin(), bucket.end(), object);
		if (foundIt == bucket.end()) {
			return;
		}

		*foundIt = bucket.back();
		bucket.pop_back();
	}
}

/**
 * @brief Performs spatial grid.
 * @param cellSize Parameter for cell size.
 * @return Result produced by this operation.
 */
SpatialGrid::SpatialGrid(float cellSize)
	: cellSize(std::max(cellSize, kMinCellSize)) {
}

/**
 * @brief Clears this object.
 * @return Result produced by this operation.
 */
void SpatialGrid::Clear() {
	cells.clear();
	objectCells_.clear();
	objectAABBs_.clear();
	queryVisitStamp_.clear();
	queryCellCache_.clear();
	updateCellCache_.clear();
	queryStamp_ = 1;
}

/**
 * @brief Performs to key.
 * @param cellX Parameter for cell x.
 * @param cellY Parameter for cell y.
 * @return Result produced by this operation.
 */
SpatialGrid::Key SpatialGrid::ToKey(int cellX, int cellY) const {
	return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cellX)) << 32)
		| static_cast<std::uint64_t>(static_cast<std::uint32_t>(cellY));
}

/**
 * @brief Returns whether same aabb.
 * @param a Parameter for a.
 * @param b Parameter for b.
 * @return True when the operation succeeds or the condition is met.
 */
bool SpatialGrid::IsSameAABB(const collision::AABB& a, const collision::AABB& b) {
	return a.min.x == b.min.x && a.min.y == b.min.y && a.max.x == b.max.x && a.max.y == b.max.y;
}

/**
 * @brief Performs reserve bucket if needed.
 * @param bucket Parameter for bucket.
 * @return Result produced by this operation.
 */
void SpatialGrid::ReserveBucketIfNeeded(std::vector<GameObject*>& bucket) {
	if (bucket.size() == bucket.capacity()) {
		const size_t nextCapacity = bucket.capacity() > 0 ? bucket.capacity() * 2 : 4;
		bucket.reserve(nextCapacity);
	}
}

/**
 * @brief Collects cells.
 * @param box Parameter for box.
 * @param expandByCells Parameter for expand by cells.
 * @param outKeys Output value for out keys.
 * @return Result produced by this operation.
 */
void SpatialGrid::CollectCells(const collision::AABB& box, int expandByCells, std::vector<Key>& outKeys) const {
	outKeys.clear();

	const CellRange range = BuildCellRange(box, cellSize, expandByCells);
	const int width = (range.maxX - range.minX + 1);
	const int height = (range.maxY - range.minY + 1);
	outKeys.reserve(static_cast<size_t>(std::max(0, width * height)));

	for (int cx = range.minX; cx <= range.maxX; ++cx) {
		for (int cy = range.minY; cy <= range.maxY; ++cy) {
			outKeys.push_back(ToKey(cx, cy));
		}
	}
}

/**
 * @brief Performs cell size.
 * @return Result produced by this operation.
 */
float SpatialGrid::CellSize() const {
	return cellSize;
}

/**
 * @brief Inserts this object.
 * @param object Parameter for object.
 * @param box Parameter for box.
 * @return Result produced by this operation.
 */
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

/**
 * @brief Removes this object.
 * @param object Parameter for object.
 * @return Result produced by this operation.
 */
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
		EraseObjectFromBucket(bucket, object);
		if (bucket.empty()) {
			cells.erase(cellIt);
		}
	}

	objectCells_.erase(it);
	objectAABBs_.erase(object);
	queryVisitStamp_.erase(object);
}

/**
 * @brief Updates this object.
 * @param object Parameter for object.
 * @param box Parameter for box.
 * @return Result produced by this operation.
 */
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

	CollectCells(box, 0, updateCellCache_);

	auto existing = objectCells_.find(object);
	if (existing != objectCells_.end() && existing->second == updateCellCache_) {
		objectAABBs_[object] = box;
		++profile_.updateSkippedSameCells;
		return;
	}

	if (existing != objectCells_.end()) {
		const std::vector<Key>& previousCells = existing->second;

		size_t previousIndex = 0;
		size_t nextIndex = 0;
		while (previousIndex < previousCells.size() || nextIndex < updateCellCache_.size()) {
			if (nextIndex >= updateCellCache_.size() ||
				(previousIndex < previousCells.size() && previousCells[previousIndex] < updateCellCache_[nextIndex])) {
				auto oldCellIt = cells.find(previousCells[previousIndex]);
				if (oldCellIt != cells.end()) {
					auto& bucket = oldCellIt->second;
					EraseObjectFromBucket(bucket, object);
					if (bucket.empty()) {
						cells.erase(oldCellIt);
					}
				}
				++previousIndex;
			}
			else if (previousIndex >= previousCells.size() || updateCellCache_[nextIndex] < previousCells[previousIndex]) {
				auto& bucket = cells[updateCellCache_[nextIndex]];
				ReserveBucketIfNeeded(bucket);
				bucket.push_back(object);
				++nextIndex;
			}
			else {
				++previousIndex;
				++nextIndex;
			}
		}

		existing->second = updateCellCache_;
		objectAABBs_[object] = box;
		return;
	}

	auto& insertedCells = objectCells_[object];
	insertedCells = updateCellCache_;
	objectAABBs_[object] = box;

	for (const Key key : insertedCells) {
		auto& bucket = cells[key];
		ReserveBucketIfNeeded(bucket);
		bucket.push_back(object);
	}
}

/**
 * @brief Performs query.
 * @param box Parameter for box.
 * @param outCandidates Output value for out candidates.
 * @return Result produced by this operation.
 */
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

/**
 * @brief Performs query point.
 * @param point Parameter for point.
 * @param outCandidates Output value for out candidates.
 * @return Result produced by this operation.
 */
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

/**
 * @brief Returns profile counters.
 * @return Requested value.
 */
const SpatialGrid::ProfileCounters& SpatialGrid::GetProfileCounters() const {
	return profile_;
}

/**
 * @brief Resets profile counters.
 * @return Result produced by this operation.
 */
void SpatialGrid::ResetProfileCounters() {
	profile_ = ProfileCounters{};
}

