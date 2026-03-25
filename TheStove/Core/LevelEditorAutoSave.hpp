/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorAutoSave.hpp
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
	/**
	 * @brief Advances the autosave timer and writes a recovery file when needed.
	 * @param levelPath Path to the main level file.
	 * @param currentLevel Current level data to serialize.
	 * @param deltaTimeSeconds Time elapsed since the previous frame in seconds.
	 */
	void Tick(const std::string& levelPath, const LevelData& currentLevel, float deltaTimeSeconds);

	/**
	 * @brief Returns whether an autosave recovery candidate exists for a level.
	 * @param levelPath Path to the main level file.
	 * @return True when the autosave file is usable as a recovery source.
	 */
	bool HasRecoveryCandidate(const std::string& levelPath);

	/**
	 * @brief Loads autosave recovery data for a level.
	 * @param levelPath Path to the main level file.
	 * @param outLevel Receives the recovered level data.
	 * @return True when recovery data was loaded successfully.
	 */
	bool LoadRecovery(const std::string& levelPath, LevelData& outLevel);

	/**
	 * @brief Deletes the autosave recovery file for a level, if present.
	 * @param levelPath Path to the main level file.
	 */
	void DiscardRecovery(const std::string& levelPath);

	/**
	 * @brief Builds the autosave filename associated with a level file.
	 * @param levelPath Path to the main level file.
	 * @return Autosave file path derived from the input level path.
	 */
	std::string BuildAutosavePath(const std::string& levelPath);
}
