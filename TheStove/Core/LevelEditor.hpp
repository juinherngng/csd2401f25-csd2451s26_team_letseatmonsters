/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelEditor.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		In-engine Level Editor controller.
					Responsibilities:
					- Show editor UI (toolbar + panels)
					- Maintain edit-state vs play-state (snapshot/restore)
					- Expose shared state (level path, LevelData) to panels
					- Delegate scene picking/dragging to helper module

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

#include "imgui.h"
#include "imgui_internal.h"
#include "LevelSerializer.hpp"
#include "LevelEditorFileIO.hpp"

struct LevelData;
class Scene;

/**
 * @class LevelEditor
 * @brief Editor UI and state manager for authoring levels at runtime.
 *
 * Notes:
 * - Rotation in JSON/UI is stored in DEGREES. Convert to RADIANS at engine call-sites.
 * - Panels access levelPath directly in current code; kept public to avoid breaking calls.
 */
class LevelEditor {
public:
	// ----- Visibility -----
	bool IsEnabled() const {
		return isEnabled;
	}
	void Toggle() {
		isEnabled = !isEnabled;
	}

	// ----- Level path control (kept public for existing panel code) -----
	void SetPath(const std::string& path) {
		levelPath = path;
	}
	const std::string& GetPath() const {
		return levelPath;
	}
	std::string levelPath{}; // public on purpose to preserve existing panel access

	// ----- Runtime / Playback -----
	bool IsPlaying() const {
		return isPlaying;
	}
	void SetPlaying(bool on) {
		isPlaying = on;
	}

	// Expose working LevelData and Play snapshot for panels
	LevelData& MutableLevel() {
		return level;
	}
	LevelData& MutablePlaySnapshot() {
		return playStartSnapshot;
	}

	// ----- UI Entrypoint -----
	void DrawUI(Scene& scene);

private:
	// ----- Flags -----
#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	bool isEnabled = true;
#else
	bool isEnabled = false;
#endif
	bool isPlaying = false;

	// ----- Data Models -----
	LevelData level{};             // Working copy while editing
	LevelData playStartSnapshot{}; // Snapshot captured at Play
};
