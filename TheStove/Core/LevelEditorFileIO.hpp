/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelEditorFileIO.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Header for Level Editor file I/O utilities.
					Declares functions for:
					- Opening native file dialogs (Windows)
					- Copying or moving files safely into project folders
					- Listing JSON and asset files
					- Saving and loading prefab data
					- Applying prefab data to scene objects
					- Loading textures bypassing the cache

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

struct LevelObject;
class Scene;
class GameObject;
class Texture;

/**
 * @namespace LEFILEIO
 * @brief Provides file management and prefab serialization helpers for the Level Editor.
 */
namespace LEFILEIO {
	// Opens a native file selection dialog.
	std::string OpenFileDialog(const char* filter);

	// ----- File Operations -----

	// Copies an external file into a project folder (e.g., ../assets).
	// Automatically adds a numeric suffix if a file with the same name exists.
	std::string CopyFileIntoProjectUnique(const std::string& sourcePath, const std::string& destinationDir);

	// Moves a file into a sibling "trash" folder instead of deleting.
	bool MoveToTrash(const std::string& filePath);

	// ----- Listing -----

	// Lists all .json files in a directory (non-recursive). Returns relative paths.
	std::vector<std::string> ListJsonFiles(const std::string& dir);

	// Lists all files with matching lowercase extensions. Returns relative paths.
	std::vector<std::string> ListAssetsWithExt(const std::string& dir, const std::vector<std::string>& exts);

	// ----- Prefab Helpers -----

	// Saves a LevelObject prefab to a JSON file (creates directories if needed).
	bool SavePrefabToFile(std::string prefabPath, const LevelObject& src);

	// Loads a prefab JSON file into a LevelObject.
	bool LoadPrefabFromFile(const std::string& prefabPath, LevelObject& out);

	// Applies prefab data (scale, collider, texture) to an existing GameObject while keeping its current position unchanged.
	void ApplyPrefabToObjectKeepPosition(const LevelObject& prefab, Scene& scene, GameObject* obj);

	// ----- Texture -----

	// Loads a texture using a unique cache key, bypassing ResourceManager's cache. Useful for reloading modified textures at runtime.
	Texture* LoadTextureBypassingCache(const std::string& path);
}
