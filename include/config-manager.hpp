#pragma once

/**
 * @file    ConfigManager.hpp
 * @brief   Config manager for loading/saving game settings.
 *
 * File format: simple key=value pairs (INI-like), `#` for comments.
 * Example config.txt:
 *
 * # Screen settings
 * window_width=1080
 * window_height=720
 * fullscreen=false
 *
 * # Audio settings
 * bgm_volume=0.50
 * vfx_volume=0.80
 *
 * Legacy support:
 * audio_volume=0.75  # If present, sets both bgm_volume and vfx_volume
 */

#include <string>

namespace ConfigManager {
	struct Resolution {
		int width{};
		int height{};
	};

	struct Settings {
		Resolution resolution{};
		bool fullscreen{};

		// Split volumes
		float bgmVolume{};
		float vfxVolume{};
	};

	// Load settings from file. Missing keys keep existing values in 'out'.
	bool Load(const std::string& path, Settings& out);

	// Save settings to file.
	bool Save(const std::string& path, const Settings& s);

	// Clamp ranges (resolution mins; volumes in [0..1]).
	void Validate(Settings& s);

	// Convenience: returns a copy with a different resolution.
	Settings WithResolution(Settings s, int width, int height);

	// Load settings from "<exe-dir>\assets\<filename>" (defaults to "config.txt")
	bool LoadFromAssets(Settings& out, const char* filename = "config.txt");

	inline Settings LoadFromAssetsOrDefaults(const char* filename = "config.txt") {
		Settings s;
		(void)LoadFromAssets(s, filename);
		return s;
	}
};
