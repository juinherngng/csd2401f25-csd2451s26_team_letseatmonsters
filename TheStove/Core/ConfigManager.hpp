/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			ConfigManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		This module defines a minimal configuration schema (window resolution, fullscreen,
					BGM/VFX volumes) and exposes helpers to load from a file or common asset locations,
					save back to disk, and validate/clamp values to safe ranges.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

namespace ConfigManager {
	/**
	 * @struct Resolution
	 * @brief Stores window resolution dimensions.
	 */
	struct Resolution {
		int width{};
		int height{};
	};

	/**
	 * @struct Settings
	 * @brief Stores all configurable settings for the game.
	 */
	struct Settings {
		Resolution resolution{};
		bool fullscreen{};
		float masterVolume{};
		float bgmVolume{};
		float vfxVolume{};
	};

	/**
	 * @brief Ensures settings are within valid ranges.
	 * @param[in,out] s Settings structure to validate.
	 */
	void Validate(Settings& s);

	/**
	 * @brief Returns a copy of given settings with updated resolution.
	 * @param s Existing settings.
	 * @param width Desired width.
	 * @param height Desired height.
	 * @return Modified settings with new resolution.
	 */
	Settings WithResolution(Settings s, int width, int height);

	/**
	 * @brief Loads settings from a given file path.
	 * @param path Path to the config file.
	 * @param[out] out Settings structure to populate.
	 * @return True if loaded successfully, false otherwise.
	 */
	bool Load(const std::string& filePath, Settings& out);

	/**
	 * @brief Saves settings to a given file path.
	 * @param path Path where to save.
	 * @param s Settings to write.
	 * @return True if successfully written.
	 */
	bool Save(const std::string& patfilePathh, const Settings& s);

	/**
	 * @brief Tries to load settings from `assets/config.txt` relative to executable.
	 * @param[out] out Settings structure to populate.
	 * @param filename Optional filename (defaults to "config.txt").
	 * @return True if file found and loaded.
	 */
	bool LoadFromAssets(Settings& out, const char* filename = "config.txt");

	/**
	 * @brief Loads settings from assets or falls back to defaults if not found.
	 * @return Loaded or default-initialized Settings.
	 */
	inline Settings LoadFromAssetsOrDefaults(const char* filename = "config.txt") {
		Settings s;
		LoadFromAssets(s, filename);
		return s;
	}
};
