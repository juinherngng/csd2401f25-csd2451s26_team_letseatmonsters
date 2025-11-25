/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			ConfigManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Definition of ConfigManager for loading/saving game settings.
					The configuration file uses a simple key=value format.

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <windows.h>

#include "ConfigManager.hpp"

namespace fs = std::filesystem;

namespace ConfigManager {
	namespace {
		/** @brief Trims whitespace from both ends of a string. */
		std::string Trim(std::string str) {
			auto isNotSpace = [](unsigned char ch) { return !std::isspace(ch); };

			auto beginIt = std::find_if(str.begin(), str.end(), isNotSpace);
			str.erase(str.begin(), beginIt);

			auto rbeginIt = std::find_if(str.rbegin(), str.rend(), isNotSpace);
			auto rendBase = rbeginIt.base();
			str.erase(rendBase, str.end());

			return str;
		}

		/** @brief Case-insensitive comparison of two strings. */
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

		/** @brief Parses a string into bool. */
		bool ParseBool(const std::string& str, bool& valueOut) {
			if (IEquals(str, "true") || str == "1") {
				valueOut = true; return true;
			}

			if (IEquals(str, "false") || str == "0") {
				valueOut = false; return true;
			}

			return false;
		}

		/** @brief Parses a string into int. */
		bool ParseInt(const std::string& str, int& valueOut) {
			try {
				size_t pos = 0;
				long v = std::stol(str, &pos, 10);
				if (pos != str.size()) return false;
				valueOut = static_cast<int>(v);
				return true;
			}

			catch (...) {
				return false;
			}
		}

		/** @brief Parses a string into float. */
		bool ParseFloat(const std::string& str, float& valueOut) {
			try {
				size_t pos = 0;
				float v = std::stof(str, &pos);
				if (pos != str.size()) return false;
				valueOut = v;
				return true;
			}

			catch (...) {
				return false;
			}
		}
	}

	void Validate(Settings& s) {
		// Enforce minimum resolution
		if (s.resolution.width < 320) s.resolution.width = 320;
		if (s.resolution.height < 200) s.resolution.height = 200;

		// Clamp master volume
		if (s.masterVolume < 0.0f) {
			s.masterVolume = 0.0f;
		}
		if (s.masterVolume > 1.0f) {
			s.masterVolume = 1.0f;
		}

		// Clamp BGM volume
		if (s.bgmVolume < 0.0f) {
			s.bgmVolume = 0.0f;
		}
		if (s.bgmVolume > 1.0f) {
			s.bgmVolume = 1.0f;
		}
		
		// Clamp VFX volume
		if (s.vfxVolume < 0.0f) {
			s.vfxVolume = 0.0f;
		}
		if (s.vfxVolume > 1.0f) {
			s.vfxVolume = 1.0f;
		}
	}

	Settings WithResolution(Settings s, int width, int height) {
		s.resolution.width = width;
		s.resolution.height = height;
		Validate(s);
		return s;
	}

	bool Load(const std::string& filePath, Settings& out) {
		std::ifstream ifs(filePath);
		if (!ifs.is_open()) {
			return false;
		}

		Settings cfg = out; // start from existing (keeps defaults if keys missing)
		std::string lineBuf;

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

			if (key == "window_width") {
				int parsed = 0;
				if (ParseInt(val, parsed)) {
					cfg.resolution.width = parsed;
				}
			}
			else if (key == "window_height") {
				int parsed = 0;
				if (ParseInt(val, parsed)) {
					cfg.resolution.height = parsed;
				}
			}
			else if (key == "fullscreen") {
				bool parsed = false;
				if (ParseBool(val, parsed)) {
					cfg.fullscreen = parsed;
				}
			}
			else if (key == "master_volume") {
				float parsed = 0.0f;
				if (ParseFloat(val, parsed)) {
					cfg.masterVolume = parsed;
				}
			}
			else if (key == "bgm_volume") {
				float parsed = 0.0f;
				if (ParseFloat(val, parsed)) {
					cfg.bgmVolume = parsed;
				}
			}
			else if (key == "vfx_volume") {
				float parsed = 0.0f;
				if (ParseFloat(val, parsed)) {
					cfg.vfxVolume = parsed;
				}
			}
			else if (key == "audio_volume") {
				// Optional convenience: applies to both BGM and VFX if present.
				float parsed = 0.0f;
				if (ParseFloat(val, parsed)) {
					cfg.bgmVolume = parsed;
					cfg.vfxVolume = parsed;
				}
			}
			else {
				// Unknown keys are ignored (forward compatible).
			}
		}

		Validate(cfg);
		out = cfg;

		return true;
	}

	bool Save(const std::string& filePath, const Settings& s) {
		std::ofstream ofs(filePath, std::ios::trunc);
		if (!ofs.is_open()) {
			return false;
		}

		ofs << "window_width=" << s.resolution.width << "\n";
		ofs << "window_height=" << s.resolution.height << "\n";
		ofs << "fullscreen=" << (s.fullscreen ? "true" : "false") << "\n";
		ofs << "master_volume=" << s.masterVolume << "\n";
		ofs << "bgm_volume=" << s.bgmVolume << "\n";
		ofs << "vfx_volume=" << s.vfxVolume << "\n";
		ofs.flush();

		return static_cast<bool>(ofs);
	}

	bool LoadFromAssets(Settings& out, const char* filename) {
		const char* fname = filename?filename:"config.txt";

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
