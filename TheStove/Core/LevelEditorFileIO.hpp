/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelEditorFileIO.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Header for Level Editor file I/O utilities.
					Declares functions for:
					- Opening native file dialogs (Windows)
					- Copying or moving files safely into project folders
					- Listing JSON and asset files
					- Saving and loading prefab data
					- Applying prefab data to scene objects
					- Loading textures bypassing the cache

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

struct LevelObject;
class Scene;
class GameObject;
class Texture;

// Forward declaration of ResourceManager for texture loading
namespace LEFILEIO {

	/**
	 * @brief Opens a native file selection dialog.
	 * @param filter Windows dialog filter string.
	 * @return Absolute path to the selected file, or an empty string when canceled.
	 */
	std::string OpenFileDialog(const char* filter);

	/**
	 * @brief Opens a native folder selection dialog.
	 * @param title Title shown by the folder picker.
	 * @return Absolute path to the selected folder, or an empty string when canceled.
	 */
	std::string OpenFolderDialog(const char* title = "Select Folder");

	// File Operations

	/**
	 * @brief Copies an imported file into a project folder using a unique name.
	 * @param sourcePath Absolute source path.
	 * @param destinationDir Project-relative destination directory.
	 * @return Project-relative path to the copied file, or an empty string on failure.
	 */
	std::string CopyFileIntoProjectUnique(const std::string& sourcePath, const std::string& destinationDir);

	/**
	 * @brief Soft-deletes a file by moving it into a sibling trash folder.
	 * @param filePath File to move.
	 * @return True when the operation succeeds.
	 */
	bool MoveToTrash(const std::string& filePath);

	// Listing

	/**
	 * @brief Lists JSON files in a directory.
	 * @param dir Directory to scan.
	 * @param recursive True to include subdirectories.
	 * @return Relative paths rooted at the provided directory.
	 */
	std::vector<std::string> ListJsonFiles(const std::string& dir, bool recursive = false);

	/**
	 * @brief Lists files with matching lowercase extensions.
	 * @param dir Directory to scan.
	 * @param exts Allowed extensions.
	 * @param recursive True to include subdirectories.
	 * @return Relative paths rooted at the provided directory.
	 */
	std::vector<std::string> ListAssetsWithExt(const std::string& dir, const std::vector<std::string>& exts, bool recursive = false);

	// Prefab Helpers

	/**
	 * @brief Saves a single LevelObject as a prefab JSON file.
	 * @param prefabPath Destination file path.
	 * @param src Object to serialize.
	 * @return True when the save succeeds.
	 */
	bool SavePrefabToFile(std::string prefabPath, const LevelObject& src);

	/**
	 * @brief Loads a prefab JSON file into a LevelObject.
	 * @param prefabPath Prefab file path.
	 * @param out Receives the loaded data.
	 * @return True when loading succeeds.
	 */
	bool LoadPrefabFromFile(const std::string& prefabPath, LevelObject& out);

	/**
	 * @brief Applies prefab data to an existing object while preserving its position.
	 * @param prefab Prefab data to apply.
	 * @param scene Scene containing the object.
	 * @param obj Object to update.
	 */
	void ApplyPrefabToObjectKeepPosition(const LevelObject& prefab, Scene& scene, GameObject* obj);

	// Texture

	/**
	 * @brief Loads a texture with a unique cache key to bypass cached entries.
	 * @param path Texture path to load.
	 * @return Pointer to the loaded texture, or null on failure.
	 */
	Texture* LoadTextureBypassingCache(const std::string& path);
}

