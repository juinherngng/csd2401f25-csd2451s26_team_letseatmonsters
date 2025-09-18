/**
 * @file  config-manager.hpp
 * @brief Declaration of ConfigManager for loading/saving game settings.
 *
 * The configuration file uses a simple key=value format.
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
		float bgmVolume{};
		float vfxVolume{};
	};

	// Clamp ranges (resolution mins; volumes in [0..1]).
	void Validate(Settings& s);

	// Returns a copy of given settings with updated resolution.
	Settings WithResolution(Settings s, int width, int height);

	// Load settings from file. Missing keys keep existing values in 'out'.
	bool Load(const std::string& path, Settings& out);

	// Save settings to file.
	bool Save(const std::string& path, const Settings& s);

	// Load settings from "<exe-dir>\assets\<filename>" (defaults to "config.txt")
	bool LoadFromAssets(Settings& out, const char* filename = "config.txt");

	inline Settings LoadFromAssetsOrDefaults(const char* filename = "config.txt") {
		Settings s{ {1600, 900}, false, 0.8f, 0.8f };
		LoadFromAssets(s, filename);
		return s;
	}
};
