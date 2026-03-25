/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SpatialGrid.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Uniform 2D spatial hash grid for broad-phase queries.
					- Insert() associates a GameObject with the grid cells overlapped by its AABB.
					- Query() returns unique candidates overlapping the query AABB (+ 1-cell neighbors).
					- QueryPoint() returns candidates in the cell containing a point.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
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

	/**
	 * @brief Constructs a `SpatialGrid` instance.
	 * @param cellSize Parameter for cell size.
	 * @return Result produced by this operation.
	 */
	explicit SpatialGrid(float cellSize);

	/**
	 * @brief Clears this object.
	 */
	void Clear();

	/**
	 * @brief Performs cell size.
	 * @return Result produced by this operation.
	 */
	float CellSize() const;

	/**
	 * @brief Inserts this object.
	 * @param object Parameter for object.
	 * @param box Parameter for box.
	 */
	void Insert(GameObject* object, const collision::AABB& box);

	/**
	 * @brief Removes this object.
	 * @param object Parameter for object.
	 */
	void Remove(GameObject* object);

	/**
	 * @brief Updates this object.
	 * @param object Parameter for object.
	 * @param box Parameter for box.
	 */
	void Update(GameObject* object, const collision::AABB& box);

	/**
	 * @brief Performs query.
	 * @param box Parameter for box.
	 * @param outCandidates Output value for out candidates.
	 */
	void Query(const collision::AABB& box, std::vector<GameObject*>& outCandidates) const;

	/**
	 * @brief Performs query point.
	 * @param point Parameter for point.
	 * @param outCandidates Output value for out candidates.
	 */
	void QueryPoint(const Math::Vector2D& point, std::vector<GameObject*>& outCandidates) const;

	/**
	 * @brief Returns profile counters.
	 * @return Requested value.
	 */
	const ProfileCounters& GetProfileCounters() const;

	/**
	 * @brief Resets profile counters.
	 */
	void ResetProfileCounters();

private:

	/**
	 * @brief Collects cells.
	 * @param box Parameter for box.
	 * @param expandByCells Parameter for expand by cells.
	 * @param outKeys Output value for out keys.
	 */
	void CollectCells(const collision::AABB& box, int expandByCells, std::vector<Key>& outKeys) const;

	/**
	 * @brief Performs to key.
	 * @param cellX Parameter for cell x.
	 * @param cellY Parameter for cell y.
	 * @return Result produced by this operation.
	 */
	Key ToKey(int cellX, int cellY) const;

	/**
	 * @brief Returns whether same aabb.
	 * @param a Parameter for a.
	 * @param b Parameter for b.
	 * @return True when the operation succeeds or the condition is met.
	 */
	static bool IsSameAABB(const collision::AABB& a, const collision::AABB& b);

	/**
	 * @brief Performs reserve bucket if needed.
	 * @param bucket Parameter for bucket.
	 * @return Result produced by this operation.
	 */
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

