/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelSerializer.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu		(70%)
 CO-AUTHORS:		Ng Juin Herng, juinherng.ng@digipen.edu (20%)
					Vu Phan Hung, phanhung.vu@digipen.edu   (10%)

 DESCRIPTION:		Handles saving and loading of LevelData to and from JSON files.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <filesystem>
#include <fstream>
#include <optional>
#include <unordered_set>

#include "EngineCore/JSONInclude.hpp"
#include "EngineCore/LevelSerializer.hpp"
#include "EngineCore/Logger.hpp"

namespace fs = std::filesystem;

using nlohmann::json;

namespace {
	constexpr int LEVEL_SCHEMA_VERSION = 2;

	/**
	 * @brief Resolves freshest level path.
	 * @param path Path to process.
	 * @return Result produced by this operation.
	 */
	std::optional<fs::path> ResolveFreshestLevelPath(const std::string& path) {
		std::vector<fs::path> candidates;
		candidates.emplace_back(path);

		const fs::path inputPath(path);
		const fs::path fileName = inputPath.filename();
		if (!fileName.empty()) {
			candidates.emplace_back(fs::path("../levels") / fileName);
			candidates.emplace_back(fs::path("../../levels") / fileName);
			candidates.emplace_back(fs::path("levels") / fileName);
		}

		std::unordered_set<std::string> seen;
		std::optional<fs::path> freshest;
		fs::file_time_type freshestTime{};

		for (const fs::path& candidate : candidates) {
			std::error_code ec;
			const fs::path normalized = candidate.lexically_normal();
			const std::string key = normalized.string();
			if (!seen.insert(key).second) {
				continue;
			}

			if (!fs::exists(normalized, ec) || ec) {
				continue;
			}

			const fs::file_time_type modified = fs::last_write_time(normalized, ec);
			if (ec) {
				continue;
			}

			if (!freshest || modified >= freshestTime) {
				freshest = normalized;
				freshestTime = modified;
			}
		}

		return freshest;
	}

	/**
	 * @brief Applies legacy migrations.
	 * @param jsonData Parameter for json data.
	 * @param schemaVersion Parameter for schema version.
	 */
	void ApplyLegacyMigrations(json& jsonData, int schemaVersion) {
		if (schemaVersion < 1) {
			if (!jsonData.contains("textObjects") && jsonData.contains("text_objects")) {
				jsonData["textObjects"] = jsonData["text_objects"];
			}
		}

		if (schemaVersion < 2 && jsonData.contains("objects") && jsonData["objects"].is_array()) {
			for (auto& jsonObj : jsonData["objects"]) {
				if (!jsonObj.is_object()) {
					continue;
				}

				if (!jsonObj.contains("prefab_path") && jsonObj.contains("prefabPath")) {
					jsonObj["prefab_path"] = jsonObj["prefabPath"];
				}
			}
		}

		if (schemaVersion < 2 && jsonData.contains("textObjects") && jsonData["textObjects"].is_array()) {
			for (auto& jsonObj : jsonData["textObjects"]) {
				if (!jsonObj.is_object()) {
					continue;
				}
				if (!jsonObj.contains("visible")) {
					jsonObj["visible"] = true;
				}
			}
		}
	}
} // namespace

/**
 * @brief Reads level object.
 * @param jsonObj Parameter for json obj.
 * @return Result produced by this operation.
 */
