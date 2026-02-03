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
	obj.hasCollider = jsonObj.value("has_collider", true);

	obj.colWidth = jsonObj.value("col_w", 64.0f);
	obj.colHeight = jsonObj.value("col_h", 128.0f);
	obj.colOffsetX = jsonObj.value("col_offx", 0.0f);
	obj.colOffsetY = jsonObj.value("col_offy", 0.0f);

	if (!obj.hasCollider) {
		obj.colWidth = 0.0f;
		obj.colHeight = 0.0f;
		obj.colOffsetX = 0.0f;
		obj.colOffsetY = 0.0f;
	}

	obj.speedX = jsonObj.value("speed_x", 0.0f);
	obj.speedY = jsonObj.value("speed_y", 0.0f);

	obj.animated = jsonObj.value("animated", false);

	// Approach offset (safe for existing JSON, defaults to 0)
	obj.approachOffsetX = jsonObj.value("approach_offx", 0.0f);
	obj.approachOffsetY = jsonObj.value("approach_offy", 0.0f);

	obj.shadow = jsonObj.value("shadow", false);

	return obj;
}

// Read a text object from JSON
static LevelTextObject ReadTextObject(const json& jsonObj) {
	LevelTextObject obj{};
	
	obj.name = jsonObj.value("name", "");
	obj.text = jsonObj.value("text", "");
	obj.fontName = jsonObj.value("fontName", "");
	obj.fontSize = jsonObj.value("fontSize", 48u);
	
	obj.x = jsonObj.value("x", 0.0f);
	obj.y = jsonObj.value("y", 0.0f);
	obj.scale = jsonObj.value("scale", 1.0f);
	obj.rotation = jsonObj.value("rotation", 0.0f);
	obj.useBlockRotation = jsonObj.value("useBlockRotation", true);
	
	obj.colorR = jsonObj.value("colorR", 1.0f);
	obj.colorG = jsonObj.value("colorG", 1.0f);
	obj.colorB = jsonObj.value("colorB", 1.0f);
	obj.colorA = jsonObj.value("colorA", 1.0f);
	
	obj.layer = jsonObj.value("layer", "1");
	
	return obj;
}

// Converts a LevelObject to JSON
static json WriteLevelObject(const LevelObject& obj) {
	json jsonData = {
		{ "texture", obj.texture },
		{ "tag", obj.tag },
		{ "layer", obj.layer},
		{ "x", obj.x },
		{ "y", obj.y },
		{ "z", obj.z },
		{ "w", obj.w },
		{ "h", obj.h },
		{ "rotation", obj.rotation },
		{ "has_collider", obj.hasCollider },
		{ "col_w", obj.colWidth },
		{ "col_h", obj.colHeight },
		{ "col_offx", obj.colOffsetX },
		{ "col_offy", obj.colOffsetY },
		{ "speed_x", obj.speedX },
		{ "speed_y", obj.speedY },
		{ "animated", obj.animated },
		{ "layer", obj.layer },
		// NEW: approach offset
{ "approach_offx", obj.approachOffsetX },
{ "approach_offy", obj.approachOffsetY }
	};

	return jsonData;
}

// Converts a LevelTextObject to JSON
static json WriteTextObject(const LevelTextObject& obj) {
	json jsonData = {
		{ "name", obj.name },
		{ "text", obj.text },
		{ "fontName", obj.fontName },
		{ "fontSize", obj.fontSize },
		{ "x", obj.x },
		{ "y", obj.y },
		{ "scale", obj.scale },
		{ "rotation", obj.rotation },
		{ "useBlockRotation", obj.useBlockRotation },
		{ "colorR", obj.colorR },
		{ "colorG", obj.colorG },
		{ "colorB", obj.colorB },
		{ "colorA", obj.colorA },
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
	outLevel.textObjects.clear();
	outLevel.background.clear();

	// optional background
	outLevel.background = jsonData.value("background", "");

	if (jsonData.contains("objects")) {
		for (auto& jsonObj : jsonData["objects"]) {
			outLevel.objects.push_back(ReadLevelObject(jsonObj));
		}
	}

	// Load text objects if present
	if (jsonData.contains("textObjects")) {
		for (auto& jsonObj : jsonData["textObjects"]) {
			outLevel.textObjects.push_back(ReadTextObject(jsonObj));
		}
	}

	return true;
}

bool LevelSerializer::Save(const std::string& path, const LevelData& inLevel) {
	json jsonData = json::object();

	// Try to load existing JSON to preserve unrelated keys
	{
		std::ifstream in(path);
		if (in) {
			try { in >> jsonData; }
			catch (...) { jsonData = json::object(); }
		}
	}

	// Write background if present
	if (!inLevel.background.empty()) {
		jsonData["background"] = inLevel.background;
	}
	else {
		// Optional: erase background if you want to remove it
		// jsonData.erase("background");
	}

	// Replace ONLY the "objects" array
	jsonData["objects"] = json::array();
	for (const auto& obj : inLevel.objects) {
		jsonData["objects"].push_back(WriteLevelObject(obj));
	}

	// NEW: Replace the "textObjects" array
	jsonData["textObjects"] = json::array();

	for (const auto& textObj : inLevel.textObjects) {
		jsonData["textObjects"].push_back(WriteTextObject(textObj));
	}

	std::ofstream file(path);
	if (!file) return false;
	file << jsonData.dump(2);
	return true;
}
