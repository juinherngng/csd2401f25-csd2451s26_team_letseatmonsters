/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			ConfigManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Definition of ConfigManager for loading/saving game settings.
					The configuration file uses a simple key=value format.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "ConfigManager.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>
#include <windows.h>

namespace fs = std::filesystem;

namespace ConfigManager {
	namespace {
		constexpr int CONFIG_SCHEMA_VERSION = 1;

		// Helper to trim leading and trailing whitespace from a string
		std::string Trim(std::string str) {
			auto isNotSpace = [](unsigned char ch) { return !std::isspace(ch); };

			auto beginIt = std::find_if(str.begin(), str.end(), isNotSpace);
			str.erase(str.begin(), beginIt);

			auto rbeginIt = std::find_if(str.rbegin(), str.rend(), isNotSpace);
			auto rendBase = rbeginIt.base();
			str.erase(rendBase, str.end());

			return str;
		}

		// Case-insensitive string comparison
		bool IEquals(const std::string& lhs, const std::string& rhs) {
			if (lhs.size() != rhs.size()) return false;
			for (size_t i = 0; i < lhs.size(); ++i) {
				if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
					std::tolower(static_cast<unsigned char>(rhs[i]))) {
					return false;
				}
			}
			return true;
		}

		// Parses a string into bool. Accepts "true"/"false" (case-insensitive) or "1"/"0".
		bool ParseBool(const std::string& str, bool& valueOut) {
			if (IEquals(str, "true") || str == "1") {
				valueOut = true; return true;
			}

			if (IEquals(str, "false") || str == "0") {
				valueOut = false; return true;
			}

			return false;
		}

		// Parses a string into int. Returns false if parsing fails or if there are extra characters.
		bool ParseInt(const std::string& str, int& valueOut) {
			try {
				size_t pos = 0;
				const long parsedValue = std::stol(str, &pos, 10);
				if (pos != str.size()) return false;
				valueOut = static_cast<int>(parsedValue);
				return true;
			}

			catch (const std::invalid_argument&) {
				return false;
			}
			catch (const std::out_of_range&) {
				return false;
			}
		}

		// Parses a string into float. Returns false if parsing fails or if there are extra characters.
		bool ParseFloat(const std::string& str, float& valueOut) {
			if (!str.empty() && str.back() == '%') {
				float percentValue = 0.0f;
				if (!ParseFloat(str.substr(0, str.size() - 1), percentValue)) {
					return false;
				}

				valueOut = percentValue / 100.0f;
				return true;
			}

			try {
				size_t pos = 0;
				const float parsedValue = std::stof(str, &pos);
				if (pos != str.size()) return false;
				valueOut = parsedValue;
				return true;
			}

			catch (const std::invalid_argument&) {
				return false;
			}
			catch (const std::out_of_range&) {
				return false;
			}
		}

