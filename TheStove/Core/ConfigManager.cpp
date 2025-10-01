/*
  ----------------------------------------------------------------------------------------------------
  FILE NAME:		ConfigManager.cpp
  PROJECT NAME:		Project GAM200
  AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

  DESCRIPTION:		Implementation of ConfigManager for loading/saving game settings from text files.
					File format: simple key=value pairs (INI-like), `#` for comments.

		  All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
  ----------------------------------------------------------------------------------------------------
  */

#include "ConfigManager.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <windows.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace ConfigManager {
	namespace {
		/** @brief Trims whitespace from both ends of a string. */
		std::string Trim(std::string s) {
			auto notSpace = [](unsigned char ch) {
				return !std::isspace(ch);
				};

			s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
			s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());

			return s;
		}

		/** @brief Case-insensitive comparison of two strings. */
		bool IEquals(const std::string& a, const std::string& b) {
			if (a.size() != b.size()) {
				return false;
			}

			for (size_t i = 0; i < a.size(); ++i) {
				if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
					return false;
				}
			}

			return true;
		}

		/** @brief Parses a string into bool. */
		bool ParseBool(const std::string& s, bool& out) {
			if (IEquals(s, "true") || s == "1") {
				out = true; return true;
			}

			if (IEquals(s, "false") || s == "0") {
				out = false; return true;
			}

			return false;
		}

		/** @brief Parses a string into int. */
		bool ParseInt(const std::string& s, int& out) {
			try {
				size_t pos = 0;
				long v = std::stol(s, &pos, 10);
				if (pos != s.size()) return false;
				out = static_cast<int>(v);
				return true;
			}

			catch (...) {
				return false;
			}
		}

		/** @brief Parses a string into float. */
		bool ParseFloat(const std::string& s, float& out) {
			try {
				size_t pos = 0;
				float v = std::stof(s, &pos);
				if (pos != s.size()) return false;
				out = v;
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

		// Clamp volumes
		if (s.bgmVolume < 0.f) s.bgmVolume = 0.f;
		if (s.bgmVolume > 1.f) s.bgmVolume = 1.f;
		if (s.vfxVolume < 0.f) s.vfxVolume = 0.f;
		if (s.vfxVolume > 1.f) s.vfxVolume = 1.f;
	}

	Settings WithResolution(Settings s, int width, int height) {
		s.resolution.width = width;
		s.resolution.height = height;
		Validate(s);
		return s;
	}

	bool Load(const std::string& path, Settings& out) {
		std::ifstream in(path);
		if (!in.is_open()) {
			return false;
		}

		Settings tmp = out;
		std::string line;

		while (std::getline(in, line)) {
			// Remove comments
			std::string::size_type pos = line.find('#');
			if (pos != std::string::npos) {
				line.erase(pos);
			}

			line = Trim(line);
			if (line.empty()) {
				continue;
			}

			std::string::size_type eq = line.find('=');
			if (eq == std::string::npos) {
				continue;
			}

			std::string key = Trim(line.substr(0, eq));
			std::string val = Trim(line.substr(eq + 1));

			if (key == "window_width") {
				int w;
				if (ParseInt(val, w)) tmp.resolution.width = w;
			}
			else if (key == "window_height") {
				int h;
				if (ParseInt(val, h)) tmp.resolution.height = h;
			}
			else if (key == "fullscreen") {
				bool b;
				if (ParseBool(val, b)) tmp.fullscreen = b;
			}
			else if (key == "bgm_volume") {
				float v; if (ParseFloat(val, v)) tmp.bgmVolume = v;
			}
			else if (key == "vfx_volume") {
				float v; if (ParseFloat(val, v)) tmp.vfxVolume = v;
			}
			else if (key == "audio_volume") {
				float v;
				if (ParseFloat(val, v)) { tmp.bgmVolume = v; tmp.vfxVolume = v; }
			}
			else {
				// Unknown keys ignored
			}
		}

		Validate(tmp);
		out = tmp;
		return true;
	}

	bool Save(const std::string& path, const Settings& s) {
		std::ofstream out(path, std::ios::trunc);
		if (!out.is_open()) {
			return false;
		}

		out << "window_width=" << s.resolution.width << "\n";
		out << "window_height=" << s.resolution.height << "\n";
		out << "fullscreen=" << (s.fullscreen ? "true" : "false") << "\n";
		out << "bgm_volume=" << s.bgmVolume << "\n";
		out << "vfx_volume=" << s.vfxVolume << "\n";
		out.flush();
		return static_cast<bool>(out);
	}

	bool LoadFromAssets(Settings& out, const char* filename) {
		const char* fname = filename ? filename : "config.txt";

		char exePath[MAX_PATH]{};
		if (!GetModuleFileNameA(nullptr, exePath, MAX_PATH)) {
			return false;
		}

		fs::path exeDir = fs::path(exePath).parent_path();

		std::vector<fs::path> candidates = {
			exeDir / "assets" / fname,
			exeDir.parent_path() / "assets" / fname,
			exeDir.parent_path().parent_path() / "assets" / fname,
			exeDir / fname
		};

		for (const auto& p : candidates) {
			std::error_code ec;
			if (fs::exists(p, ec)) {
				return Load(p.string(), out);
			}
		}

		return false;
	}
}
