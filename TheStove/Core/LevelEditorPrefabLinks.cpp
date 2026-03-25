/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPrefabLinks.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Definition of the prefab link registry used by the Level Editor.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "LevelEditorPrefabLinks.hpp"

namespace LELINKS {
	// Global registry for prefab links
	std::unordered_map<int, std::string> PrefabLinkByID;
}

