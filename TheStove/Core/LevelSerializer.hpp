/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelSerializer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu
 CO-AUTHORS:		Seah Wang Hua, wanghua.seah@digipen.edu

 DESCRIPTION:		JSON-based (de)serialization for level data used by the editor/runtime.

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

 // Types
struct LevelObject {
	// Texture / tagging / layer
	std::string texture;
	std::string tag;
	std::string layer;

	// Transform (z used for sort/layering if applicable)
	float x{ 0.0f };
	float y{ 0.0f };
	float z{ 0.0f };
	float w{ 128.0f };
	float h{ 128.0f };
	float rotation{ 0.0f }; // stored in degrees for editor compatibility

	// Collision box (size + local offset)
	float colWidth{ 64.0f };
	float colHeight{ 128.0f };
	float colOffsetX{ 0.0f };
	float colOffsetY{ 0.0f };

	// Optional motion (editor helpers)
	float speedX{ 0.0f };
	float speedY{ 0.0f };

	// Animation flag
	bool animated{ false };
	std::string animName;
};

struct LevelData {
	std::vector<LevelObject> objects{};
	std::string background; // optional background texture path
};

// Public Interface
struct LevelSerializer {
	// Load a JSON file into outLevel; returns false if file open/parse failed.
	static bool Load(const std::string& path, LevelData& outLevel);

	// Save inLevel as pretty-printed JSON; returns false if file open failed.
	static bool Save(const std::string& path, const LevelData& inLevel);
};
