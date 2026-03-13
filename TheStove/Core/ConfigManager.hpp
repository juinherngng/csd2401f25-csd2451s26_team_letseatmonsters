/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			ConfigManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		This module defines a minimal configuration schema (window resolution, fullscreen,
					BGM/VFX volumes) and exposes helpers to load from a file or common asset locations,
					save back to disk, and validate/clamp values to safe ranges.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

namespace ConfigManager {
	// Forward declare Resolution struct for use in Settings.
	struct Resolution {
		int width{};
		int height{};
	};

	// Configuration structure representing all configurable settings for the application.
	struct Settings {
		int schemaVersion{ 1 };
		Resolution resolution{};
		bool fullscreen{};
		float masterVolume{};
		float bgmVolume{};
		float vfxVolume{};
	};

	// Current config schema version. Increment this when making breaking changes to the config format.
	void Validate(Settings& s);

	// Helper functions to create modified copies of Settings with specific fields updated.
	Settings WithResolution(Settings s, int width, int height);
	Settings WithFullscreen(Settings s, bool fullscreenEnabled);
	Settings WithVolumes(Settings s, float master, float bgm, float vfx);

	// Load settings from a file at the given path. Returns true if successful, false if file not found or parse error.
	bool Load(const std::string& filePath, Settings& out);
	bool Save(const std::string& filePath, const Settings& s);

	// Attempts to load settings from common asset locations (e.g. "assets/config.txt" or parent directories).
	bool LoadFromAssets(Settings& out, const char* filename = "config.txt");

	// Resolves the first config file path that would be used by LoadFromAssets.
	// Returns true when an existing file is found and writes it to outFilePath.
	bool ResolveAssetPath(std::string& outFilePath, const char* filename = "config.txt");

	// Convenience function to load settings from assets with defaults applied. Defaults are used if file is missing or keys are missing.
	inline Settings LoadFromAssetsOrDefaults(const char* filename = "config.txt") {
		Settings s;
		// Set default fullscreen to true
		s.fullscreen = true;
		LoadFromAssets(s, filename);
		return s;
	}
};
