/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorCommandSystem.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Header for the Level Editor command system, which provides undo/redo functionality
					by recording snapshots of the level state before mutations and restoring them on demand.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <functional>

class LevelEditor;
struct LevelData;

namespace LECOMMAND {
	/// @brief Callback type used to capture the current level state into a snapshot.
	using CaptureStateFn = std::function<void(LevelData&)>;
	/// @brief Callback type used to restore a previously captured level snapshot.
	using RestoreStateFn = std::function<void(const LevelData&)>;

	/**
	 * @brief Records an undo snapshot before mutating level data.
	 * @param editor Shared level editor controller.
	 * @param capture Callback used to capture the current level state.
	 */
	void RecordPreMutationSnapshot(LevelEditor& editor, const CaptureStateFn& capture);

	/**
	 * @brief Restores the most recent undo snapshot.
	 * @param editor Shared level editor controller.
	 * @param capture Callback used to capture the current state before undoing.
	 * @param restore Callback used to apply the restored snapshot.
	 * @return True when an undo action was performed.
	 */
	bool Undo(LevelEditor& editor, const CaptureStateFn& capture, const RestoreStateFn& restore);

	/**
	 * @brief Restores the most recent redo snapshot.
	 * @param editor Shared level editor controller.
	 * @param capture Callback used to capture the current state before redoing.
	 * @param restore Callback used to apply the restored snapshot.
	 * @return True when a redo action was performed.
	 */
	bool Redo(LevelEditor& editor, const CaptureStateFn& capture, const RestoreStateFn& restore);

	/**
	 * @brief Clears all undo and redo history managed by the command system.
	 */
	void ClearHistory();
}
