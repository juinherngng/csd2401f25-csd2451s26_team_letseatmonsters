/*
----------------------------------------------------------------------------------------------------
 FILE NAME:         FilePaths.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Ng Juin Herng, juinherng.ng@digipen.edu (70%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (30%)

 DESCRIPTION:       Centralized file path constants for all game assets.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <string>

/**
 * @brief Centralized constants and path helpers for engine asset files.
 */
namespace FilePaths {
	/**
	 * @brief Concatenates a directory prefix and filename into a relative path.
	 * @param dir Directory prefix to prepend.
	 * @param filename File name or relative tail path.
	 * @return Concatenated relative path string.
	 */
	inline std::string JoinPath(const char* dir, const std::string& filename) {
		// Keep path assembly in one helper so all higher-level builders stay consistent.
		return std::string(dir) + filename;
	}

	// ============================================================================
	// Base Directories
	// ============================================================================
	namespace Dirs {
		// Relative to build output directory (build/Debug or build/Release)
		constexpr const char* ASSETS = "../assets/";
		constexpr const char* LEVELS = "../levels/";
		constexpr const char* PREFABS = "../prefabs/";
		constexpr const char* FONTS = "../assets/Font/";
		constexpr const char* AUDIO = "../assets/Audio/";
		constexpr const char* CUTSCENES = "../assets/Cutscenes/";
		constexpr const char* VIDEOS = "../assets/Videos/";

		// Relative to deeper directories (used by editor panels running from build/Release)
		constexpr const char* ASSETS_EDITOR = "../../assets/";
		constexpr const char* PREFABS_EDITOR = "../../prefabs/";
		constexpr const char* AUDIO_EDITOR = "../../assets/Audio/";
		constexpr const char* FONTS_EDITOR = "../../assets/Font/";
		constexpr const char* LEVELS_EDITOR = "../../levels/";
	}

	// ============================================================================
	// Level Files
	// ============================================================================
	namespace Levels {
		constexpr const char* KITCHEN_01 = "../levels/kitchen01.json";
		constexpr const char* KITCHEN_02 = "../levels/kitchen02.json";
		constexpr const char* MAIN_MENU = "../levels/main_menu.json";
		constexpr const char* SETTINGS = "../levels/settings.json";
		constexpr const char* CREDITS = "../levels/credits.json";
		constexpr const char* WIN = "../levels/win.json";
		constexpr const char* LOSE = "../levels/lose.json";
	}

	// ============================================================================
	// Texture Assets
	// ============================================================================
	namespace Textures {
		// Backgrounds
		constexpr const char* BACKGROUND = "../assets/Backgrounds/Background.png";

		// UI - Pause Menu
		constexpr const char* PAUSE_BG = "../assets/pause.png";
		constexpr const char* PAUSED_BG = "../assets/UI/paused.png";
		constexpr const char* BTN_CONTINUE = "../assets/UI/continue_s.png";
		constexpr const char* BTN_RESUME = "../assets/UI/resume_s.png";
		constexpr const char* BTN_HOW = "../assets/UI/how_s.png";
		constexpr const char* BTN_QUIT = "../assets/UI/quit_s.png";
		constexpr const char* BTN_RETURN = "../assets/UI/return_s.png";

		// UI - How To Play
		constexpr const char* HOW_TO_PLAY = "../assets/UI/HowToPlay.png";

		// Placeholder/Debug
		constexpr const char* PLACEHOLDER = "../assets/Characters/mc_sprite_front.png";
	}

	// ============================================================================
	// Font Assets
	// ============================================================================
	namespace Fonts {
		constexpr const char* CHRUSTY_ROCK = "../assets/Font/ChrustyRock-ORLA.ttf";
		constexpr const char* TO_THE_POINT = "../assets/Font/ToThePointRegular-n9y4.ttf";
		constexpr const char* AGENCYB = "../assets/Font/AGENCYB.ttf";
	}

	// ============================================================================
	// Audio Assets
	// ============================================================================
	namespace Audio {
		constexpr const char* CATALOG = "../assets/Audio/AudioCatalog.json";
		constexpr const char* CATALOG_EDITOR = "../../assets/Audio/AudioCatalog.json";
	}


	// Helper functions are inline so this header can be shared without linker duplication.

	/**
	 * @brief Constructs a full path to a texture file in the assets directory.
	 * @param filename Texture filename such as `"player.png"`.
	 * @return Full relative texture path.
	 */
	inline std::string TexturePath(const std::string& filename) {
		// Route all texture-path construction through the shared directory constants.
		return JoinPath(Dirs::ASSETS, filename);
	}

	/**
	 * @brief Constructs a full path to a level file.
	 * @param filename Level filename such as `"kitchen02.json"`.
	 * @return Full relative level path.
	 */
	inline std::string LevelPath(const std::string& filename) {
		// Keep level-path construction aligned with the engine's shared level directory.
		return JoinPath(Dirs::LEVELS, filename);
	}

	/**
	 * @brief Constructs a full path to a prefab file.
	 * @param filename Prefab filename such as `"player.json"`.
	 * @return Full relative prefab path.
	 */
	inline std::string PrefabPath(const std::string& filename) {
		// Build prefab paths from the dedicated prefab directory constant.
		return JoinPath(Dirs::PREFABS, filename);
	}

	/**
	 * @brief Constructs a full path to a font file.
	 * @param filename Font filename such as `"MyFont.ttf"`.
	 * @return Full relative font path.
	 */
	inline std::string FontPath(const std::string& filename) {
		// Centralize font-path creation so text systems do not hardcode font folders.
		return JoinPath(Dirs::FONTS, filename);
	}

	/**
	 * @brief Constructs a full path to an audio file.
	 * @param filename The audio filename (e.g., "bgm_main.wav")
	 * @return Full relative path (e.g., "../assets/Audio/bgm_main.wav")
	 */
	inline std::string AudioPath(const std::string& filename) {
		// Route audio-path construction through the shared audio directory constant.
		return JoinPath(Dirs::AUDIO, filename);
	}

	/**
	 * @brief Constructs a full path to a cutscene image file.
	 * @param filename Cutscene filename such as `"Cutscene_starting_1.png"`.
	 * @return Full relative cutscene path.
	 */
	inline std::string CutscenePath(const std::string& filename) {
		// Keep cutscene-path generation aligned with the dedicated cutscene asset directory.
		return JoinPath(Dirs::CUTSCENES, filename);
	}

	/**
	 * @brief Constructs a full path to a video file stored under the shared videos directory.
	 * @param filename Video filename such as `"intro.mp4"`.
	 * @return Full relative path such as `"../assets/Videos/intro.mp4"`.
	 */
	inline std::string VideoPath(const std::string& filename) {
		// Build video paths from the shared videos root to avoid scattered string literals.
		return JoinPath(Dirs::VIDEOS, filename);
	}

} // namespace FilePaths
