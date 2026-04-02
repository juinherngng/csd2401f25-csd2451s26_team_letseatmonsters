/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelSerializer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu		(50%)
 CO-AUTHORS:		Seah Wang Hua, wanghua.seah@digipen.edu (20%)
					Ng Juin Herng, juinherng.ng@digipen.edu (30%)

 DESCRIPTION:		JSON-based (de)serialization for level data used by the editor/runtime.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
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
	std::string prefabPath;

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

	// Approach offsets (relative to object center)
	float approachOffsetX{ 0.0f };
	float approachOffsetY{ 0.0f };
	bool hasApproachOffset2{ false };
	float approachOffset2X{ 0.0f };
	float approachOffset2Y{ 0.0f };

	// Customer table seating
	int customerSeatCapacity{ 1 };   // 1 = single-seat, 2 = double-seat

	bool hasCustomerSeatOffset2{ false };
	float customerSeatOffset2X{ 0.0f };
	float customerSeatOffset2Y{ 0.0f };

	// Customer table seat offset (relative to object center)
	bool hasCustomerSeatOffset{ false };
	float customerSeatOffsetX{ 0.0f };
	float customerSeatOffsetY{ 0.0f };

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
	std::string horizontalAlign{ "left" };

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

	// Visibility flag (per-object toggle)
	bool visible{ true };
};

// Separate struct for editor use (no need to store font size in JSON or runtime)
struct TextObjectData {
	std::string name;
	std::string fontName;
	std::string text;
	float x = 0.0f;
	float y = 0.0f;
	float scale = 1.0f;
};

// Top-level level data structure for JSON (de)serialization
struct LevelData {
	int schemaVersion{ 2 };
	std::vector<LevelObject> objects{};
	std::vector<LevelTextObject> textObjects{};  // text objects in the level
	std::string background; // optional background texture path
	std::string backgroundOverlay; // optional overlay texture path drawn above background
};

// Public Interface
struct LevelSerializer {

	/**
	 * @brief Loads this object.
	 * @param path Path to process.
	 * @param outLevel Output value for out level.
	 * @return Result produced by this operation.
	 */
	static bool Load(const std::string& path, LevelData& outLevel);

	/**
	 * @brief Loads exactly the file at the requested path without redirecting to fresher copies.
	 * @param path Filesystem path chosen by the caller.
	 * @param outLevel Output value for out level.
	 * @return True when the exact file could be parsed successfully.
	 */
	static bool LoadExact(const std::string& path, LevelData& outLevel);

	/**
	 * @brief Saves this object.
	 * @param path Path to process.
	 * @param inLevel Parameter for in level.
	 * @return Result produced by this operation.
	 */
	static bool Save(const std::string& path, const LevelData& inLevel);
};
