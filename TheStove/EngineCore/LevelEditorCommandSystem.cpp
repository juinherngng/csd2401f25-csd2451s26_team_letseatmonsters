/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorCommandSystem.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implementation of the Level Editor command system, which provides undo/redo functionality.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <cstdlib>
#include <string>
#include <vector>

#include "EngineCore/LevelEditor.hpp"
#include "EngineCore/LevelEditorCommandSystem.hpp"

namespace {
	constexpr const char* kUndoLimitEnvVar = "LE_EDITOR_UNDO_LIMIT";
	constexpr int kDefaultUndoLimit = 50;

	/**
	 * @brief Reads an environment variable and returns its value as a string.
	 * @param key Environment-variable name.
	 * @return Variable value, or an empty string when unset.
	 */
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

	/**
	 * @brief Parses a positive integer from text with a fallback.
	 * @param value Text value to parse.
	 * @param fallback Value to return when parsing fails.
	 * @return Parsed positive integer, or the fallback value.
	 */
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

	/**
	 * @brief Returns the configured undo-history limit.
	 * @return Maximum number of undo snapshots to retain.
	 */
	int GetUndoLimit() {
		static const int undoLimit = ParsePositiveInt(ReadEnvVar(kUndoLimitEnvVar), kDefaultUndoLimit);
		return undoLimit;
	}

	std::vector<LevelData> sUndoStack;
	std::vector<LevelData> sRedoStack;

	/**
	 * @brief Pushes a snapshot onto a history stack while enforcing the undo limit.
	 * @param stack History stack to append to.
	 * @param state Snapshot to store.
	 */
	void BoundedPush(std::vector<LevelData>& stack, LevelData&& state) {
		stack.push_back(std::move(state));
		if (stack.size() > static_cast<std::size_t>(GetUndoLimit())) {
			stack.erase(stack.begin());
		}
	}

	/**
	 * @brief Moves one snapshot between history stacks and applies it to the editor.
	 * @param editor Shared level editor controller.
	 * @param source Source history stack to pop from.
	 * @param destination Destination history stack to push the current state onto.
	 * @param capture Callback used to capture the current level state.
	 * @param restore Callback used to apply the retrieved snapshot.
	 * @return True when a snapshot was applied.
	 */
	template <typename CaptureFn, typename RestoreFn>

	/**
	 * @brief Applies snapshot from history.
	 * @param editor Level editor state to operate on.
	 * @param source Parameter for source.
	 * @param destination Parameter for destination.
	 * @param capture Parameter for capture.
	 * @param restore Parameter for restore.
	 * @return True when the operation succeeds or the condition is met.
	 */
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

	/**
	 * @brief Records an undo snapshot before a level mutation.
	 * @param editor Shared level editor controller.
	 * @param capture Callback used to capture the current level state.
	 */
	void RecordPreMutationSnapshot(LevelEditor& editor, const CaptureStateFn& capture) {
		if (editor.IsPlaying() || !capture) {
			return;
		}

		LevelData snap{};
		capture(snap);
		BoundedPush(sUndoStack, std::move(snap));
		sRedoStack.clear();
	}

	/**
	 * @brief Restores the latest snapshot from the undo history.
	 * @param editor Shared level editor controller.
	 * @param capture Callback used to capture the current level state.
	 * @param restore Callback used to apply the restored snapshot.
	 * @return True when an undo operation was performed.
	 */
	bool Undo(LevelEditor& editor, const CaptureStateFn& capture, const RestoreStateFn& restore) {
		return ApplySnapshotFromHistory(editor, sUndoStack, sRedoStack, capture, restore);
	}

	/**
	 * @brief Restores the latest snapshot from the redo history.
	 * @param editor Shared level editor controller.
	 * @param capture Callback used to capture the current level state.
	 * @param restore Callback used to apply the restored snapshot.
	 * @return True when a redo operation was performed.
	 */
	bool Redo(LevelEditor& editor, const CaptureStateFn& capture, const RestoreStateFn& restore) {
		return ApplySnapshotFromHistory(editor, sRedoStack, sUndoStack, capture, restore);
	}

	/**
	 * @brief Clears all stored undo and redo snapshots.
	 */
	void ClearHistory() {
		sUndoStack.clear();
		sRedoStack.clear();
	}
}
