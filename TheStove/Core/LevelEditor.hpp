/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelEditor.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		In-engine Level Editor controller.
					- Show editor UI (toolbar + panels)
					- Maintain edit-state vs play-state (snapshot/restore)
					- Expose shared state (level path, LevelData) to panels
					- Delegate scene picking/dragging to helper module

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <string>

#include "LevelEditorFileIO.hpp"
#include "LevelSerializer.hpp"

struct LevelData;
class Scene;

/**
 * @brief Controller for the in-engine level editor.
 *
 * The editor owns shared state that multiple panels depend on, including the
 * active level path, editable snapshots, and tool visibility flags.
 */
class LevelEditor {
public:
	/**
	 * @brief Returns whether the editor UI is currently enabled.
	 * @return True when the level editor interface should be shown.
	 */
	bool IsEnabled() const {
		return isEnabled;
	}

	/**
	 * @brief Toggles the overall editor visibility.
	 *
	 * This switches the editor between visible and hidden states without
	 * changing any of its currently loaded data.
	 */
	void Toggle() {
		isEnabled = !isEnabled;
	}

	/**
	 * @brief Stores the active level path used by the editor.
	 * @param path Filesystem path to the level currently being edited.
	 */
	void SetPath(const std::string& path) {
		levelPath = path;
	}

	/**
	 * @brief Returns the active level path.
	 * @return Constant reference to the current level path string.
	 */
	const std::string& GetPath() const {
		return levelPath;
	}
	std::string levelPath{}; // public on purpose to preserve existing panel access

	/**
	 * @brief Returns whether the editor is currently in play mode.
	 * @return True when the editor is simulating the scene in play mode.
	 */
	bool IsPlaying() const {
		return isPlaying;
	}

	/**
	 * @brief Updates the play-state flag tracked by the editor.
	 * @param on True to mark the editor as being in play mode.
	 */
	void SetPlaying(bool on) {
		isPlaying = on;
	}

	/**
	 * @brief Returns whether the Build Size Analyzer panel should be shown.
	 * @return True when the Build Size Analyzer panel is open.
	 */
	bool IsBuildSizeAnalyzerOpen() const {
		return buildSizeAnalyzerOpen;
	}

	/**
	 * @brief Updates the Build Size Analyzer panel visibility.
	 * @param open True to display the Build Size Analyzer panel.
	 */
	void SetBuildSizeAnalyzerOpen(bool open) {
		buildSizeAnalyzerOpen = open;
	}

	/**
	 * @brief Returns the working LevelData snapshot being edited.
	 * @return Mutable reference to the editor's active level data.
	 */
	LevelData& MutableLevel() {
		return level;
	}

	/**
	 * @brief Returns the snapshot captured when play mode begins.
	 * @return Mutable reference to the play-mode entry snapshot.
	 */
	LevelData& MutablePlaySnapshot() {
		return playStartSnapshot;
	}

	/**
	 * @brief Draws the editor UI for the current frame.
	 * @param scene Scene currently being edited.
	 */
	void DrawUI(Scene& scene);

private:
	// Flags
#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	bool isEnabled = true;
#else
	bool isEnabled = false;
#endif
	bool isPlaying = false;
	bool buildSizeAnalyzerOpen = true;

	// Data Models
	LevelData level{};             // Working copy while editing
	LevelData playStartSnapshot{}; // Snapshot captured at Play
};
