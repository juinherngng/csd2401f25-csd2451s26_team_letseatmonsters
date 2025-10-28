#include <fstream>

#include "LevelSerializer.hpp"
#include "JSON.hpp"
using nlohmann::json;

bool LevelSerializer::Load(const std::string& path, LevelData& out) {
	std::ifstream f(path);
	if (!f) return false;
	json j; f >> j;
	out.objects.clear();
	if (!j.contains("objects")) return true;
	for (auto& o : j["objects"]) {
		LevelObject lo;
		lo.texture = o.value("texture", "");
		lo.x = o.value("x", 0.f);
		lo.y = o.value("y", 0.f);
		lo.w = o.value("w", 128.f);
		lo.h = o.value("h", 128.f);
		lo.rotation = o.value("rotation", 0.f);
		out.objects.push_back(lo);
	}
	return true;
}

bool LevelSerializer::Save(const std::string& path, const LevelData& in) {
	json j;
	j["objects"] = json::array();
	for (auto& o : in.objects) {
		j["objects"].push_back({
		  {"texture",  o.texture},
		  {"x",        o.x},
		  {"y",        o.y},
		  {"w",        o.w},
		  {"h",        o.h},
		  {"rotation", o.rotation}
			});
	}
	std::ofstream f(path);
	if (!f) return false;
	f << j.dump(2);
	return true;
}
