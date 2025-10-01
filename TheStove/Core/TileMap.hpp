/*
----------------------------------------------------------------------------------------------------
FILE NAME:			TileMap.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Darren Toh, darren.toh@digipen.edu

DESCRIPTION:		Header file with member function declaration to construct and read from class MapData
					containing a 2D vector of Int, which serves
					as a tile map for level environment and logic

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#include <iostream>
#include <vector>

//To be Edited with more IDs depending on what is needed on the map
enum TileStateID {
	WALKABLE = 0,
	ENTITY = 1,
	WALL = 99,
};

class MapData {
private:
	int width, height; //dimensions
	std::vector<std::vector<int>> tiles; //2d grid of tiles

public:
	//Constructor
	MapData(int x, int y) : width(x), height(y), tiles(y, std::vector<int>(x, 0)){}

	//Modifiable Access
	void setTile(int x, int y, int value);

	//Read-Only Access
	int getTile(int x, int y) const;

	//GetArea(No of Tiles)
	int getArea() const;

	//Read-Only Access to Map Width
	int getWidth() const;

	//Read-Only Access to Map Height
	int getHeight() const;

	//For Debugging purposes
	void printMap()const;

	//Count num of this value on the Map
	int SweepFor(int dataType);

	//Check if tile is walkable
	bool isWalkable(int x, int y);
};