static LevelObject ReadLevelObject(const json& jsonObj) {
	LevelObject obj{};

	obj.texture = jsonObj.value("texture", "");
	obj.tag = jsonObj.value("tag", "");
	obj.layer = jsonObj.value("layer", "");
	obj.prefabPath = jsonObj.value("prefab_path", "");

	obj.x = jsonObj.value("x", 0.0f);
	obj.y = jsonObj.value("y", 0.0f);
	obj.z = jsonObj.value("z", 0.0f);
	obj.w = jsonObj.value("w", 128.0f);
	obj.h = jsonObj.value("h", 128.0f);

	// Stored in degrees for editor friendliness
	obj.rotation = jsonObj.value("rotation", 0.0f);
	obj.hasCollider = jsonObj.value("has_collider", true);

	obj.colWidth = jsonObj.value("col_w", 64.0f);
	obj.colHeight = jsonObj.value("col_h", 128.0f);
	obj.colOffsetX = jsonObj.value("col_offx", 0.0f);
	obj.colOffsetY = jsonObj.value("col_offy", 0.0f);

	if (!obj.hasCollider) {
		obj.colWidth = 0.0f;
		obj.colHeight = 0.0f;
		obj.colOffsetX = 0.0f;
		obj.colOffsetY = 0.0f;
	}

	obj.speedX = jsonObj.value("speed_x", 0.0f);
	obj.speedY = jsonObj.value("speed_y", 0.0f);

	obj.animated = jsonObj.value("animated", false);
	obj.animName = jsonObj.value("anim_name", "");

	// Approach offsets (safe for existing JSON)
	obj.approachOffsetX = jsonObj.value("approach_offx", 0.0f);
	obj.approachOffsetY = jsonObj.value("approach_offy", 0.0f);
	obj.hasApproachOffset2 = jsonObj.contains("approach2_offx") || jsonObj.contains("approach2_offy");
	obj.approachOffset2X = jsonObj.value("approach2_offx", 0.0f);
	obj.approachOffset2Y = jsonObj.value("approach2_offy", 0.0f);
	obj.customerSeatCapacity = jsonObj.value("customer_seat_capacity", 1);

	obj.hasCustomerSeatOffset2 = jsonObj.value("has_customer_seat2", false);
	if (!jsonObj.contains("has_customer_seat2")) {
		// Legacy fallback: infer second seat only when authored offsets are non-zero.
		obj.hasCustomerSeatOffset2 =
			(jsonObj.value("customer_seat2_offx", 0.0f) != 0.0f) ||
			(jsonObj.value("customer_seat2_offy", 0.0f) != 0.0f);
	}

	obj.customerSeatOffset2X = jsonObj.value("customer_seat2_offx", 0.0f);
	obj.customerSeatOffset2Y = jsonObj.value("customer_seat2_offy", 0.0f);

	// Optional explicit customer seating offset
	obj.hasCustomerSeatOffset = jsonObj.contains("customer_seat_offx") || jsonObj.contains("customer_seat_offy");
	obj.customerSeatOffsetX = jsonObj.value("customer_seat_offx", 0.0f);
	obj.customerSeatOffsetY = jsonObj.value("customer_seat_offy", 0.0f);

	// Audio bindings (safe for existing JSON, defaults to empty)
	obj.audioOnSpawn = jsonObj.value("audio_on_spawn", "");
	obj.audioOnInteract = jsonObj.value("audio_on_interact", "");
	obj.audioOnDestroy = jsonObj.value("audio_on_destroy", "");
	obj.audioOnProcessing = jsonObj.value("audio_on_processing", "");
	obj.audioLoop = jsonObj.value("audio_loop", false);

	obj.shadow = jsonObj.value("shadow", false);

	// Per-object visibility (defaults true for backward compatibility)
	obj.visible = jsonObj.value("visible", true);

	return obj;
}

/**
 * @brief Reads text object.
 * @param jsonObj Parameter for json obj.
 * @return Result produced by this operation.
 */
static LevelTextObject ReadTextObject(const json& jsonObj) {
	LevelTextObject obj{};

	obj.name = jsonObj.value("name", "");
	obj.text = jsonObj.value("text", "");
	obj.fontName = jsonObj.value("fontName", "");
	obj.fontSize = jsonObj.value("fontSize", 48u);
	obj.horizontalAlign = jsonObj.value("horizontalAlign", "left");

	obj.x = jsonObj.value("x", 0.0f);
	obj.y = jsonObj.value("y", 0.0f);
	obj.scale = jsonObj.value("scale", 1.0f);
	obj.rotation = jsonObj.value("rotation", 0.0f);
	obj.useBlockRotation = jsonObj.value("useBlockRotation", true);

	obj.colorR = jsonObj.value("colorR", 1.0f);
	obj.colorG = jsonObj.value("colorG", 1.0f);
	obj.colorB = jsonObj.value("colorB", 1.0f);
	obj.colorA = jsonObj.value("colorA", 1.0f);

	obj.layer = jsonObj.value("layer", "1");
	obj.visible = jsonObj.value("visible", true);

	return obj;
}

