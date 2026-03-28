/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorFileIO.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu        (90%)
 CO-AUTHORS:        Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:       File I/O helpers used by the Level Editor:
					- Native file open dialog (Windows)
					- Safe copy into project (unique suffix)
					- Soft delete (move to "trash")
					- Directory listings (JSON, assets by extension)
					- Prefab save/load (JSON via LevelSerializer)
					- Apply prefab data to an object (preserve position)
					- Force-bypass texture cache reload

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <imgui.h>
#include <string>
#include <system_error>
#include <vector>

#include "EngineCore/LevelEditorFileIO.hpp"
#include "EngineCore/LevelSerializer.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#endif

namespace fs = std::filesystem;

namespace {

	/**
	 * @brief Returns whether a path contains a specific component.
	 * @param path Path to inspect.
	 * @param componentName Folder name to search for.
	 * @return True when the component exists in the path.
	 */
	bool PathContainsComponent(const fs::path& path, const std::string& componentName) {
		// Compare per component so only exact folder names match.
		for (const auto& part : path) {
			if (part == componentName) {
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Normalizes a directory string to use forward slashes and a trailing slash.
	 * @param dir Directory string to normalize.
	 * @return Normalized directory string.
	 */
	std::string NormalizeDirectoryPath(std::string dir) {
		std::replace(dir.begin(), dir.end(), '\\', '/');
		if (!dir.empty() && dir.back() != '/') {
			dir += '/';
		}

		return dir;
	}

	/**
	 * @brief Appends a child filename to a normalized directory path.
	 * @param normalizedDir Parent directory string.
	 * @param child Child path whose filename should be appended.
	 * @return Relative path built from the directory and filename.
	 */
	std::string BuildRelativeChildPath(const std::string& normalizedDir, const fs::path& child) {
		return normalizedDir + child.filename().string();
	}

	/**
	 * @brief Builds a relative path from a root directory and child path.
	 * @param normalizedDir Normalized root directory string.
	 * @param root Root path used for the relative conversion.
	 * @param child Child path to convert.
	 * @return Relative child path, or a filename fallback if conversion fails.
	 */
	std::string BuildRelativePathFromRoot(const std::string& normalizedDir, const fs::path& root, const fs::path& child) {
		std::error_code ec;
		const fs::path relative = fs::relative(child, root, ec);
		if (ec || relative.empty()) {
			return BuildRelativeChildPath(normalizedDir, child);
		}

		return normalizedDir + relative.generic_string();
	}

	/**
	 * @brief Returns whether a path's extension is in the allowed list.
	 * @param path Path to inspect.
	 * @param normalizedExts Lowercase extensions accepted by the caller.
	 * @return True when the extension matches.
	 */
	bool ExtensionAllowed(const fs::path& path, const std::vector<std::string>& normalizedExts) {
		// Lowercase the extension first so matching remains case-insensitive.
		std::string ext = path.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		return std::find(normalizedExts.begin(), normalizedExts.end(), ext) != normalizedExts.end();
	}

	/**
	 * @brief Generates a unique destination path by appending numeric suffixes.
	 * @param destinationDir Directory that should contain the file.
	 * @param baseName Desired filename.
	 * @return A unique path inside the destination directory.
	 */
	fs::path MakeUniquePath(const fs::path& destinationDir, const fs::path& baseName) {
		std::error_code ec;
		fs::path candidate = destinationDir / baseName;
		int suffix = 1;

		while (fs::exists(candidate, ec)) {
			candidate = destinationDir /
				(baseName.stem().string() + " (" + std::to_string(suffix++) + ")" + baseName.extension().string());
		}

		return candidate;
	}
}

namespace LEFILEIO {

	/**
	 * @brief Opens a native Windows file dialog.
	 * @param filter Windows dialog filter string.
	 * @return Selected file path, or an empty string when canceled.
	 */
	std::string OpenFileDialog(const char* filter) {
#ifdef _WIN32
		// Save the current working directory before opening the dialog
		char originalCwd[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, originalCwd);

		char filePathBuffer[MAX_PATH] = { 0 };

		OPENFILENAMEA ofn{};
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = filePathBuffer;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR; // Add OFN_NOCHANGEDIR flag

		bool result = GetOpenFileNameA(&ofn);

		// Restore the original working directory after the dialog closes
		SetCurrentDirectoryA(originalCwd);

		if (result) {
			return std::string(filePathBuffer);
		}
#endif
		return {};
	}

	/**
	 * @brief Opens a native Windows folder picker.
	 * @param title Title shown by the dialog.
	 * @return Selected folder path, or an empty string when canceled.
	 */
	std::string OpenFolderDialog(const char* title) {
#ifdef _WIN32
		// Preserve the working directory so the dialog does not disturb relative asset paths.
		char originalCwd[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, originalCwd);

		BROWSEINFOA browseInfo{};
		browseInfo.lpszTitle = title;
		browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;

		PIDLIST_ABSOLUTE pidList = SHBrowseForFolderA(&browseInfo);
		if (pidList == nullptr) {
			SetCurrentDirectoryA(originalCwd);
			return {};
		}

		char folderPath[MAX_PATH] = { 0 };
		const BOOL ok = SHGetPathFromIDListA(pidList, folderPath);
		CoTaskMemFree(pidList);
		SetCurrentDirectoryA(originalCwd);

		if (ok) {
			return std::string(folderPath);
		}
#endif
		return {};
	}

	/**
	 * @brief Copies an imported file into a project directory with a collision-safe name.
	 * @param sourcePath Source file selected by the user.
	 * @param destinationDir Project-relative destination directory.
	 * @return Project-relative copied path, or an empty string on failure.
	 */
	std::string CopyFileIntoProjectUnique(const std::string& sourcePath, const std::string& destinationDir) {
		if (sourcePath.empty()) {
			return {};
		}

		std::error_code ec;

		const fs::path src(sourcePath);
		if (!fs::exists(src, ec)) {
			return {};
		}

		const fs::path dstDir(destinationDir);
		if (!fs::exists(dstDir, ec)) {
			fs::create_directories(dstDir, ec);
			if (ec) {
				return {};
			}
		}

		const fs::path baseName = src.filename();
		const fs::path dst = MakeUniquePath(dstDir, baseName);

		fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
		if (ec) {
			return {};
		}

		// Return a project-relative path so serialized editor data stays portable across machines.
		const std::string normalizedDir = NormalizeDirectoryPath(destinationDir);
		const std::string relativePath = BuildRelativeChildPath(normalizedDir, dst);

		TS_LOG_INFO("[CopyFileIntoProjectUnique] Returning path: " << relativePath);
		return relativePath;
	}

	/**
	 * @brief Moves a file into a sibling trash folder.
	 * @param filePath File to move.
	 * @return True when the file is moved successfully.
	 */
	bool MoveToTrash(const std::string& filePath) {
		if (filePath.empty()) {
			return false;
		}

		std::error_code ec;
		fs::path src(filePath);

		if (!fs::exists(src, ec)) {
			ec.clear();
			const fs::path fallback = fs::current_path(ec) / src;
			if (!ec && fs::exists(fallback, ec)) {
				src = fallback;
			}
		}

		if (!fs::exists(src, ec)) {
			return false;
		}

		const fs::path trashDir = src.parent_path() / "trash";
		if (!fs::exists(trashDir, ec)) {
			fs::create_directories(trashDir, ec);
			if (ec) {
				return false;
			}
		}

		const fs::path dst = MakeUniquePath(trashDir, src.filename());
		fs::rename(src, dst, ec);
		if (!ec) {
			return true;
		}

		// Fall back to copy+delete when rename fails across filesystem boundaries.
		ec.clear();
		fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
		if (ec) {
			return false;
		}

		ec.clear();
		fs::remove(src, ec);
		if (ec) {
			std::error_code cleanupEc;
			fs::remove(dst, cleanupEc);
			return false;
		}

		return true;
	}

	/**
	 * @brief Lists JSON files inside a directory.
	 * @param dir Directory to scan.
	 * @param recursive True to include subdirectories.
	 * @return Relative JSON paths rooted at the provided directory.
	 */
	std::vector<std::string> ListJsonFiles(const std::string& dir, bool recursive) {
		return ListAssetsWithExt(dir, { ".json" }, recursive);
	}

	/**
	 * @brief Lists files with the requested extensions.
	 * @param dir Directory to scan.
	 * @param extensions Allowed lowercase extensions.
	 * @param recursive True to include subdirectories.
	 * @return Sorted relative asset paths.
	 */
	std::vector<std::string> ListAssetsWithExt(const std::string& dir, const std::vector<std::string>& extensions, bool recursive) {
		std::vector<std::string> out;
		std::error_code ec;

		if (!fs::exists(dir, ec)) {
			return out;
		}

		const fs::path rootPath(dir);
		const std::string normalizedDir = NormalizeDirectoryPath(dir);
		std::vector<std::string> normalizedExts;
		normalizedExts.reserve(extensions.size());
		for (std::string ext : extensions) {
			std::transform(ext.begin(), ext.end(), ext.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			normalizedExts.push_back(std::move(ext));
		}

		auto appendIfMatch = [&](const fs::directory_entry& entry) {
			if (!entry.is_regular_file()) {
				return;
			}

			// Ignore soft-deleted files so editor lists only show active content.
			const fs::path relativePath = fs::relative(entry.path(), rootPath, ec);
			if (!ec && PathContainsComponent(relativePath, "trash")) {
				return;
			}

			ec.clear();

			if (ExtensionAllowed(entry.path(), normalizedExts)) {
				out.push_back(BuildRelativePathFromRoot(normalizedDir, rootPath, entry.path()));
			}
			};

		if (recursive) {
			for (auto it = fs::recursive_directory_iterator(rootPath, ec); !ec && it != fs::recursive_directory_iterator(); ++it) {
				const auto& entry = *it;

				if (entry.is_directory() && entry.path().filename() == "trash") {
					it.disable_recursion_pending();
					continue;
				}

				if (ec) {
					break;
				}

				appendIfMatch(entry);
			}
		}
		else {
			for (const auto& entry : fs::directory_iterator(rootPath, ec)) {
				if (ec) {
					break;
				}

				appendIfMatch(entry);
			}
		}

		std::sort(out.begin(), out.end());
		return out;
	}

	/**
	 * @brief Saves a single LevelObject as a prefab JSON file.
	 * @param prefabPath Output path.
	 * @param src Source object to serialize.
	 * @return True when the prefab save succeeds.
	 */
	bool SavePrefabToFile(std::string prefabPath, const LevelObject& src) {
		if (fs::path(prefabPath).extension().empty()) {
			prefabPath += ".json";
		}

		std::error_code ec;
		const fs::path dir = fs::path(prefabPath).parent_path();

		if (!dir.empty() && !fs::exists(dir, ec)) {
			fs::create_directories(dir, ec);
			if (ec) {
				return false;
			}
		}

		LevelData one;
		one.objects.clear();
		one.objects.push_back(src);

		return LevelSerializer::Save(prefabPath, one);
	}

	/**
	 * @brief Loads a single LevelObject prefab from disk.
	 * @param prefabPath Prefab file path.
	 * @param out Receives the loaded data.
	 * @return True when loading succeeds.
	 */
	bool LoadPrefabFromFile(const std::string& prefabPath, LevelObject& out) {
		LevelData one;

		if (!LevelSerializer::Load(prefabPath, one) || one.objects.empty()) {
			return false;
		}

		out = one.objects.front();
		return true;
	}

	/**
	 * @brief Applies prefab dimensions, collider data, and texture while preserving position.
	 * @param prefab Prefab data to apply.
	 * @param scene Scene containing the object.
	 * @param obj Target object to update.
	 */
	void ApplyPrefabToObjectKeepPosition(const LevelObject& prefab, Scene& scene, GameObject* obj) {
		if (obj == nullptr) {
			return;
		}

		const int id = obj->GetID();
		const glm::vec3 keepPos = obj->GetPositionGLM();
		const std::string resolvedLayer = prefab.layer.empty() ? "1" : prefab.layer;

		const float w = prefab.w;
		const float h = prefab.h;

		obj->SetScale(glm::vec3(w, h, 1.0f));
		if (prefab.hasCollider) {
			obj->SetColliderSize({ prefab.colWidth, prefab.colHeight });
			obj->SetColliderOffset({ prefab.colOffsetX, prefab.colOffsetY });
		}
		else {
			obj->SetColliderSize({ 0.0f, 0.0f });
			obj->SetColliderOffset({ 0.0f, 0.0f });
		}

		// Reapply the transform using editor rotation units so runtime state matches serialized data.
		scene.SetTransformFromLevel(id, obj->GetPositionGLM(), { w, h, 1.0f }, prefab.rotation);
		scene.SetObjectTexturePath(id, prefab.texture);
		scene.AssignObjectToLayer(id, resolvedLayer);

		if (auto* tex = ResourceManager::Instance().LoadTexture("sprite_" + prefab.texture, prefab.texture)) {
			obj->SetTexture(tex);
		}

		Scene::Defaults defaults = scene.GetDefaults(id);
		defaults.pos = keepPos;
		defaults.size = { prefab.w, prefab.h, 1.0f };
		defaults.rot = prefab.rotation;
		defaults.colSize = prefab.hasCollider ? Math::Vector2D{ prefab.colWidth, prefab.colHeight } : Math::Vector2D{ 0.0f, 0.0f };
		defaults.colOff = prefab.hasCollider ? Math::Vector2D{ prefab.colOffsetX, prefab.colOffsetY } : Math::Vector2D{ 0.0f, 0.0f };
		defaults.vel = { prefab.speedX, prefab.speedY };
		defaults.approachOffset = { prefab.approachOffsetX, prefab.approachOffsetY };
		defaults.hasApproachOffset2 = prefab.hasApproachOffset2;
		defaults.approachOffset2 = { prefab.approachOffset2X, prefab.approachOffset2Y };
		defaults.hasCustomerSeatOffset = prefab.hasCustomerSeatOffset;
		defaults.customerSeatOffset = { prefab.customerSeatOffsetX, prefab.customerSeatOffsetY };
		defaults.customerSeatCapacity = prefab.customerSeatCapacity;
		defaults.hasCustomerSeatOffset2 = prefab.hasCustomerSeatOffset2;
		defaults.customerSeatOffset2 = { prefab.customerSeatOffset2X, prefab.customerSeatOffset2Y };
		defaults.texture = prefab.texture;
		defaults.tag = prefab.tag;
		defaults.layer = resolvedLayer;
		defaults.audioOnSpawn = prefab.audioOnSpawn;
		defaults.audioOnInteract = prefab.audioOnInteract;
		defaults.audioOnDestroy = prefab.audioOnDestroy;
		defaults.audioOnProcessing = prefab.audioOnProcessing;
		defaults.audioLoop = prefab.audioLoop;
		defaults.visible = prefab.visible;
		scene.SetDefaults(id, defaults);

		scene.SetObjectTag(id, prefab.tag);
		scene.SetNPCVelocity(id, prefab.speedX, prefab.speedY);
		scene.SetObjectVisible(id, prefab.visible);
		scene.ApplyTagRules(id, prefab.tag, prefab.speedX, prefab.speedY);
		scene.AttachLogicForTag(id, prefab.tag);
		scene.MarkAnimated(id, prefab.animated);
		if (prefab.animated && !prefab.animName.empty() && scene.HasAnimations(id)) {
			scene.SetAnimation(id, prefab.animName);
		}
		else if (!prefab.animated) {
			obj->SetUVRect({ 0.0f, 0.0f, 1.0f, 1.0f });
		}

		obj->SetPosition(keepPos);
		scene.ClampToWalkArea(obj);
		scene.RebuildColliders();
	}

	/**
	 * @brief Loads a texture with a unique cache key to bypass cached entries.
	 * @param path Texture path to load.
	 * @return Loaded texture pointer, or null on failure.
	 */
	Texture* LoadTextureBypassingCache(const std::string& path) {
		// Add a time-based suffix so repeated imports always request a fresh texture from the manager.
		const std::uint64_t tick = static_cast<std::uint64_t>(ImGui::GetTime() * 1'000'000.0);
		const std::string key = "sprite_" + path + "#v" + std::to_string(tick);

		return ResourceManager::Instance().LoadTexture(key, path);
	}
}
