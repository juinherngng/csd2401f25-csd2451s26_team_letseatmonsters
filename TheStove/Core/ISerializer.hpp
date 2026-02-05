/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			ISerializer.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:		Very simple serializer for reading key=value pairs from a text file.

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct ComponentData {
	std::string type;   // e.g. "Transform"
	std::unordered_map<std::string, std::string> properties; // key=value pairs
};

class ISerializer {
public:
	bool Load(const std::string&);

	const std::vector<ComponentData>& GetComponents() const;

	static float GetFloat(const ComponentData& comp, const std::string& key, float defaultVal = 0.0f);
	static bool  GetBool(const ComponentData& comp, const std::string& key, bool defaultVal = false);

private:
	std::vector<ComponentData> components;
};
