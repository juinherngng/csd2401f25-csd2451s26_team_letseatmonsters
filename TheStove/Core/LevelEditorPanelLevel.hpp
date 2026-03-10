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
	// Draw the Level panel window and handle all interactions.
	void DrawLevelPanel(LevelEditor& editor,
		Scene& scene,
		int& selectedIndex,
		int& selectedObjectId);

	// Allow other modules(pick / drag) to push an undo snapshot.
	void RecordUndoSnapshot(LevelEditor& editor, Scene& scene);
}
