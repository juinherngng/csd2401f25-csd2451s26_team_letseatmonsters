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
