/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelPrefabs.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Header for the Level Editor Prefabs panel, which allows users to save scene
					objects as prefabs (JSON files) and instantiate them in the scene.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

class LevelEditor;
class Scene;

/**
 * @namespace LEPANELPREFABS
 * @brief ImGui panel for working with JSON prefabs/archetypes.
 */
namespace LEPANELPREFABS {

	/**
	 * @brief Draws the Prefabs panel for saving and instantiating prefab assets.
	 * @param editor Shared level editor controller.
	 * @param scene Scene currently being edited.
	 * @param selectedObjectId Engine object ID for the active scene selection.
	 */
	void DrawPrefabsPanel(LevelEditor& editor, Scene& scene, int& selectedObjectId);
}
