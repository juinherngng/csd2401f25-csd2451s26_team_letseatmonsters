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

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

class LevelEditor;
class Scene;

// Forward declare GameObject to avoid circular dependency.
namespace LEPANELASSETS {
	// Draw the Assets panel window.
	void DrawAssetsPanel(LevelEditor& editor,
		Scene& scene,
		int& selectedIndex,
		int selectedObjectId);
}
