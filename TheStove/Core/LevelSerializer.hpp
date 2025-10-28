/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelSerializer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

struct LevelObject {
	std::string texture;
	float x{}, y{}, w{ 128 }, h{ 128 }, rotation{ 0 };
};

struct LevelData {
	std::vector<LevelObject> objects;
};

namespace LevelSerializer {
	bool Load(const std::string& path, LevelData& out);
	bool Save(const std::string& path, const LevelData& in);
}
