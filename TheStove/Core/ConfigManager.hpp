/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			ConfigManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		This module defines a minimal configuration schema (window resolution, fullscreen,
					BGM/VFX volumes) and exposes helpers to load from a file or common asset locations,
					save back to disk, and validate/clamp values to safe ranges.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
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

	/**
	 * @brief Validates this object.
	 * @param s Parameter for s.
	 */
	void Validate(Settings& s);

	/**
	 * @brief Performs with resolution.
	 * @param s Parameter for s.
	 * @param width Width value in pixels.
	 * @param height Height value in pixels.
	 * @return Result produced by this operation.
	 */
	Settings WithResolution(Settings s, int width, int height);

	/**
	 * @brief Performs with fullscreen.
	 * @param s Parameter for s.
	 * @param fullscreenEnabled Parameter for fullscreen enabled.
	 * @return Result produced by this operation.
	 */
	Settings WithFullscreen(Settings s, bool fullscreenEnabled);

	/**
	 * @brief Performs with volumes.
	 * @param s Parameter for s.
	 * @param master Parameter for master.
	 * @param bgm Parameter for bgm.
	 * @param vfx Parameter for vfx.
	 * @return Result produced by this operation.
	 */
	Settings WithVolumes(Settings s, float master, float bgm, float vfx);

	/**
	 * @brief Loads this object.
	 * @param filePath Path to the target file.
	 * @param out Output value for out.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool Load(const std::string& filePath, Settings& out);

	/**
	 * @brief Saves this object.
	 * @param filePath Path to the target file.
	 * @param s Parameter for s.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool Save(const std::string& filePath, const Settings& s);

	/**
	 * @brief Loads from assets.
	 * @param out Output value for out.
	 * @param filename Parameter for filename.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool LoadFromAssets(Settings& out, const char* filename = "config.txt");

	/**
	 * @brief Resolves asset path.
	 * @param outFilePath Output value for out file path.
	 * @param filename Parameter for filename.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool ResolveAssetPath(std::string& outFilePath, const char* filename = "config.txt");

	/**
	 * @brief Loads from assets or defaults.
	 * @param filename Parameter for filename.
	 * @return Result produced by this operation.
	 */
	inline Settings LoadFromAssetsOrDefaults(const char* filename = "config.txt") {
		Settings s;
		// Set default fullscreen to true
		s.fullscreen = true;
		LoadFromAssets(s, filename);
		return s;
	}
};

