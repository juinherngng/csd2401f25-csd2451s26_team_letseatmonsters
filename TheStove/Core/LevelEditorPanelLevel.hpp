/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelLevel.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Level panel for the Level Editor.
					- Load/Save level JSON files
					- Manage play/stop state
					- Display scene hierarchy and object properties
					- Handle prefab/texture drag-drop instantiation
					- Sync LevelData <-> Scene

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

class LevelEditor;
class Scene;

/**
 * @namespace LEPANELLEVEL
 * @brief ImGui Level panel that manages the overall level state.
 */
namespace LEPANELLEVEL {
	// Draw the Level panel window and handle all interactions.
	void DrawLevelPanel(LevelEditor& editor,
						Scene& scene,
						int& selectedIndex,
						int& selectedObjectId);

	// Allow other modules(pick / drag) to push an undo snapshot
	void RecordUndoSnapshot(LevelEditor& editor, Scene& scene);
}
