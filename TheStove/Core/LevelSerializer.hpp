/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelSerializer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

struct LevelObject {
	std::string texture;
	std::string tag;        // e.g., "player", "npc1", "npc2", "dino"

	float x = 0.f, y = 0.f, z = 0.f;   // z optional
	float w = 128.f, h = 128.f;
	float rotation = 0.f;

	// collider
	float col_w = 64.f, col_h = 128.f;
	float col_offx = 0.f, col_offy = 0.f;

	// optional motion (for simple NPC lane movers, etc.)
	float speed_x = 0.f, speed_y = 0.f;

	bool animated = false;  // if true, we’ll use animated spawn
};

struct LevelData {
	std::vector<LevelObject> objects;
};

namespace LevelSerializer {
	bool Load(const std::string& path, LevelData& out);
	bool Save(const std::string& path, const LevelData& in);
}
