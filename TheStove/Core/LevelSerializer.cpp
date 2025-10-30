/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelSerializer.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <fstream>

#include "LevelSerializer.hpp"
#include "JSONInclude.hpp"
using nlohmann::json;

static LevelObject ReadObj(const json& o) {
	LevelObject lo;
	lo.texture = o.value("texture", "");
	lo.tag = o.value("tag", "");

	lo.x = o.value("x", 0.f);
	lo.y = o.value("y", 0.f);
	lo.z = o.value("z", 0.f);
	lo.w = o.value("w", 128.f);
	lo.h = o.value("h", 128.f);
	lo.rotation = o.value("rotation", 0.f);

	lo.col_w = o.value("col_w", 64.f);
	lo.col_h = o.value("col_h", 128.f);
	lo.col_offx = o.value("col_offx", 0.f);
	lo.col_offy = o.value("col_offy", 0.f);

	lo.speed_x = o.value("speed_x", 0.f);
	lo.speed_y = o.value("speed_y", 0.f);

	lo.animated = o.value("animated", false);
	return lo;
}

static json WriteObj(const LevelObject& o) {
	return json{
		{"texture",   o.texture},
		{"tag",       o.tag},
		{"x",         o.x}, {"y", o.y}, {"z", o.z},
		{"w",         o.w}, {"h", o.h},
		{"rotation",  o.rotation},
		{"col_w",     o.col_w}, {"col_h", o.col_h},
		{"col_offx",  o.col_offx}, {"col_offy", o.col_offy},
		{"speed_x",   o.speed_x}, {"speed_y", o.speed_y},
		{"animated",  o.animated}
	};
}

bool LevelSerializer::Load(const std::string& path, LevelData& out) {
	std::ifstream f(path);
	if (!f) return false;
	json j; f >> j;
	out.objects.clear();

	if (!j.contains("objects")) return true;
	for (auto& o : j["objects"])
		out.objects.push_back(ReadObj(o));
	return true;
}

bool LevelSerializer::Save(const std::string& path, const LevelData& in) {
	json j;
	j["objects"] = json::array();
	for (auto& o : in.objects) j["objects"].push_back(WriteObj(o));
	std::ofstream f(path);
	if (!f) return false;
	f << j.dump(2);
	return true;
}
