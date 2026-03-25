/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorActions.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Declarations for helper functions related to level editor actions,
					such as rendering action buttons and handling undo/redo shortcuts.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <functional>

class LevelEditor;
class Scene;
struct LevelData;

namespace LEACTIONS {

	/**
	 * @brief Callback bundle used by the Level Editor action bar.
	 *
	 * Each callback is supplied by the caller so the UI layer can trigger
	 * scene-management and undo/redo behavior without owning that logic.
	 */
	struct Callbacks {
		std::function<void()> onLoad;
		std::function<void()> onNewScene;
		std::function<void()> onSave;
		std::function<bool()> onUndo;
		std::function<bool()> onRedo;
		std::function<void()> onPlay;
		std::function<void()> onStop;
	};

	/**
	 * @brief Draws the action-button grid for the Level Editor toolbar.
	 * @param editor Shared level editor controller.
	 * @param scene Scene currently being edited.
	 * @param callbacks Action handlers invoked by the toolbar buttons.
	 */
	void DrawActionGrid(LevelEditor& editor, Scene& scene, const Callbacks& callbacks);

	/**
	 * @brief Processes editor undo and redo keyboard shortcuts.
	 * @param editor Shared level editor controller.
	 * @param undo Callback used when an undo shortcut is triggered.
	 * @param redo Callback used when a redo shortcut is triggered.
	 */
	void HandleUndoRedoShortcuts(LevelEditor& editor, const std::function<bool()>& undo, const std::function<bool()>& redo);
}

