/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelSerializer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu
 CO-AUTHORS:		Seah Wang Hua, wanghua.seah@digipen.edu
					Ng Juin Herng, juinherng.ng@digipen.edu (30%)

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
	bool hasCollider = true;
	float colWidth{ 64.0f };
	float colHeight{ 128.0f };
	float colOffsetX{ 0.0f };
	float colOffsetY{ 0.0f };

	// Optional motion (editor helpers)
	float speedX{ 0.0f };
	float speedY{ 0.0f };

	// Approach offset (relative to object center)
	float approachOffsetX{ 0.0f };
	float approachOffsetY{ 0.0f };

	// Animation flag
	bool animated{ false };
	std::string animName;

	// Audio bindings (names from AudioCatalog)
	std::string audioOnSpawn;      // Played when object spawns/loads
	std::string audioOnInteract;   // Played when player interacts with object
	std::string audioOnDestroy;    // Played when object is destroyed/despawned
	std::string audioOnProcessing; // Played while work table is processing (loops)
	bool audioLoop{ false };       // Whether audioOnSpawn should loop

	// Shadow flag
	bool shadow{ false };

	// Visibility flag (per-object toggle)
	bool visible{ true };
};

// Text object for font-based text rendering in levels
struct LevelTextObject {
	// Identification
	std::string name;            // Display name in editor
	
	// Text content
	std::string text;            // The text to render
	std::string fontName;		 // Name of the loaded font to use
	unsigned int fontSize{ 48 }; // Font size (for reloading font if needed)
	
	// Transform
	float x{ 0.0f };
	float y{ 0.0f };
	float scale{ 1.0f };
	float rotation{ 0.0f };		   // Rotation in degrees
	bool useBlockRotation{ true }; // true = block rotation, false = per-character
	
	// Appearance
	float colorR{ 1.0f };
	float colorG{ 1.0f };
	float colorB{ 1.0f };
	float colorA{ 1.0f };
	
	// Layer for rendering order
	std::string layer{ "1" };
};

struct TextObjectData {
	std::string name;
	std::string fontName;
	std::string text;
	float x = 0.0f;
	float y = 0.0f;
	float scale = 1.0f;
};

struct LevelData {
	std::vector<LevelObject> objects{};
	std::vector<LevelTextObject> textObjects{};  // text objects in the level
	std::string background; // optional background texture path
};

// Public Interface
struct LevelSerializer {
	// Load a JSON file into outLevel; returns false if file open/parse failed.
	static bool Load(const std::string& path, LevelData& outLevel);

	// Save inLevel as pretty-printed JSON; returns false if file open failed.
	static bool Save(const std::string& path, const LevelData& inLevel);
};
