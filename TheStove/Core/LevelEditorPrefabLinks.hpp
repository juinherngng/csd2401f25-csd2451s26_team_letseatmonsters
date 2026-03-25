/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPrefabLinks.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Central registry mapping scene object IDs to their prefab file paths.
                    Used by the Level Editor to:
                    - Remember which instances came from which prefab
                    - Propagate prefab changes to linked instances

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <unordered_map>

/**
 * @namespace LELINKS
 * @brief Shared prefab link table for the Level Editor.
 *
 * Notes:
 * - Key:   Engine/GameObject ID (int)
 * - Value: Prefab JSON path (project-relative or absolute, depending on your pipeline)
 */
namespace LELINKS {
// Global map storing prefab links by object ID.
extern std::unordered_map<int, std::string> PrefabLinkByID;
} // namespace LELINKS

