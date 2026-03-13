/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorCommandSystem.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implementation of the Level Editor command system, which provides undo/redo functionality.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "LevelEditor.hpp"
#include "LevelEditorCommandSystem.hpp"

#include <cstdlib>
#include <string>
#include <vector>

namespace {
	constexpr const char* kUndoLimitEnvVar = "LE_EDITOR_UNDO_LIMIT";
	constexpr int kDefaultUndoLimit = 50;

	// Reads an environment variable and returns its value as a string.
	std::string ReadEnvVar(const char* key) {
#ifdef _WIN32
		char* rawValue = nullptr;
		size_t len = 0;
		if (_dupenv_s(&rawValue, &len, key) != 0 || rawValue == nullptr) {
			return {};
		}

		std::string value(rawValue);
		std::free(rawValue);
		return value;
#else
		const char* rawValue = std::getenv(key);
		return rawValue ? std::string(rawValue) : std::string{};
#endif
	}

	// Parses a string as a positive integer, returning a fallback value if parsing fails or if the value is not positive.
	int ParsePositiveInt(const std::string& value, int fallback) {
		if (value.empty()) {
			return fallback;
		}

		char* end = nullptr;
		const long parsed = std::strtol(value.c_str(), &end, 10);
		if (end == value.c_str() || *end != '\0' || parsed <= 0) {
			return fallback;
		}

		return static_cast<int>(parsed);
	}

	// Retrieves the undo limit from the environment variable, parsing it as a positive integer and falling back to a default value if necessary.
	int GetUndoLimit() {
		static const int undoLimit = ParsePositiveInt(ReadEnvVar(kUndoLimitEnvVar), kDefaultUndoLimit);
		return undoLimit;
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

	// Applies a snapshot from the source history stack to the editor, moving the current state to the destination stack.
	template <typename CaptureFn, typename RestoreFn>
	bool ApplySnapshotFromHistory(LevelEditor& editor,
		std::vector<LevelData>& source,
		std::vector<LevelData>& destination,
		const CaptureFn& capture,
		const RestoreFn& restore) {
		if (editor.IsPlaying() || source.empty() || !capture || !restore) {
			return false;
		}

		LevelData current{};
		capture(current);
		BoundedPush(destination, std::move(current));

		LevelData snap = source.back();
		source.pop_back();
		restore(snap);
		editor.MutableLevel() = snap;
		return true;
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
		return ApplySnapshotFromHistory(editor, sUndoStack, sRedoStack, capture, restore);
	}

	bool Redo(LevelEditor& editor, const CaptureStateFn& capture, const RestoreStateFn& restore) {
		return ApplySnapshotFromHistory(editor, sRedoStack, sUndoStack, capture, restore);
	}

	void ClearHistory() {
		sUndoStack.clear();
		sRedoStack.clear();
	}
}
