/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			ISerializer.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (70%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (30%)

 DESCRIPTION:		Very simple serializer for reading key=value pairs from a text file.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <fstream>
#include <sstream>

#include "EngineCore/ISerializer.hpp"
#include "EngineCore/Logger.hpp"

/**
 * @brief Loads component records from a simple key-value text format.
 * @param filename Path to the file to parse.
 * @return True when the file was opened and parsed successfully.
 */
bool ISerializer::Load(const std::string& filename) {
	// Start from a clean state so repeated loads never leave stale component data behind.
	components.clear();

	std::ifstream file(filename);
	if (!file.is_open()) {
		TS_LOG_ERROR("[ISerializer] Failed to open file: " << filename);
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		// Skip empty lines so the serializer can tolerate simple spacing in the source file.
		if (line.empty()) continue;

		std::istringstream iss(line);
		std::string type;
		// The first token identifies which component type the remaining properties belong to.
		iss >> type;

		ComponentData comp;
		comp.type = type;

		std::string token;
		while (iss >> token) {
			auto eq = token.find('=');
			if (eq != std::string::npos) {
				// Split each token into key=value pairs and store them as raw strings for later conversion.
				std::string key = token.substr(0, eq);
				std::string val = token.substr(eq + 1);
				comp.properties[key] = val;
			}
		}
		// Preserve the original record ordering so callers can process components deterministically.
		components.push_back(comp);
	}

	return true;
}

/**
 * @brief Returns the component records parsed by the serializer.
 * @return Immutable reference to the stored component list.
 */
const std::vector<ComponentData>& ISerializer::GetComponents() const {
	return components;
}

/**
 * @brief Converts a stored property into a float value.
 * @param comp The parsed component record.
 * @param key The property key to read.
 * @param defaultVal Fallback used when the property is missing or invalid.
 * @return The parsed float value or the supplied default.
 */
float ISerializer::GetFloat(const ComponentData& comp, const std::string& key, float defaultVal) {
	auto it = comp.properties.find(key);
	if (it == comp.properties.end()) {
		return defaultVal;
	}
	try {
		// std::stof handles the raw string-to-float conversion for the simple text format.
		return std::stof(it->second);
	}
	catch (...) {
		// Invalid numeric text falls back to the caller-provided default instead of aborting the load.
		return defaultVal;
	}
}

/**
 * @brief Converts a stored property into a boolean value.
 * @param comp The parsed component record.
 * @param key The property key to read.
 * @param defaultVal Fallback used when the property is missing or invalid.
 * @return True when the stored numeric value is non-zero, otherwise false.
 */
bool ISerializer::GetBool(const ComponentData& comp, const std::string& key, bool defaultVal) {
	auto it = comp.properties.find(key);
	if (it == comp.properties.end()) {
		return defaultVal;
	}
	try {
		// The format stores booleans as integer-like strings such as 0 or 1.
		return std::stoi(it->second) != 0;
	}
	catch (...) {
		// Invalid values are treated as absent so callers keep predictable defaults.
		return defaultVal;
	}
}
