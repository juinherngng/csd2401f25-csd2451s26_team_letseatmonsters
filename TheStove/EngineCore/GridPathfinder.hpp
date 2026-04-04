/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GridPathfinder.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (80%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (20%)

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

	/**
	 * @brief Compares two grid coordinates for equality.
	 * @param rhs Coordinate to compare against.
	 * @return True if both coordinates store the same cell position.
	 */
	bool operator==(const GridCoord& rhs) const {
		// Equality is purely coordinate-based.
		return x == rhs.x && y == rhs.y;
	}
};

class NavGrid {
public:
	NavGrid() = default;

	/**
	 * @brief Constructs a navigation grid with the supplied world-space layout.
	 * @param originX World-space X coordinate of the grid origin.
	 * @param originY World-space Y coordinate of the grid origin.
	 * @param cellSize Size of each grid cell in world units.
	 * @param width Number of cells horizontally.
	 * @param height Number of cells vertically.
	 */
	NavGrid(float originX, float originY, float cellSize, int width, int height) {
		// Reuse Reset so construction and reconfiguration share the same setup path.
		Reset(originX, originY, cellSize, width, height);
	}

	/**
	 * @brief Reconfigures the grid and clears all blocked-cell state.
	 * @param originX World-space X coordinate of the grid origin.
	 * @param originY World-space Y coordinate of the grid origin.
	 * @param cellSize Size of each grid cell in world units.
	 * @param width Number of cells horizontally.
	 * @param height Number of cells vertically.
	 */
	void Reset(float originX, float originY, float cellSize, int width, int height) {
		// Replace every grid setting at once so callers can rebuild the navigation space cheaply.
		originX_ = originX;
		originY_ = originY;
		cellSize_ = cellSize;
		width_ = width;
		height_ = height;
		blocked_.assign(width_ * height_, 0);
	}

	/**
	 * @brief Checks whether the grid has usable dimensions and cell size.
	 * @return True if the grid dimensions and cell size are valid.
	 */
	bool IsValid() const {
		// Pathfinding requires positive bounds and non-zero cell size.
		return width_ > 0 && height_ > 0 && cellSize_ > 0.0f;
	}

	/**
	 * @brief Returns the grid width in cells.
	 * @return Horizontal cell count.
	 */
	int Width() const {
		// Expose the cached width used by pathfinding and debug tooling.
		return width_;
	}

	/**
	 * @brief Returns the grid height in cells.
	 * @return Vertical cell count.
	 */
	int Height() const {
		// Expose the cached height used by pathfinding and debug tooling.
		return height_;
	}

	/**
	 * @brief Returns the size of a single cell in world units.
	 * @return Cell size in world units.
	 */
	float CellSize() const {
		// Expose the configured cell size for world/grid conversions.
		return cellSize_;
	}

	/**
	 * @brief Checks whether a cell coordinate lies inside the grid bounds.
	 * @param x Cell X coordinate.
	 * @param y Cell Y coordinate.
	 * @return True if the coordinate is in bounds.
	 */
	bool InBounds(int x, int y) const {
		// Bound checks protect every later access into the blocked-cell array.
		return (x >= 0 && x < width_ && y >= 0 && y < height_);
	}

	/**
	 * @brief Converts a 2D cell coordinate into the backing array index.
	 * @param x Cell X coordinate.
	 * @param y Cell Y coordinate.
	 * @return Flattened array index for the requested cell.
	 */
	int ToIndex(int x, int y) const {
		// Store cells row-major so neighboring Y values advance by grid width.
		return y * width_ + x;
	}

	/**
	 * @brief Checks whether a cell is blocked.
	 * @param x Cell X coordinate.
	 * @param y Cell Y coordinate.
	 * @return True if the cell is blocked or outside the grid.
	 */
	bool IsBlocked(int x, int y) const {
		// Treat out-of-bounds cells as blocked so pathfinding never walks outside the grid.
		if (!InBounds(x, y)) return true;
		return blocked_[ToIndex(x, y)] != 0;
	}

	/**
	 * @brief Checks whether a cell is walkable.
	 * @param x Cell X coordinate.
	 * @param y Cell Y coordinate.
	 * @return True if the cell is inside the grid and not blocked.
	 */
	bool IsWalkable(int x, int y) const {
		// Walkable means both in-bounds and not marked blocked.
		return InBounds(x, y) && !IsBlocked(x, y);
	}

	/**
	 * @brief Sets whether a cell is blocked.
	 * @param x Cell X coordinate.
	 * @param y Cell Y coordinate.
	 * @param blocked True to mark the cell blocked, false to mark it walkable.
	 */
	void SetBlocked(int x, int y, bool blocked) {
		// Ignore writes outside the grid so callers do not need to pre-check bounds.
		if (!InBounds(x, y)) return;
		blocked_[ToIndex(x, y)] = blocked ? 1 : 0;
	}

	/**
	 * @brief Converts world coordinates into a clamped grid cell coordinate.
	 * @param world World-space position to convert.
	 * @return Grid cell coordinate clamped to the grid bounds.
	 */
	GridCoord WorldToCell(const glm::vec2& world) const {
		GridCoord c;
		// Convert world offsets into cell indices relative to the grid origin.
		c.x = static_cast<int>((world.x - originX_) / cellSize_);
		c.y = static_cast<int>((world.y - originY_) / cellSize_);

		// Clamp to the nearest valid cell so callers always receive an in-bounds coordinate.
		if (c.x < 0) c.x = 0;
		if (c.y < 0) c.y = 0;
		if (c.x >= width_)  c.x = width_ - 1;
		if (c.y >= height_) c.y = height_ - 1;

		return c;
	}

	/**
	 * @brief Returns the world-space center position of a grid cell.
	 * @param c Cell coordinate to convert.
	 * @return World-space center of the cell.
	 */
	glm::vec2 CellCenter(const GridCoord& c) const {
		// Offset by half a cell so movement targets land at the center of each tile.
		return glm::vec2(
			originX_ + (static_cast<float>(c.x) + 0.5f) * cellSize_,
			originY_ + (static_cast<float>(c.y) + 0.5f) * cellSize_
		);
	}

	/**
	 * @brief Finds the nearest walkable cell to a starting coordinate.
	 * @param from Starting cell to search from.
	 * @param outCell Output walkable cell if one is found.
	 * @return True if a walkable cell was found, otherwise false.
	 */
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
	/**
	 * @brief Finds a path between two grid cells using A* search.
	 * @param grid Navigation grid to search on.
	 * @param start Start cell.
	 * @param goal Goal cell.
	 * @param outPath Output path from start to goal.
	 * @return True if a path was found, otherwise false.
	 */
	static bool FindPath(const NavGrid& grid,
		const GridCoord& start,
		const GridCoord& goal,
		std::vector<GridCoord>& outPath);
};
