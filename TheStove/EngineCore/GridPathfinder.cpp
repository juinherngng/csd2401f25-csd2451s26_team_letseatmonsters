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

/**
 * @brief Finds the nearest walkable cell to a starting coordinate using breadth-first search.
 * @param from Starting grid coordinate.
 * @param outCell Output walkable coordinate if one is found.
 * @return True if a walkable cell was found, otherwise false.
 */
bool NavGrid::FindNearestWalkable(const GridCoord& from, GridCoord& outCell) const {
	if (!IsValid()) return false;

	// Clamp the starting position into the grid so searches can begin from out-of-range requests.
	GridCoord start = from;
	if (start.x < 0) start.x = 0;
	if (start.y < 0) start.y = 0;
	if (start.x >= width_)  start.x = width_ - 1;
	if (start.y >= height_) start.y = height_ - 1;

	if (IsWalkable(start.x, start.y)) {
		outCell = start;
		return true;
	}

	// Search outward from the start cell until the first walkable location is discovered.
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
			// Expand outward in four directions because navigation is axis-aligned.
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

/**
 * @brief Finds a path from start to goal using A* search on the supplied navigation grid.
 * @param grid Navigation grid to search on.
 * @param start Start cell.
 * @param goal Goal cell.
 * @param outPath Output path from start to goal.
 * @return True if a path was found, otherwise false.
 */
bool GridPathfinder::FindPath(const NavGrid& grid, const GridCoord& start, const GridCoord& goal, std::vector<GridCoord>& outPath) {
	outPath.clear();

	// Reject impossible searches up front before any pathfinding work begins.
	if (!grid.IsValid()) return false;
	if (!grid.IsWalkable(start.x, start.y)) return false;
	if (!grid.IsWalkable(goal.x, goal.y)) return false;

	const int width = grid.Width();
	const int height = grid.Height();
	const int total = width * height;

	const int startIdx = grid.ToIndex(start.x, start.y);
	const int goalIdx = grid.ToIndex(goal.x, goal.y);

	if (startIdx == goalIdx) {
		// Degenerate case: the caller is already standing on the goal cell.
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
	// Seed the open list with the start node.
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

		// Build neighbour order dynamically so ties prefer horizontal movement before vertical movement.
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

			// Bias vertical movement upward in cost until horizontal alignment with the goal is achieved.
			const bool isVerticalMove = (dy != 0);
			if (isVerticalMove && cx != goal.x) {
				stepCost += kVerticalPenaltyWhenXMisaligned;
			}

			const int tentativeG = gCost[curIdx] + stepCost;
			if (tentativeG < gCost[nextIdx]) {
				// Record the improved route and push the neighbor back into the open set.
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
		// Reconstruct the path backwards from the goal using the parent chain.
		reversed.push_back(cur);
	}

	std::reverse(reversed.begin(), reversed.end());

	outPath.reserve(reversed.size());
	for (int idx : reversed) {
		// Convert flattened indices back into 2D grid coordinates for the caller.
		GridCoord c;
		c.x = idx % width;
		c.y = idx / width;
		outPath.push_back(c);
	}

	return !outPath.empty();
}
