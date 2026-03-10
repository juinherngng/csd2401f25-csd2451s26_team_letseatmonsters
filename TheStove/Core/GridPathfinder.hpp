/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GridPathfinder.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)
 DESCRIPTION:		

        All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glm/vec2.hpp>
#include <vector>

struct GridCoord
{
    int x = 0;
    int y = 0;

    bool operator==(const GridCoord& rhs) const
    {
        return x == rhs.x && y == rhs.y;
    }
};

class NavGrid
{
public:
    NavGrid() = default;

    NavGrid(float originX, float originY, float cellSize, int width, int height)
    {
        Reset(originX, originY, cellSize, width, height);
    }

    void Reset(float originX, float originY, float cellSize, int width, int height)
    {
        originX_ = originX;
        originY_ = originY;
        cellSize_ = cellSize;
        width_ = width;
        height_ = height;
        blocked_.assign(width_ * height_, 0);
    }

    bool IsValid() const
    {
        return width_ > 0 && height_ > 0 && cellSize_ > 0.0f;
    }

    int Width() const { return width_; }
    int Height() const { return height_; }
    float CellSize() const { return cellSize_; }

    bool InBounds(int x, int y) const
    {
        return (x >= 0 && x < width_ && y >= 0 && y < height_);
    }

    int ToIndex(int x, int y) const
    {
        return y * width_ + x;
    }

    bool IsBlocked(int x, int y) const
    {
        if (!InBounds(x, y)) return true;
        return blocked_[ToIndex(x, y)] != 0;
    }

    bool IsWalkable(int x, int y) const
    {
        return InBounds(x, y) && !IsBlocked(x, y);
    }

    void SetBlocked(int x, int y, bool blocked)
    {
        if (!InBounds(x, y)) return;
        blocked_[ToIndex(x, y)] = blocked ? 1 : 0;
    }

    GridCoord WorldToCell(const glm::vec2& world) const
    {
        GridCoord c;
        c.x = static_cast<int>((world.x - originX_) / cellSize_);
        c.y = static_cast<int>((world.y - originY_) / cellSize_);

        if (c.x < 0) c.x = 0;
        if (c.y < 0) c.y = 0;
        if (c.x >= width_)  c.x = width_ - 1;
        if (c.y >= height_) c.y = height_ - 1;

        return c;
    }

    glm::vec2 CellCenter(const GridCoord& c) const
    {
        return glm::vec2(
            originX_ + (static_cast<float>(c.x) + 0.5f) * cellSize_,
            originY_ + (static_cast<float>(c.y) + 0.5f) * cellSize_
        );
    }

    bool FindNearestWalkable(const GridCoord& from, GridCoord& outCell) const;

private:
    float originX_ = 0.0f;
    float originY_ = 0.0f;
    float cellSize_ = 50.0f;
    int width_ = 0;
    int height_ = 0;

    std::vector<unsigned char> blocked_;
};

class GridPathfinder
{
public:
    static bool FindPath(const NavGrid& grid,
        const GridCoord& start,
        const GridCoord& goal,
        std::vector<GridCoord>& outPath);
};