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

	// Generates a hash for the given LevelData by combining hashes of its background, object counts, and text object counts.
	std::size_t HashLevelData(const LevelData& level) {
		std::size_t seed = std::hash<std::string>{}(level.background);
		seed ^= std::hash<std::size_t>{}(level.objects.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<std::size_t>{}(level.textObjects.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
}

namespace LEAUTOSAVE {
	// Builds the autosave file path by taking the original level path, extracting its stem, and appending ".autosave.json" before the extension.
	std::string BuildAutosavePath(const std::string& levelPath) {
		const fs::path original(levelPath);
		const std::string stem = original.stem().string();
		return (original.parent_path() / (stem + ".autosave.json")).generic_string();
	}

	// Ticks the autosave timer and saves the current level data to an autosave file if the specified interval has elapsed and the level data has changed since the last save.
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

	// Checks if a recovery candidate exists by verifying the presence of the autosave file and comparing its last write time with the main level file.
	// Returns true if the autosave is newer or if the main level file is missing.
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

	// Loads recovery data from the autosave file into the provided LevelData reference. Returns true on successful load.
	bool LoadRecovery(const std::string& levelPath, LevelData& outLevel) {
		return LevelSerializer::Load(BuildAutosavePath(levelPath), outLevel);
	}

	// Discards the recovery candidate by deleting the autosave file if it exists.
	void DiscardRecovery(const std::string& levelPath) {
		fs::path autosavePath(BuildAutosavePath(levelPath));
		if (fs::exists(autosavePath)) {
			fs::remove(autosavePath);
		}
	}
}
