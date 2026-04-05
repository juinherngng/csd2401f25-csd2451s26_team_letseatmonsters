/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			ISerializer.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (90%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (10%)

 DESCRIPTION:		Very simple serializer for reading key=value pairs from a text file.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
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
	/**
	 * @brief Loads component records from a simple text file.
	 * @param filename Path to the serialized component data file.
	 * @return True when the file was opened and parsed successfully.
	 */
	bool Load(const std::string&);

	/**
	 * @brief Returns the parsed component records from the last successful load.
	 * @return Immutable reference to the stored component list.
	 */
	const std::vector<ComponentData>& GetComponents() const;

	/**
	 * @brief Reads a floating-point property from a component record.
	 * @param comp The parsed component record.
	 * @param key The property key to read.
	 * @param defaultVal Value returned when the property is missing or invalid.
	 * @return The parsed float value or the provided default.
	 */
	static float GetFloat(const ComponentData& comp, const std::string& key, float defaultVal = 0.0f);

	/**
	 * @brief Reads a boolean property from a component record.
	 * @param comp The parsed component record.
	 * @param key The property key to read.
	 * @param defaultVal Value returned when the property is missing or invalid.
	 * @return The parsed boolean value or the provided default.
	 */
	static bool  GetBool(const ComponentData& comp, const std::string& key, bool defaultVal = false);

private:
	std::vector<ComponentData> components;
};
