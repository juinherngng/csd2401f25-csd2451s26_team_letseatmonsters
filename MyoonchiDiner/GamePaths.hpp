/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GamePaths.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (80%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu		(20%)

 DESCRIPTION:		Centralized path and key constants for all Myoonchi Diner
					game assets. Keeps every game-specific file path and audio
					catalog key in one place so they can be referenced from
					both the bootstrap layer and the game logic layer without
					scattering magic strings.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

/**
 * @brief Groups game-specific asset paths and audio catalog keys for Myoonchi Diner.
 */
namespace MyoonchiPaths {

	// JSON levels registered with the engine's game-state bootstrap layer.
	namespace Levels {
		constexpr const char* MAIN_MENU = "../levels/main_menu.json";
		constexpr const char* KITCHEN_01 = "../levels/kitchen01.json";
		constexpr const char* TUTORIAL = "../levels/tutorial.json";
	}

	// Texture assets referenced directly by runtime hooks and logic systems.
	namespace Textures {
		constexpr const char* BACKGROUND = "../assets/Backgrounds/Background.png";

		constexpr const char* CUSTOMER_GOAT = "../assets/Characters/goat-Sheet.png";
		constexpr const char* CUSTOMER_TIGER = "../assets/Characters/tiger-Sheet.png";
		constexpr const char* CUSTOMER_ANTEATER = "../assets/Characters/anteater spritesheet.png";

		constexpr const char* AMBIENT_VFX_SHEET = "../assets/VFX/VFX_SpriteSheet.png";
	}

	// Video files triggered by the game's cutscene flow.
	namespace Videos {
		constexpr const char* INTRO_CUTSCENE = "../assets/Videos/Starting_Cutscene_Animation_TEXT.mp4";
	}

	// Audio catalog keys. These must match the names authored in AudioCatalog.json.
	namespace Audio {
		constexpr const char* BGM_MAIN_MENU = "bgm_MyoonchiDiner_MainMenu";
		constexpr const char* BGM_LEVEL_THEME = "bgm_MyoonchiDiner_LevelTheme";
		constexpr const char* BGM_KITCHEN_AMBIENCE = "bgm_KitchenAmbience";
		constexpr const char* BGM_FOREST_AMBIENCE = "bgm_forest_ambience";
		constexpr const char* BGM_INTRO_CUTSCENE = "bgm_MyoonchiDiner_IntroCutscene";
		constexpr const char* BGM_WIN_CUTSCENE = "bgm_win_cutscene";
		constexpr const char* SFX_GAMEOVER = "sfx_gameover";
		constexpr const char* SFX_START_BUTTON = "sfx_start_button";
		constexpr const char* SFX_SKIP_INTRO_CUTSCENE = "sfx_skip_intro_cutscene";
		constexpr const char* SFX_INTRO_CUTSCENE = "sfx_introcutscene";
		constexpr const char* SFX_UI_BACK = "ui_back";
		constexpr const char* SFX_UI_HOVER = "ui_hover";
		constexpr const char* SFX_UI_CLICK_BUTTON = "ui_clickbutton";
		constexpr const char* SFX_UI_START_RESUME = "ui_startresume";
	}
}
