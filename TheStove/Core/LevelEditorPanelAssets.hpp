/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelAssets.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Assets panel for the Level Editor.
					- Import textures/prefabs into project folders
					- List & refresh textures/prefabs
					- Drag & drop sources for other panels
					- Double-click texture to apply to the current selection

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

class LevelEditor;
class Scene;

// Forward declare GameObject to avoid circular dependency.
namespace LEPANELASSETS {

	/**
	 * @brief Draws the Assets panel and handles asset import/apply actions.
	 * @param editor Shared level editor controller.
	 * @param scene Scene currently being edited.
	 * @param selectedIndex Hierarchy selection index tracked across editor panels.
	 * @param selectedObjectId Engine object ID for the active scene selection.
	 */
	void DrawAssetsPanel(LevelEditor& editor,
		Scene& scene,
		int& selectedIndex,
		int selectedObjectId);
}

