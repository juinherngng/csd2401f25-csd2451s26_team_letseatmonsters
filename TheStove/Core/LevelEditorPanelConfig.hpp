/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelConfig.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Config panel for the Level Editor.
					- Edit game configuration values in-engine
					- Reload from config file
					- Save settings back to config.txt

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

class LevelEditor;
class Scene;

// Forward declare GameObject to avoid circular dependency.
namespace LEPANELCONFIG {

	/**
	 * @brief Draws the configuration editing panel for engine settings.
	 * @param editor Shared level editor controller.
	 * @param scene Scene currently being edited.
	 */
	void DrawConfigPanel(LevelEditor& editor, Scene& scene);
}
