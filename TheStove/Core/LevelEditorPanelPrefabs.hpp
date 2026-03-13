/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelPrefabs.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Header for the Level Editor Prefabs panel, which allows users to save scene
					objects as prefabs (JSON files) and instantiate them in the scene.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
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
	// Draw the Prefabs panel window.
	void DrawPrefabsPanel(LevelEditor& editor, Scene& scene, int& selectedObjectId);
}
