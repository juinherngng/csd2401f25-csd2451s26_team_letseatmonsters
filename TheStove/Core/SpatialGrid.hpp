#pragma once

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <functional>
#include "Math.hpp"
#include "Collision.hpp"
#include "../Graphics/GameObject.hpp"

// Broad-phase uniform grid for 2D AABBs.
class SpatialGrid {
public:
	explicit SpatialGrid(float cellSize = 128.0f);

	void Clear();

	// Register/update an object's current AABB in the grid (call once per frame).
	void Insert(GameObject* obj, const collision::AABB& box);

	// Return unique candidate objects overlapping the 1-cell neighborhood of 'box'.
	void Query(const collision::AABB& box, std::vector<GameObject*>& outCandidates) const;

	// Return objects in the cell containing point p.
	void QueryPoint(const Math::Vector2D& p, std::vector<GameObject*>& outCandidates) const;

	float CellSize() const { return m_cellSize; }

private:
	using Key = std::uint64_t;

	struct ObjRec { GameObject* obj; collision::AABB box; };

	// fast “seen” set for dedupe during Query()
	struct PtrHasher {
		size_t operator()(const void* p) const noexcept {
			return std::hash<std::uintptr_t>{}(reinterpret_cast<std::uintptr_t>(p));
		}
	};
	struct TempVisited {
		std::unordered_map<GameObject*, bool, PtrHasher> seen;
		bool Seen(GameObject* g) const { return seen.find(g) != seen.end(); }
		void Mark(GameObject* g) { seen[g] = true; }
	};

	// data
	float m_cellSize;
	std::unordered_map<Key, std::vector<GameObject*>> m_cells;
	std::vector<ObjRec> m_objects;

	// helpers
	Key ToKey(int cx, int cy) const;
	void ForEachCell(const collision::AABB& b, const std::function<void(Key)>& fn) const;
	void ForEachCellWithNeighbors(const collision::AABB& b, const std::function<void(Key)>& fn) const;
};
