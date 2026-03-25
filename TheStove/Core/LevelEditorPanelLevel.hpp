/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelLevel.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Header for the Level panel in the Level Editor, which manages the overall level state and provides

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

class LevelEditor;
class Scene;

// Forward declare GameObject to avoid circular dependency.
namespace LEPANELLEVEL {
	/**
	 * @brief Draws the main Level panel and handles its editor interactions.
	 * @param editor Shared level editor controller.
	 * @param scene Scene currently being edited.
	 * @param selectedIndex Hierarchy selection index tracked across editor panels.
	 * @param selectedObjectId Engine object ID for the active scene selection.
	 */
	void DrawLevelPanel(LevelEditor& editor,
		Scene& scene,
		int& selectedIndex,
		int& selectedObjectId);

	/**
	 * @brief Captures an undo snapshot for external editor tools such as pick/drag.
	 * @param editor Shared level editor controller.
	 * @param scene Scene whose current state should be captured.
	 */
	void RecordUndoSnapshot(LevelEditor& editor, Scene& scene);
}

