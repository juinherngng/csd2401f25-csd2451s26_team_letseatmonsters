/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelSerializer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Declares LevelSerializer for saving/loading LevelData.
					Rotation values are stored in DEGREES in JSON (editor/UI friendly).

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

 // Represents a single object saved/loaded in a level file.
struct LevelObject {
	// Visuals / identity
	std::string texture;
	std::string tag;

	// Transform (position, size, rotationDeg)
	float x{ 0.0f };
	float y{ 0.0f };
	float z{ 0.0f };
	float w{ 128.0f };
	float h{ 128.0f };
	float rotation{ 0.0f }; // Stored in DEGREES in JSON

	// Collider
	float colWidth{ 64.0f };
	float colHeight{ 128.0f };
	float colOffsetX{ 0.0f };
	float colOffsetY{ 0.0f };

	// Optional motion (used by some NPCs)
	float speedX{ 0.0f };
	float speedY{ 0.0f };

	// Animation flag
	bool animated{ false };
};

struct LevelData {
	std::vector<LevelObject> objects{};
};

struct LevelSerializer {
	static bool Load(const std::string& path, LevelData& outLevel);
	static bool Save(const std::string& path, const LevelData& inLevel);
};
