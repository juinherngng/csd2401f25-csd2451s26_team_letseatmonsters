/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorAutoSave.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Handles autosaving of level data at regular intervals and provides
					recovery options if the main level file is missing or older than the autosave.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

struct LevelData;

namespace LEAUTOSAVE {
	// Ticks the autosave timer and saves the current level data to an autosave file if the specified interval has elapsed
	void Tick(const std::string& levelPath, const LevelData& currentLevel, float deltaTimeSeconds);

	// Checks if there is a recovery candidate for the given level path.
	// A recovery candidate exists if the autosave file exists and is newer than the main level file, or if the main level file does not exist.
	bool HasRecoveryCandidate(const std::string& levelPath);

	// Loads the recovery data from the autosave file for the given level path into outLevel. Returns true if successful.
	bool LoadRecovery(const std::string& levelPath, LevelData& outLevel);

	// Discards the recovery file for the given level path by deleting the autosave file if it exists.
	void DiscardRecovery(const std::string& levelPath);

	// Builds the autosave file path by taking the original level path, extracting its stem, and appending ".autosave.json" before the extension.
	std::string BuildAutosavePath(const std::string& levelPath);
}
