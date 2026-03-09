/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorCommandSystem.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implementation of the Level Editor command system, which provides undo/redo functionality
					by capturing and restoring snapshots of LevelData. The undo limit is configurable
					via the LE_EDITOR_UNDO_LIMIT environment variable (default 50). Undo/redo stacks are
					cleared when a new snapshot is recorded.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "LevelEditor.hpp"
#include "LevelEditorCommandSystem.hpp"
#include "LevelSerializer.hpp"

#include <cstdlib>
#include <vector>

namespace {
	int GetUndoLimit() {
		const char* envValue = std::getenv("LE_EDITOR_UNDO_LIMIT");
		if (!envValue) {
			return 50;
		}

		const int parsed = std::atoi(envValue);
		return parsed > 0 ? parsed : 50;
	}

	std::vector<LevelData> sUndoStack;
	std::vector<LevelData> sRedoStack;

	// Pushes a new state onto the given stack and ensures the stack size does not exceed the configured undo limit by removing the oldest entry if necessary.
	void BoundedPush(std::vector<LevelData>& stack, LevelData&& state) {
		stack.push_back(std::move(state));
		if (stack.size() > static_cast<std::size_t>(GetUndoLimit())) {
			stack.erase(stack.begin());
		}
	}
}

namespace LECOMMAND {
	// Records a snapshot of the current level state before a mutation occurs.
	// The capture function is called to fill a LevelData snapshot, which is then pushed onto the undo stack.
	// The redo stack is cleared to maintain consistency with the new action.
	void RecordPreMutationSnapshot(LevelEditor& editor, const CaptureStateFn& capture) {
		if (editor.IsPlaying() || !capture) {
			return;
		}

		LevelData snap{};
		capture(snap);
		BoundedPush(sUndoStack, std::move(snap));
		sRedoStack.clear();
	}

	bool Undo(LevelEditor& editor, const CaptureStateFn& capture, const RestoreStateFn& restore) {
		if (editor.IsPlaying() || sUndoStack.empty() || !capture || !restore) {
			return false;
		}

		LevelData current{};
		capture(current);
		BoundedPush(sRedoStack, std::move(current));

		LevelData snap = sUndoStack.back();
		sUndoStack.pop_back();
		restore(snap);
		editor.MutableLevel() = snap;
		return true;
	}

	bool Redo(LevelEditor& editor, const CaptureStateFn& capture, const RestoreStateFn& restore) {
		if (editor.IsPlaying() || sRedoStack.empty() || !capture || !restore) {
			return false;
		}

		LevelData current{};
		capture(current);
		BoundedPush(sUndoStack, std::move(current));

		LevelData snap = sRedoStack.back();
		sRedoStack.pop_back();
		restore(snap);
		editor.MutableLevel() = snap;
		return true;
	}

	void ClearHistory() {
		sUndoStack.clear();
		sRedoStack.clear();
	}
}
