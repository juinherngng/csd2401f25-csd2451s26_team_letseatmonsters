/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelPrefabs.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Prefabs panel for the Level Editor.
					- Choose prefab path (combo + input)
					- Save selected object as prefab
					- Instantiate from prefab
					- Propagate prefab changes to linked instances

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