		// Applies a single key=value setting to the config struct. Returns true if the key is recognized.
		bool ApplySetting(const std::string& key, const std::string& val, Settings& cfg) {
			int parsedInt = 0;
			bool parsedBool = false;
			float parsedFloat = 0.0f;

			if (key == "schema_version") {
				if (ParseInt(val, parsedInt)) cfg.schemaVersion = parsedInt;
				return true;
			}
			if (key == "window_width") {
				if (ParseInt(val, parsedInt)) cfg.resolution.width = parsedInt;
				return true;
			}
			if (key == "window_height") {
				if (ParseInt(val, parsedInt)) cfg.resolution.height = parsedInt;
				return true;
			}
			if (key == "fullscreen") {
				if (ParseBool(val, parsedBool)) cfg.fullscreen = parsedBool;
				return true;
			}
			if (key == "master_volume") {
				if (ParseFloat(val, parsedFloat)) cfg.masterVolume = parsedFloat;
				return true;
			}
			if (key == "bgm_volume") {
				if (ParseFloat(val, parsedFloat)) cfg.bgmVolume = parsedFloat;
				return true;
			}
			if (key == "vfx_volume") {
				if (ParseFloat(val, parsedFloat)) cfg.vfxVolume = parsedFloat;
				return true;
			}

			if (key == "audio_volume") {
				if (ParseFloat(val, parsedFloat)) {
					// Optional convenience: applies to both BGM and VFX if present.
					cfg.bgmVolume = parsedFloat;
					cfg.vfxVolume = parsedFloat;
				}

				return true;
			}

			return false;
		}
	}

	// Enforces valid ranges and constraints on settings(e.g.minimum resolution, volume clamping)
	void Validate(Settings& s) {
		// Enforce minimum resolution
		if (s.resolution.width < 320) s.resolution.width = 320;
		if (s.resolution.height < 200) s.resolution.height = 200;

		// Clamp master volume
		const auto clamp01 = [](float value) {
			return std::clamp(value, 0.0f, 1.0f);
			};

		s.masterVolume = clamp01(s.masterVolume);
		s.bgmVolume = clamp01(s.bgmVolume);
		s.vfxVolume = clamp01(s.vfxVolume);
	}

	// Fluent setters for chaining (returns modified copy)
	Settings WithResolution(Settings s, int width, int height) {
		s.resolution.width = width;
		s.resolution.height = height;
		Validate(s);
		return s;
	}

	// Fluent setter for fullscreen
	Settings WithFullscreen(Settings s, bool fullscreenEnabled) {
		s.fullscreen = fullscreenEnabled;
		Validate(s);
		return s;
	}

	// Fluent setter for volumes
	Settings WithVolumes(Settings s, float master, float bgm, float vfx) {
		s.masterVolume = master;
		s.bgmVolume = bgm;
		s.vfxVolume = vfx;
		Validate(s);
		return s;
	}

	// Loads settings from a file. Returns false if the file cannot be read. Missing keys are left unchanged in the output struct (allowing for defaults).
	bool Load(const std::string& filePath, Settings& out) {
		std::ifstream ifs(filePath);
		if (!ifs.is_open()) {
			return false;
		}

		Settings cfg = out; // start from existing (keeps defaults if keys missing)
		cfg.schemaVersion = CONFIG_SCHEMA_VERSION;
		std::string lineBuf;
		std::set<std::string> unknownKeys;

		while (std::getline(ifs, lineBuf)) {
			// Strip comments
			std::string::size_type commentPos = lineBuf.find('#');
			if (commentPos != std::string::npos) {
				lineBuf.erase(commentPos);
			}

			lineBuf = Trim(lineBuf);
			if (lineBuf.empty()) continue;

			// Split key=value
			std::string::size_type eqPos = lineBuf.find('=');
			if (eqPos == std::string::npos) continue;

			std::string key = Trim(lineBuf.substr(0, eqPos));
			std::string val = Trim(lineBuf.substr(eqPos + 1));

			if (!ApplySetting(key, val, cfg)) {
				unknownKeys.insert(key);
			}
		}

		if (!unknownKeys.empty()) {
			std::cerr << "[ConfigManager] Unknown config key(s): ";
			bool first = true;
			for (const auto& unknownKey : unknownKeys) {
				if (!first) {
					std::cerr << ", ";
				}

				std::cerr << unknownKey;
				first = false;
			}

			std::cerr << "\n";
		}

		if (cfg.schemaVersion < 1) {
			// Legacy configs without version tag are treated as v0 and migrated by keeping parsed keys.
			cfg.schemaVersion = CONFIG_SCHEMA_VERSION;
		}

		Validate(cfg);
		out = cfg;

		return true;
	}

	// Fluent setter for fullscreen
	bool Save(const std::string& filePath, const Settings& s) {
		std::ofstream ofs(filePath, std::ios::trunc);
		if (!ofs.is_open()) {
			return false;
		}

		Settings validatedSettings = s;
		Validate(validatedSettings);

		ofs << "schema_version=" << CONFIG_SCHEMA_VERSION << "\n";
		ofs << "window_width=" << validatedSettings.resolution.width << "\n";
		ofs << "window_height=" << validatedSettings.resolution.height << "\n";
		ofs << "fullscreen=" << (validatedSettings.fullscreen ? "true" : "false") << "\n";
		ofs << "master_volume=" << validatedSettings.masterVolume << "\n";
		ofs << "bgm_volume=" << validatedSettings.bgmVolume << "\n";
		ofs << "vfx_volume=" << validatedSettings.vfxVolume << "\n";
		ofs.flush();

		return static_cast<bool>(ofs);
	}

	// Attempts to load config from multiple candidate locations in the executable's directory hierarchy. Falls back to defaults if not found.
	bool LoadFromAssets(Settings& out, const char* filename) {
		const char* fname = filename ? filename : "config.txt";

		char exePath[MAX_PATH]{};
		if (!GetModuleFileNameA(nullptr, exePath, MAX_PATH)) {
			return false;
		}

		fs::path exeDir = fs::path(exePath).parent_path();

		// Candidate search order (nearest first)
		std::vector<fs::path> candidates;
		candidates.emplace_back(exeDir / "assets" / fname);
		candidates.emplace_back(exeDir.parent_path() / "assets" / fname);
		candidates.emplace_back(exeDir.parent_path().parent_path() / "assets" / fname);
		candidates.emplace_back(exeDir / fname);

		for (const auto& candidate : candidates) {
			std::error_code ec;
			if (fs::exists(candidate, ec)) {
				return Load(candidate.string(), out);
			}
		}

		return false;
	}
}
