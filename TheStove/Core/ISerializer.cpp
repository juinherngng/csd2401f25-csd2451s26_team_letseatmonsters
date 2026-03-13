/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			ISerializer.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:		Very simple serializer for reading key=value pairs from a text file.

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "Core/ISerializer.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

bool ISerializer::Load(const std::string& filename) {
	components.clear();

	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Failed to open file: " << filename << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		if (line.empty()) continue;

		std::istringstream iss(line);
		std::string type;
		iss >> type;

		ComponentData comp;
		comp.type = type;

		std::string token;
		while (iss >> token) {
			auto eq = token.find('=');
			if (eq != std::string::npos) {
				std::string key = token.substr(0, eq);
				std::string val = token.substr(eq + 1);
				comp.properties[key] = val;
			}
		}
		components.push_back(comp);
	}

	return true;
}

const std::vector<ComponentData>& ISerializer::GetComponents() const {
	return components;
}

float ISerializer::GetFloat(const ComponentData& comp, const std::string& key, float defaultVal) {
	auto it = comp.properties.find(key);
	if (it == comp.properties.end()) {
		return defaultVal;
	}
	try {
		return std::stof(it->second);
	}
	catch (...) {
		return defaultVal;
	}
}

bool ISerializer::GetBool(const ComponentData& comp, const std::string& key, bool defaultVal) {
	auto it = comp.properties.find(key);
	if (it == comp.properties.end()) {
		return defaultVal;
	}
	try {
		return std::stoi(it->second) != 0;
	}
	catch (...) {
		return defaultVal;
	}
}
