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
#include <unordered_map>
#include <vector>

class SpatialGrid {
public:
	using Key = std::uint64_t;

	// Profiling counters for performance analysis
	struct ProfileCounters {
		std::uint64_t queryCalls = 0;
		std::uint64_t queryCellsVisited = 0;
		std::uint64_t queryCandidatesReturned = 0;
		std::uint64_t updateCalls = 0;
		std::uint64_t updateSkippedSameAABB = 0;
		std::uint64_t updateSkippedSameCells = 0;
	};

	// Public interface
	explicit SpatialGrid(float cellSize);
	void Clear();

	// Getters/setters
	float CellSize() const;

	// Insert or update an object with its current AABB. Automatically handles cell associations.
	void Insert(GameObject* object, const collision::AABB& box);
	void Remove(GameObject* object);
	void Update(GameObject* object, const collision::AABB& box);

	// Query grid for objects overlapping an AABB (including 1-cell neighbors for broader queries). Returns unique candidates.
	void Query(const collision::AABB& box, std::vector<GameObject*>& outCandidates) const;
	void QueryPoint(const Math::Vector2D& point, std::vector<GameObject*>& outCandidates) const;

	// Access profiling counters for performance analysis
	const ProfileCounters& GetProfileCounters() const;
	void ResetProfileCounters();

private:
	// Helper methods
	void CollectCells(const collision::AABB& box, int expandByCells, std::vector<Key>& outKeys) const;
	Key ToKey(int cellX, int cellY) const;
	static bool IsSameAABB(const collision::AABB& a, const collision::AABB& b);
	static void ReserveBucketIfNeeded(std::vector<GameObject*>& bucket);

	// Internal state
	float cellSize;
	std::unordered_map<Key, std::vector<GameObject*>> cells;
	std::unordered_map<GameObject*, std::vector<Key>> objectCells_;
	std::unordered_map<GameObject*, collision::AABB> objectAABBs_;
	mutable std::unordered_map<GameObject*, std::uint32_t> queryVisitStamp_;
	mutable std::vector<Key> queryCellCache_;
	std::vector<Key> updateCellCache_;
	mutable std::uint32_t queryStamp_ = 1;
	mutable ProfileCounters profile_;
};