/**
 * @brief Writes level object.
 * @param obj Parameter for obj.
 * @return Result produced by this operation.
 */
static json WriteLevelObject(const LevelObject& obj) {
	json jsonData = {
		{"texture", obj.texture},
		{"tag", obj.tag},
		{"layer", obj.layer},
		{"prefab_path", obj.prefabPath},
		{"x", obj.x},
		{"y", obj.y},
		{"z", obj.z},
		{"w", obj.w},
		{"h", obj.h},
		{"rotation", obj.rotation},
		{"has_collider", obj.hasCollider},
		{"col_w", obj.colWidth},
		{"col_h", obj.colHeight},
		{"col_offx", obj.colOffsetX},
		{"col_offy", obj.colOffsetY},
		{"speed_x", obj.speedX},
		{"speed_y", obj.speedY},
		{"animated", obj.animated},
		{"anim_name", obj.animName},
		{"shadow", obj.shadow},
		// Approach offsets
		{"approach_offx", obj.approachOffsetX},
		{"approach_offy", obj.approachOffsetY},
		{"approach2_offx", obj.approachOffset2X},
		{"approach2_offy", obj.approachOffset2Y},
		// Optional customer seating offset
		{"customer_seat_offx", obj.customerSeatOffsetX},
		{"customer_seat_offy", obj.customerSeatOffsetY},
		{"customer_seat_capacity", obj.customerSeatCapacity},
		{"has_customer_seat2", obj.hasCustomerSeatOffset2},
		{"customer_seat2_offx", obj.customerSeatOffset2X},
		{"customer_seat2_offy", obj.customerSeatOffset2Y},
		// Audio bindings
		{"audio_on_spawn", obj.audioOnSpawn},
		{"audio_on_interact", obj.audioOnInteract},
		{"audio_on_destroy", obj.audioOnDestroy},
		{"audio_on_processing", obj.audioOnProcessing},
		{"audio_loop", obj.audioLoop},
		// Per-object visibility
		{"visible", obj.visible},
		// Shadow flag
		{"shadow", obj.shadow} };

	return jsonData;
}

/**
 * @brief Writes text object.
 * @param obj Parameter for obj.
 * @return Result produced by this operation.
 */
static json WriteTextObject(const LevelTextObject& obj) {
	json jsonData = {
		{"name", obj.name},
		{"text", obj.text},
		{"fontName", obj.fontName},
		{"fontSize", obj.fontSize},
		{"horizontalAlign", obj.horizontalAlign},
		{"x", obj.x},
		{"y", obj.y},
		{"scale", obj.scale},
		{"rotation", obj.rotation},
		{"useBlockRotation", obj.useBlockRotation},
		{"colorR", obj.colorR},
		{"colorG", obj.colorG},
		{"colorB", obj.colorB},
		{"colorA", obj.colorA},
		{"layer", obj.layer},
		{"visible", obj.visible} };

	return jsonData;
}

/**
 * @brief Loads level data from an already-resolved path.
 * @param resolvedPath Concrete filesystem path to parse.
 * @param outLevel Output value for out level.
 * @return True when the file exists and parses into level data.
 */
