/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			RuntimeLevelPipeline.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements a small runtime level-loading pipeline that centralizes dependency
					preload and scene-build entry points shared by Scene and GameStateManager.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "RuntimeLevelPipeline.hpp"

#include "../Graphics/ResourceManager.hpp"

#include "LevelSerializer.hpp"
#include "Logger.hpp"
#include "RuntimeLevel.hpp"

#include <unordered_set>

namespace {
	/**
	 * @brief Builds a unique texture dependency list from runtime level data.
	 * @param data Parsed level data to inspect.
	 * @return Unique texture paths used by backgrounds and spawned objects.
	 */
	std::vector<std::string> BuildTextureManifest(const LevelData& data) {
		std::vector<std::string> textures;
		textures.reserve(data.objects.size() + 2);

		std::unordered_set<std::string> seen;
		seen.reserve(data.objects.size() + 2);

		auto appendIfNew = [&](const std::string& texturePath) {
			if (!texturePath.empty() && seen.insert(texturePath).second) {
				textures.push_back(texturePath);
			}
			};

		appendIfNew(data.background);
		appendIfNew(data.backgroundOverlay);

		for (const LevelObject& obj : data.objects) {
			appendIfNew(obj.texture);
		}

		return textures;
	}
}

/**
 * @brief Collects unique texture dependencies referenced by a runtime level.
 * @param levelPath Path to the level JSON file.
 * @return Unique texture paths referenced by the level.
 */
std::vector<std::string> RuntimeLevelPipeline::CollectTextureDependencies(const std::string& levelPath) {
	LevelData data{};
	if (!LevelSerializer::Load(levelPath, data)) {
		TS_LOG_WARN("[RuntimeLevelPipeline] Failed to inspect level dependencies for '" << levelPath << "'.");
		return {};
	}

	return BuildTextureManifest(data);
}

/**
 * @brief Preloads all texture dependencies referenced by a runtime level.
 * @param levelPath Path to the level JSON file.
 */
void RuntimeLevelPipeline::PreloadLevelDependencies(const std::string& levelPath) {
	const std::vector<std::string> textures = CollectTextureDependencies(levelPath);
	if (!textures.empty()) {
		ResourceManager::Instance().PreloadTextures(textures);
	}
}

/**
 * @brief Loads a runtime level and reports warnings/failure details in a structured result.
 * @param levelPath Path to the level JSON file.
 * @param scene Target scene that should be rebuilt from the runtime level.
 * @return Structured result describing success, warnings, and any failure reason.
 */
RuntimeLevelPipeline::LevelLoadResult RuntimeLevelPipeline::LoadLevelIntoSceneDetailed(const std::string& levelPath, Scene& scene) {
	LevelLoadResult result;
	result.levelPath = levelPath;

	LevelData data{};
	if (!LevelSerializer::Load(levelPath, data)) {
		result.failureReason = "Failed to parse level JSON";
		return result;
	}

	const RuntimeLevel::LevelValidationReport validation = RuntimeLevel::ValidateLevelData(levelPath, data);
	result.validationWarnings = validation.warnings;

	// Keep the dependency warm-up close to the structured load path so callers get consistent behavior.
	const std::vector<std::string> textures = BuildTextureManifest(data);
	if (!textures.empty()) {
		ResourceManager::Instance().PreloadTextures(textures);
	}

	result.success = RuntimeLevel::LoadAndBuild(levelPath, scene);
	if (!result.success) {
		result.failureReason = "Runtime scene build failed";
	}

	return result;
}

/**
 * @brief Loads a runtime level JSON file and builds it into the target scene.
 * @param levelPath Path to the level JSON file.
 * @param scene Target scene that should be rebuilt from the runtime level.
 * @return `true` when the load and build completed successfully.
 */
bool RuntimeLevelPipeline::LoadLevelIntoScene(const std::string& levelPath, Scene& scene) {
	return LoadLevelIntoSceneDetailed(levelPath, scene).success;
}
