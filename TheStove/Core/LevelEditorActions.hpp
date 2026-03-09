/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorActions.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Declarations for helper functions related to level editor actions,
					such as rendering action buttons and handling undo/redo shortcuts.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <functional>

class LevelEditor;
class Scene;
struct LevelData;

namespace LEACTIONS {
	// Struct to hold callbacks for level editor actions.
	struct Callbacks {
		std::function<void()> onLoad;
		std::function<void()> onNewScene;
		std::function<void()> onSave;
		std::function<bool()> onUndo;
		std::function<bool()> onRedo;
		std::function<void()> onPlay;
		std::function<void()> onStop;
	};

	// Draws the action grid with buttons for Load, New Scene, Save, Undo, Redo, Play, Stop, and Pause/Resume.
	void DrawActionGrid(LevelEditor& editor, Scene& scene, const Callbacks& callbacks);

	// Handles undo/redo shortcuts (Ctrl+Z / Ctrl+Y or Cmd+Z / Cmd+Shift+Z) and calls the provided callbacks if the shortcuts are triggered.
	void HandleUndoRedoShortcuts(LevelEditor& editor, const std::function<bool()>& undo, const std::function<bool()>& redo);
}