static bool LoadFromResolvedPath(const fs::path& resolvedPath, LevelData& outLevel) {
	std::ifstream file(resolvedPath);
	if (!file) {
		return false;
	}

	json jsonData;
	try {
		file >> jsonData;
	}
	catch (const json::parse_error&) {
		return false;
	}

	outLevel.objects.clear();
	outLevel.textObjects.clear();
	outLevel.background.clear();
	outLevel.backgroundOverlay.clear();

	outLevel.schemaVersion = jsonData.value("schema_version", 0);
	ApplyLegacyMigrations(jsonData, outLevel.schemaVersion);
	if (outLevel.schemaVersion > LEVEL_SCHEMA_VERSION) {
		TS_LOG_WARN("[LevelSerializer] Loading newer schema version " << outLevel.schemaVersion
			<< " with reader version " << LEVEL_SCHEMA_VERSION);
	}

	outLevel.schemaVersion = LEVEL_SCHEMA_VERSION;

	// optional background
	outLevel.background = jsonData.value("background", "");
	outLevel.backgroundOverlay = jsonData.value("background_overlay", "");

	if (jsonData.contains("objects") && jsonData["objects"].is_array()) {
		outLevel.objects.reserve(jsonData["objects"].size());
		for (const auto& jsonObj : jsonData["objects"]) {
			outLevel.objects.push_back(ReadLevelObject(jsonObj));
		}
	}

	// Load text objects if present
	if (jsonData.contains("textObjects") && jsonData["textObjects"].is_array()) {
		outLevel.textObjects.reserve(jsonData["textObjects"].size());
		for (const auto& jsonObj : jsonData["textObjects"]) {
			outLevel.textObjects.push_back(ReadTextObject(jsonObj));
		}
	}

	return true;
}

/**
 * @brief Loads this object.
 * @param path Path to process.
 * @param outLevel Output value for out level.
 * @return Result produced by this operation.
 */
bool LevelSerializer::Load(const std::string& path, LevelData& outLevel) {
	const std::optional<fs::path> resolvedPath = ResolveFreshestLevelPath(path);
	if (!resolvedPath) {
		return false;
	}

	return LoadFromResolvedPath(*resolvedPath, outLevel);
}

/**
 * @brief Loads exactly the file at the requested path.
 * @param path Path to process.
 * @param outLevel Output value for out level.
 * @return True when the exact file could be parsed.
 */
bool LevelSerializer::LoadExact(const std::string& path, LevelData& outLevel) {
	return LoadFromResolvedPath(fs::path(path), outLevel);
}

/**
 * @brief Saves this object.
 * @param path Path to process.
 * @param inLevel Parameter for in level.
 * @return Result produced by this operation.
 */
bool LevelSerializer::Save(const std::string& path, const LevelData& inLevel) {
	json jsonData = json::object();

	std::error_code ec;
	const fs::path outputPath(path);
	const fs::path parentDir = outputPath.parent_path();
	if (!parentDir.empty() && !fs::exists(parentDir, ec)) {
		fs::create_directories(parentDir, ec);
		if (ec) {
			return false;
		}
	}

	// Try to load existing JSON to preserve unrelated keys
	{
		std::ifstream in(path);
		if (in) {
			try {
				in >> jsonData;
			}
			catch (const json::parse_error&) {
				jsonData = json::object();
			}
		}
	}

	// Write background if present
	if (!inLevel.background.empty()) {
		jsonData["background"] = inLevel.background;
	}
	else {
		// Optional: erase background if you want to remove it
		// jsonData.erase("background");
	}

	if (!inLevel.backgroundOverlay.empty()) {
		jsonData["background_overlay"] = inLevel.backgroundOverlay;
	}

	jsonData["schema_version"] = LEVEL_SCHEMA_VERSION;

	// Replace ONLY the "objects" array
	jsonData["objects"] = json::array();
	for (const auto& obj : inLevel.objects) {
		jsonData["objects"].push_back(WriteLevelObject(obj));
	}

	// Replace the "textObjects" array
	jsonData["textObjects"] = json::array();

	for (const auto& textObj : inLevel.textObjects) {
		jsonData["textObjects"].push_back(WriteTextObject(textObj));
	}

	std::ofstream file(path);
	if (!file)
		return false;
	file << jsonData.dump(2);
	return true;
}
