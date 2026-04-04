/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GridPathfinder.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (90%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (10%)

 DESCRIPTION:		Implements the NavGrid and GridPathfinder classes, which provide grid-based pathfinding
					functionality using A* search. NavGrid represents a 2D grid of walkable/blocked cells,
					while GridPathfinder contains the A* algorithm to find a path between two points on the grid.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <vector>

#include "EngineCore/GridPathfinder.hpp"

// Finds the nearest walkable cell to the given grid coordinates, searching in a spiral pattern. Returns true if a walkable cell is found and sets outCell to its coordinates.
bool NavGrid::FindNearestWalkable(const GridCoord& from, GridCoord& outCell) const {
	if (!IsValid()) return false;

	GridCoord start = from;
	if (start.x < 0) start.x = 0;
	if (start.y < 0) start.y = 0;
	if (start.x >= width_)  start.x = width_ - 1;
	if (start.y >= height_) start.y = height_ - 1;

	if (IsWalkable(start.x, start.y)) {
		outCell = start;
		return true;
	}

	std::queue<GridCoord> q;
	std::vector<unsigned char> visited(width_ * height_, 0);

	q.push(start);
	visited[ToIndex(start.x, start.y)] = 1;

	static const int kDirs[4][2] = {
		{ 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }
	};

	while (!q.empty()) {
		GridCoord cur = q.front();
		q.pop();

		for (const auto& d : kDirs) {
			GridCoord next{ cur.x + d[0], cur.y + d[1] };
			if (!InBounds(next.x, next.y)) continue;

			const int idx = ToIndex(next.x, next.y);
			if (visited[idx]) continue;
			visited[idx] = 1;

			if (IsWalkable(next.x, next.y)) {
				outCell = next;
				return true;
			}

			q.push(next);
		}
	}

	return false;
}

// Finds a path from start to goal on the given NavGrid using A* search. Returns true if a path is found and sets outPath to the sequence of grid coordinates from start to goal.
bool GridPathfinder::FindPath(const NavGrid& grid, const GridCoord& start, const GridCoord& goal, std::vector<GridCoord>& outPath) {
	outPath.clear();

	if (!grid.IsValid()) return false;
	if (!grid.IsWalkable(start.x, start.y)) return false;
	if (!grid.IsWalkable(goal.x, goal.y)) return false;

	const int width = grid.Width();
	const int height = grid.Height();
	const int total = width * height;

	const int startIdx = grid.ToIndex(start.x, start.y);
	const int goalIdx = grid.ToIndex(goal.x, goal.y);

	if (startIdx == goalIdx) {
		outPath.push_back(start);
		return true;
	}

	// Base movement cost
	static constexpr int kBaseMoveCost = 10;

	// Extra penalty for moving vertically BEFORE we are aligned on X with the goal.
	// Increase this if you want even stronger "move horizontally first" behaviour.
	static constexpr int kVerticalPenaltyWhenXMisaligned = 30;

	auto heuristic = [&](int x, int y) -> int {
		// Admissible heuristic: minimum possible cost per step is kBaseMoveCost
		return (std::abs(x - goal.x) + std::abs(y - goal.y)) * kBaseMoveCost;
		};

	struct Node {
		int idx = -1;
		int f = 0;
		int h = 0;
		int order = 0; // preserves insertion order for stable tie-breaking
	};

	struct NodeGreater {
		bool operator()(const Node& a, const Node& b) const {
			if (a.f != b.f) return a.f > b.f;
			if (a.h != b.h) return a.h > b.h;
			return a.order > b.order;
		}
	};

	std::priority_queue<Node, std::vector<Node>, NodeGreater> open;
	std::vector<int> gCost(total, std::numeric_limits<int>::max());
	std::vector<int> parent(total, -1);
	std::vector<unsigned char> closed(total, 0);

	int pushOrder = 0;

	gCost[startIdx] = 0;
	const int startH = heuristic(start.x, start.y);
	open.push({ startIdx, startH, startH, pushOrder++ });

	while (!open.empty()) {
		Node curNode = open.top();
		open.pop();

		const int curIdx = curNode.idx;
		if (closed[curIdx]) continue;
		closed[curIdx] = 1;

		if (curIdx == goalIdx) break;

		const int cx = curIdx % width;
		const int cy = curIdx / width;

		// Build neighbour order dynamically:
		// Prefer horizontal movement first, then vertical movement.
		int dirs[4][2];
		int dirCount = 0;

		auto pushDir = [&](int dx, int dy) {
			dirs[dirCount][0] = dx;
			dirs[dirCount][1] = dy;
			++dirCount;
			};

		// Horizontal preference first
		if (goal.x > cx) {
			pushDir(1, 0);   // move right toward goal first
			pushDir(-1, 0);  // then left
		}
		else if (goal.x < cx) {
			pushDir(-1, 0);  // move left toward goal first
			pushDir(1, 0);   // then right
		}
		else {
			// already aligned on X, either horizontal direction is neutral
			pushDir(1, 0);
			pushDir(-1, 0);
		}

		// Vertical second
		if (goal.y > cy) {
			pushDir(0, 1);   // move down toward goal first (depending on your grid/world convention)
			pushDir(0, -1);  // then up
		}
		else if (goal.y < cy) {
			pushDir(0, -1);  // move up toward goal first
			pushDir(0, 1);   // then down
		}
		else {
			pushDir(0, 1);
			pushDir(0, -1);
		}

		for (int i = 0; i < 4; ++i) {
			const int dx = dirs[i][0];
			const int dy = dirs[i][1];

			const int nx = cx + dx;
			const int ny = cy + dy;

			if (!grid.IsWalkable(nx, ny)) continue;

			const int nextIdx = grid.ToIndex(nx, ny);
			if (closed[nextIdx]) continue;

			int stepCost = kBaseMoveCost;

			// Strong bias:
			// if we are not yet aligned with the goal on X,
			// vertical moves are more expensive than horizontal moves.
			const bool isVerticalMove = (dy != 0);
			if (isVerticalMove && cx != goal.x) {
				stepCost += kVerticalPenaltyWhenXMisaligned;
			}

			const int tentativeG = gCost[curIdx] + stepCost;
			if (tentativeG < gCost[nextIdx]) {
				gCost[nextIdx] = tentativeG;
				parent[nextIdx] = curIdx;

				const int h = heuristic(nx, ny);
				const int f = tentativeG + h;
				open.push({ nextIdx, f, h, pushOrder++ });
			}
		}
	}

	if (parent[goalIdx] == -1) {
		return false;
	}

	std::vector<int> reversed;
	for (int cur = goalIdx; cur != -1; cur = parent[cur]) {
		reversed.push_back(cur);
	}

	std::reverse(reversed.begin(), reversed.end());

	outPath.reserve(reversed.size());
	for (int idx : reversed) {
		GridCoord c;
		c.x = idx % width;
		c.y = idx / width;
		outPath.push_back(c);
	}

	return !outPath.empty();
}
