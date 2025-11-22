/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			TileMap.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Darren Toh, darren.toh@digipen.edu

 DESCRIPTION:		Source file with member function definitions to construct and read from class MapData
					containing a 2D vector of Int, which serves
					as a tile map for level environment and logic

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "TileMap.hpp"

//Read-Only Access
int MapData::getWidth() const {
	return width;
}

//Read-Only Access
int MapData::getHeight() const {
	return height;
}

//Cal Area
int MapData::getArea() const {
	return (MapData::getHeight() * MapData::getWidth());
}

//Modifiable Access
void MapData::setTile(int x, int y, int value) {
	if (x >= 0 && x < width && y >= 0 && y < height) {
		tiles[y][x] = value;
	}
	else {
		std::cout << "ERROR: Attempting to set tile out of bounds" << std::endl;
	}
}

//Read-Only Access
int MapData::getTile(int x, int y) const {
	if (x >= 0 && x < width && y >= 0 && y < height) {
		return tiles[y][x];
	}
	std::cout << "ERROR: Attempting to get tile out of bounds" << std::endl;
	return -99; // invalid
}

//For Debugging purposes
void MapData::printMap() const {
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			std::cout << tiles[y][x] << " ";
		}
		std::cout << "\n";
	}
}

int MapData::SweepFor(int dataType) {
	(void)dataType; // Suppress unused parameter warning

	int retVal = 0;
	for (int y = 0; y < getHeight(); ++y) {
		for (int x = 0; x < getWidth(); ++x) {
			if (getTile(y, x) == 1) {
				retVal++;
			}
		}
	}
	return retVal;
}

bool MapData::isWalkable(int x, int y) {
	if (getTile(x, y) == 0) {
		return true;
	}
	return false;
}
