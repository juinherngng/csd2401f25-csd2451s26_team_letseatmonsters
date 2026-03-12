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
	// Type aliases for snapshot capture and restore functions.
	using CaptureStateFn = std::function<void(LevelData&)>;
	using RestoreStateFn = std::function<void(const LevelData&)>;

	// Records a snapshot of the current level state before a mutation occurs.
	void RecordPreMutationSnapshot(LevelEditor& editor, const CaptureStateFn& capture);

	// Undo the last action by restoring the most recent snapshot from the undo stack.
	bool Undo(LevelEditor& editor, const CaptureStateFn& capture, const RestoreStateFn& restore);

	// Redo the last undone action by restoring the next snapshot from the redo stack.
	bool Redo(LevelEditor& editor, const CaptureStateFn& capture, const RestoreStateFn& restore);

	// Clears the undo and redo history stacks.
	void ClearHistory();
}
