/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorAutoSave.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements a simple autosave mechanism for the level editor.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "LevelEditorAutoSave.hpp"
#include "LevelSerializer.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace {
	// Accumulates delta time between ticks to determine when to perform the next autosave.
	float sAccumulatedSeconds = 0.0f;
	std::size_t sLastSavedHash = 0;
	constexpr float kAutosaveIntervalSeconds = 30.0f;

	/**
	 * @brief Builds a lightweight hash for autosave change detection.
	 * @param level Level data to hash.
	 * @return Hash representing the current level snapshot.
	 */
	std::size_t HashLevelData(const LevelData& level) {
		std::size_t seed = std::hash<std::string>{}(level.background);
		seed ^= std::hash<std::size_t>{}(level.objects.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<std::size_t>{}(level.textObjects.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
}

namespace LEAUTOSAVE {
	/**
	 * @brief Builds the autosave file path associated with a level file.
	 * @param levelPath Path to the main level file.
	 * @return Autosave path derived from the level path.
	 */
	std::string BuildAutosavePath(const std::string& levelPath) {
		const fs::path original(levelPath);
		const std::string stem = original.stem().string();
		return (original.parent_path() / (stem + ".autosave.json")).generic_string();
	}

	/**
	 * @brief Advances the autosave timer and writes an autosave when required.
	 * @param levelPath Path to the main level file.
	 * @param currentLevel Current level data to serialize.
	 * @param deltaTimeSeconds Time elapsed since the previous frame in seconds.
	 */
	void Tick(const std::string& levelPath, const LevelData& currentLevel, float deltaTimeSeconds) {
		sAccumulatedSeconds += deltaTimeSeconds;
		if (sAccumulatedSeconds < kAutosaveIntervalSeconds) {
			return;
		}

		sAccumulatedSeconds = 0.0f;

		const std::size_t currentHash = HashLevelData(currentLevel);
		if (currentHash == sLastSavedHash) {
			return;
		}

		const std::string autosavePath = BuildAutosavePath(levelPath);
		if (LevelSerializer::Save(autosavePath, currentLevel)) {
			sLastSavedHash = currentHash;
		}
	}

	/**
	 * @brief Returns whether an autosave can be used for recovery.
	 * @param levelPath Path to the main level file.
	 * @return True when the autosave exists and is newer than the main file, or when the main file is missing.
	 */
	bool HasRecoveryCandidate(const std::string& levelPath) {
		const fs::path autosavePath(BuildAutosavePath(levelPath));
		if (!fs::exists(autosavePath)) {
			return false;
		}

		const fs::path mainPath(levelPath);
		if (!fs::exists(mainPath)) {
			return true;
		}

		return fs::last_write_time(autosavePath) >= fs::last_write_time(mainPath);
	}

	/**
	 * @brief Loads recovery data from an autosave file.
	 * @param levelPath Path to the main level file.
	 * @param outLevel Receives the recovered level data.
	 * @return True when recovery data was loaded successfully.
	 */
	bool LoadRecovery(const std::string& levelPath, LevelData& outLevel) {
		return LevelSerializer::Load(BuildAutosavePath(levelPath), outLevel);
	}

	/**
	 * @brief Deletes the autosave file associated with a level, if present.
	 * @param levelPath Path to the main level file.
	 */
	void DiscardRecovery(const std::string& levelPath) {
		fs::path autosavePath(BuildAutosavePath(levelPath));
		if (fs::exists(autosavePath)) {
			fs::remove(autosavePath);
		}
	}
}
