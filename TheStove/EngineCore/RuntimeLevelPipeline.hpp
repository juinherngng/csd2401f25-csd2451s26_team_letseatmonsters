/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			RuntimeLevelPipeline.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Declares a small runtime level-loading pipeline that centralizes dependency
					preload and scene-build entry points shared by Scene and GameStateManager.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

class Scene;

namespace RuntimeLevelPipeline {
	/**
	 * @brief Structured result returned by the runtime level-load pipeline.
	 */
	struct LevelLoadResult {
		bool success = false;
		std::string levelPath;
		std::string failureReason;
		std::vector<std::string> validationWarnings;
	};

	/**
	 * @brief Collects unique texture dependencies referenced by a runtime level.
	 * @param levelPath Path to the level JSON file.
	 * @return Unique texture paths referenced by the level.
	 */
	std::vector<std::string> CollectTextureDependencies(const std::string& levelPath);

	/**
	 * @brief Preloads all texture dependencies referenced by a runtime level.
	 * @param levelPath Path to the level JSON file.
	 */
	void PreloadLevelDependencies(const std::string& levelPath);

	/**
	 * @brief Loads a runtime level and reports warnings/failure details in a structured result.
	 * @param levelPath Path to the level JSON file.
	 * @param scene Target scene that should be rebuilt from the runtime level.
	 * @return Structured result describing success, warnings, and any failure reason.
	 */
	LevelLoadResult LoadLevelIntoSceneDetailed(const std::string& levelPath, Scene& scene);

	/**
	 * @brief Loads a runtime level JSON file and builds it into the target scene.
	 * @param levelPath Path to the level JSON file.
	 * @param scene Target scene that should be rebuilt from the runtime level.
	 * @return `true` when the load and build completed successfully.
	 */
	bool LoadLevelIntoScene(const std::string& levelPath, Scene& scene);
}
