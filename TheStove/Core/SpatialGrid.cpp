#include "SpatialGrid.hpp"
#include <cmath>
#include <algorithm>

SpatialGrid::SpatialGrid(float cellSize)
	: m_cellSize(cellSize) {
}

void SpatialGrid::Clear() {
	m_cells.clear();
	m_objects.clear();
}

SpatialGrid::Key SpatialGrid::ToKey(int cx, int cy) const {
	return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cx)) << 32) | static_cast<std::uint64_t>(static_cast<std::uint32_t>(cy));
}

void SpatialGrid::ForEachCell(const collision::AABB& b, const std::function<void(Key)>& fn) const {
	const int minCx = static_cast<int>(std::floor(b.min.x / m_cellSize));
	const int maxCx = static_cast<int>(std::floor(b.max.x / m_cellSize));
	const int minCy = static_cast<int>(std::floor(b.min.y / m_cellSize));
	const int maxCy = static_cast<int>(std::floor(b.max.y / m_cellSize));

	for (int cy = minCy; cy <= maxCy; ++cy) {
		for (int cx = minCx; cx <= maxCx; ++cx) {
			fn(ToKey(cx, cy));
		}
	}
}

void SpatialGrid::ForEachCellWithNeighbors(const collision::AABB& b, const std::function<void(Key)>& fn) const {
	const int minCx = static_cast<int>(std::floor(b.min.x / m_cellSize)) - 1;
	const int maxCx = static_cast<int>(std::floor(b.max.x / m_cellSize)) + 1;
	const int minCy = static_cast<int>(std::floor(b.min.y / m_cellSize)) - 1;
	const int maxCy = static_cast<int>(std::floor(b.max.y / m_cellSize)) + 1;

	for (int cy = minCy; cy <= maxCy; ++cy) {
		for (int cx = minCx; cx <= maxCx; ++cx) {
			fn(ToKey(cx, cy));
		}
	}
}

void SpatialGrid::Insert(GameObject* obj, const collision::AABB& box) {
	if (!obj) {
		return;
	}

	m_objects.push_back({ obj, box });

	ForEachCell(box, [&](Key k) {
		m_cells[k].push_back(obj);
		});
}

void SpatialGrid::Query(const collision::AABB& box, std::vector<GameObject*>& outCandidates) const {
	outCandidates.clear();
	TempVisited visited;

	ForEachCellWithNeighbors(box, [&](Key k) {
		auto it = m_cells.find(k);
		if (it == m_cells.end()) {
			return;
		}

		for (GameObject* g : it->second) {
			if (!g) {
				continue;
			}

			if (!visited.Seen(g)) {
				visited.Mark(g);
				outCandidates.push_back(g);
			}
		}
		});
}

void SpatialGrid::QueryPoint(const Math::Vector2D& p, std::vector<GameObject*>& outCandidates) const {
	outCandidates.clear();
	const int cx = static_cast<int>(std::floor(p.x / m_cellSize));
	const int cy = static_cast<int>(std::floor(p.y / m_cellSize));
	const Key k = ToKey(cx, cy);

	auto it = m_cells.find(k);
	if (it != m_cells.end()) {
		outCandidates = it->second;
	}
}
