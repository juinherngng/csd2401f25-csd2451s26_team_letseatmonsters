/*
----------------------------------------------------------------------------------------------------
 FILE NAME:         FilePaths.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:       Centralized file path constants for all game assets.

		All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <string>

/************************************************************************/
/*!
\brief
Centralized constants for all asset file paths used throughout the engine.
*/
/************************************************************************/
namespace FilePaths {

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
		constexpr const char* MAIN_MENU = "../levels/main_menu.json";
		constexpr const char* WIN = "../levels/win.json";
		constexpr const char* LOSE = "../levels/lose.json";
	}

	// ============================================================================
	// Texture Assets
	// ============================================================================
	namespace Textures {
		// Backgrounds
		constexpr const char* BACKGROUND = "../assets/Background.png";

		// UI - Pause Menu
		constexpr const char* PAUSE_BG = "../assets/pause.png";
		constexpr const char* PAUSED_BG = "../assets/paused.png";
		constexpr const char* BTN_CONTINUE = "../assets/continue_s.png";
		constexpr const char* BTN_RESUME = "../assets/resume_s.png";
		constexpr const char* BTN_HOW = "../assets/how_s.png";
		constexpr const char* BTN_QUIT = "../assets/quit_s.png";

		// UI - How To Play
		constexpr const char* HOW_TO_PLAY = "../assets/HowToPlay.png";

		// Placeholder/Debug
		constexpr const char* PLACEHOLDER = "../assets/mc_sprite_front.png";
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


	// Helper Functions (inline to avoid linker issues)

	 /************************************************************************/
	 /*!
	 \brief
	 Constructs a full path to a texture file in the assets directory.
	 \param filename
	 The texture filename (e.g., "player.png")
	 \return
	 Full relative path (e.g., "../assets/player.png")
	 */
	 /************************************************************************/
	inline std::string TexturePath(const std::string& filename) {
		return std::string(Dirs::ASSETS) + filename;
	}

	/************************************************************************/
	/*!
	\brief
	Constructs a full path to a level file.
	\param filename
	The level filename (e.g., "kitchen02.json")
	\return
	Full relative path (e.g., "../levels/kitchen02.json")
	*/
	/************************************************************************/
	inline std::string LevelPath(const std::string& filename) {
		return std::string(Dirs::LEVELS) + filename;
	}

	/************************************************************************/
	/*!
	\brief
	Constructs a full path to a prefab file.
	\param filename
	The prefab filename (e.g., "player.json")
	\return
	Full relative path (e.g., "../prefabs/player.json")
	*/
	/************************************************************************/
	inline std::string PrefabPath(const std::string& filename) {
		return std::string(Dirs::PREFABS) + filename;
	}

	/************************************************************************/
	/*!
	\brief
	Constructs a full path to a font file.
	\param filename
	The font filename (e.g., "MyFont.ttf")
	\return
	Full relative path (e.g., "../assets/Font/MyFont.ttf")
	*/
	/************************************************************************/
	inline std::string FontPath(const std::string& filename) {
		return std::string(Dirs::FONTS) + filename;
	}

	/**
	 * @brief Constructs a full path to an audio file.
	 * @param filename The audio filename (e.g., "bgm_main.wav")
	 * @return Full relative path (e.g., "../assets/Audio/bgm_main.wav")
	 */
	 /************************************************************************/
	 /*!
	 \brief
	 Constructs a full path to an audio file.
	 \param filename
	 The audio filename (e.g., "bgm_main.wav")
	 \return
	 Full relative path (e.g., "../assets/Audio/bgm_main.wav")
	 */
	 /************************************************************************/
	inline std::string AudioPath(const std::string& filename) {
		return std::string(Dirs::AUDIO) + filename;
	}

	/************************************************************************/
	/*!
	\brief
	Constructs a full path to a cutscene file.
	\param filename
	The cutscene filename (e.g., "Cutscene_starting_1.png")
	\return
	Full relative path (e.g., "../assets/Cutscenes/Cutscene_starting_1.png")
	*/
	/************************************************************************/
	inline std::string CutscenePath(const std::string& filename) {
		return std::string(Dirs::CUTSCENES) + filename;
	}

} // namespace FilePaths
