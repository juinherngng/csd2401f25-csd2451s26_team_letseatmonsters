/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GridPathfinder.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)
 DESCRIPTION:		Declares the NavGrid and GridPathfinder classes, which provide a grid-based navigation system
					for pathfinding in the game world. NavGrid represents a 2D grid of walkable and blocked cells,
					while GridPathfinder implements A* search to find paths between grid coordinates.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glm/vec2.hpp>
#include <vector>

// Simple struct to represent grid coordinates (x, y) and support equality comparison.
struct GridCoord {
	int x = 0;
	int y = 0;

	bool operator==(const GridCoord& rhs) const {
		return x == rhs.x && y == rhs.y;
	}
};

class NavGrid {
public:
	NavGrid() = default;

	// Constructor that initializes the grid with given parameters.
	NavGrid(float originX, float originY, float cellSize, int width, int height) {
		Reset(originX, originY, cellSize, width, height);
	}

	// Resets the grid with new parameters and clears blocked cells.
	void Reset(float originX, float originY, float cellSize, int width, int height) {
		originX_ = originX;
		originY_ = originY;
		cellSize_ = cellSize;
		width_ = width;
		height_ = height;
		blocked_.assign(width_ * height_, 0);
	}

	// Checks if the grid is valid (positive dimensions and cell size).
	bool IsValid() const {
		return width_ > 0 && height_ > 0 && cellSize_ > 0.0f;
	}

	// Accessors for grid properties.
	int Width() const {
		return width_;
	}
	int Height() const {
		return height_;
	}
	float CellSize() const {
		return cellSize_;
	}

	// Checks if the given grid coordinates are within bounds.
	bool InBounds(int x, int y) const {
		return (x >= 0 && x < width_ && y >= 0 && y < height_);
	}

	// Converts 2D grid coordinates to a 1D index for the blocked_ vector.
	int ToIndex(int x, int y) const {
		return y * width_ + x;
	}

	// Checks if the cell at the given grid coordinates is blocked (not walkable).
	bool IsBlocked(int x, int y) const {
		if (!InBounds(x, y)) return true;
		return blocked_[ToIndex(x, y)] != 0;
	}

	// Checks if the cell at the given grid coordinates is walkable (not blocked and within bounds).
	bool IsWalkable(int x, int y) const {
		return InBounds(x, y) && !IsBlocked(x, y);
	}

	// Sets the blocked state of the cell at the given grid coordinates.
	void SetBlocked(int x, int y, bool blocked) {
		if (!InBounds(x, y)) return;
		blocked_[ToIndex(x, y)] = blocked ? 1 : 0;
	}

	// Converts world coordinates to grid coordinates, clamping to grid bounds if necessary.
	GridCoord WorldToCell(const glm::vec2& world) const {
		GridCoord c;
		c.x = static_cast<int>((world.x - originX_) / cellSize_);
		c.y = static_cast<int>((world.y - originY_) / cellSize_);

		if (c.x < 0) c.x = 0;
		if (c.y < 0) c.y = 0;
		if (c.x >= width_)  c.x = width_ - 1;
		if (c.y >= height_) c.y = height_ - 1;

		return c;
	}

	// Converts grid coordinates to world coordinates of the cell center.
	glm::vec2 CellCenter(const GridCoord& c) const {
		return glm::vec2(
			originX_ + (static_cast<float>(c.x) + 0.5f) * cellSize_,
			originY_ + (static_cast<float>(c.y) + 0.5f) * cellSize_
		);
	}

	// Finds the nearest walkable cell to the given grid coordinates, searching in a spiral pattern. Returns true if a walkable cell is found and sets outCell to its coordinates.
	bool FindNearestWalkable(const GridCoord& from, GridCoord& outCell) const;

private:
	// Grid origin in world coordinates, cell size, grid dimensions, and blocked cell data.
	float originX_ = 0.0f;
	float originY_ = 0.0f;
	float cellSize_ = 50.0f;
	int width_ = 0;
	int height_ = 0;

	std::vector<unsigned char> blocked_;
};

class GridPathfinder {
public:
	// Finds a path from start to goal on the given NavGrid using A* search. Returns true if a path is found and sets outPath to the sequence of grid coordinates from start to goal.
	static bool FindPath(const NavGrid& grid,
		const GridCoord& start,
		const GridCoord& goal,
		std::vector<GridCoord>& outPath);
};

