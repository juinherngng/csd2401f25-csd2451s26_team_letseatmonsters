/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelSerializer.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu
 CO-AUTHORS:		Seah Wang Hua, wanghua.seah@digipen.edu

 DESCRIPTION:		Handles saving and loading of LevelData to and from JSON files.

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <fstream>

#include "JSONInclude.hpp"
#include "LevelSerializer.hpp"

using nlohmann::json;

// Helpers (local)
static LevelObject ReadLevelObject(const json& jsonObj) {
	LevelObject obj{};

	obj.texture = jsonObj.value("texture", "");
	obj.tag = jsonObj.value("tag", "");
	obj.layer = jsonObj.value("layer", "");

	obj.x = jsonObj.value("x", 0.0f);
	obj.y = jsonObj.value("y", 0.0f);
	obj.z = jsonObj.value("z", 0.0f);
	obj.w = jsonObj.value("w", 128.0f);
	obj.h = jsonObj.value("h", 128.0f);

	// Stored in degrees for editor friendliness
	obj.rotation = jsonObj.value("rotation", 0.0f);

	obj.colWidth = jsonObj.value("col_w", 64.0f);
	obj.colHeight = jsonObj.value("col_h", 128.0f);
	obj.colOffsetX = jsonObj.value("col_offx", 0.0f);
	obj.colOffsetY = jsonObj.value("col_offy", 0.0f);

	obj.speedX = jsonObj.value("speed_x", 0.0f);
	obj.speedY = jsonObj.value("speed_y", 0.0f);

	obj.animated = jsonObj.value("animated", false);

	return obj;
}

// Converts a LevelObject to JSON
static json WriteLevelObject(const LevelObject& obj) {
	json jsonData = {
		{ "texture", obj.texture },
		{ "tag", obj.tag },
		{ "x", obj.x },
		{ "y", obj.y },
		{ "z", obj.z },
		{ "w", obj.w },
		{ "h", obj.h },
		{ "rotation", obj.rotation },
		{ "col_w", obj.colWidth },
		{ "col_h", obj.colHeight },
		{ "col_offx", obj.colOffsetX },
		{ "col_offy", obj.colOffsetY },
		{ "speed_x", obj.speedX },
		{ "speed_y", obj.speedY },
		{ "animated", obj.animated },
		{ "layer", obj.layer }
	};

	return jsonData;
}

// Public Interface
bool LevelSerializer::Load(const std::string& path, LevelData& outLevel) {
	std::ifstream file(path);
	if (!file) {
		return false;
	}

	json jsonData;
	file >> jsonData;

	outLevel.objects.clear();

	if (!jsonData.contains("objects")) {
		return true; // empty level file is valid
	}

	for (auto& jsonObj : jsonData["objects"]) {
		outLevel.objects.push_back(ReadLevelObject(jsonObj));
	}

	return true;
}

// Saves LevelData into a JSON file
bool LevelSerializer::Save(const std::string& path, const LevelData& inLevel) {
	json jsonData = json::object();

	// Try to load existing JSON so we keep things like "collision"
	{
		std::ifstream in(path);
		if (in) {
			try {
				in >> jsonData;
			}
			catch (...) {
				// If parse fails, fall back to a clean object
				jsonData = json::object();
			}
		}
	}

	// Replace ONLY the "objects" array with the new snapshot
	jsonData["objects"] = json::array();
	for (const auto& obj : inLevel.objects) {
		jsonData["objects"].push_back(WriteLevelObject(obj));
	}

	std::ofstream file(path);
	if (!file) {
		return false;
	}

	file << jsonData.dump(2);
	return true;
}
