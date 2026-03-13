/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorFileIO.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu		(90%)
 CO-AUTHORS:		Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:       File I/O helpers used by the Level Editor:
					- Native file open dialog (Windows)
					- Safe copy into project (unique suffix)
					- Soft delete (move to "trash")
					- Directory listings (JSON, assets by extension)
					- Prefab save/load (JSON via LevelSerializer)
					- Apply prefab data to an object (preserve position)
					- Force-bypass texture cache reload

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <system_error>
#include <vector>
#include <string>
#include <imgui.h>
#include <glm/glm.hpp>

#include "../Graphics/GameObject.hpp"
#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"

#include "LevelEditorFileIO.hpp"
#include "LevelSerializer.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif

namespace fs = std::filesystem;

// Helper functions for path normalization, relative path construction, extension filtering, and unique path generation
namespace {
	// Checks if the given path contains a specific component (e.g., "assets" or "prefabs")
	bool PathContainsComponent(const fs::path& path, const std::string& componentName) {
		for (const auto& part : path) {
			if (part == componentName) {
				return true;
			}
		}

		return false;
	}

	std::string NormalizeDirectoryPath(std::string dir) {
		std::replace(dir.begin(), dir.end(), '\\', '/');
		if (!dir.empty() && dir.back() != '/') {
			dir += '/';
		}

		return dir;
	}

	std::string BuildRelativeChildPath(const std::string& normalizedDir, const fs::path& child) {
		return normalizedDir + child.filename().string();
	}

	std::string BuildRelativePathFromRoot(const std::string& normalizedDir, const fs::path& root, const fs::path& child) {
		std::error_code ec;
		const fs::path relative = fs::relative(child, root, ec);
		if (ec || relative.empty()) {
			return BuildRelativeChildPath(normalizedDir, child);
		}

		return normalizedDir + relative.generic_string();
	}

	bool ExtensionAllowed(const fs::path& path, const std::vector<std::string>& normalizedExts) {
		std::string ext = path.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		return std::find(normalizedExts.begin(), normalizedExts.end(), ext) != normalizedExts.end();
	}

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
	// Open a native file dialog (Windows). Returns empty string if canceled.
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

	// Copy a file into the project directory, ensuring unique name by suffixing
	// " (n)" if a collision occurs. Returns project-relative path or empty on fail.
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

		// IMPORTANT: Don't use dst.generic_string() - it may resolve to absolute path
		// Instead, manually construct the relative path string from the original destinationDir
		const std::string normalizedDir = NormalizeDirectoryPath(destinationDir);
		const std::string relativePath = BuildRelativeChildPath(normalizedDir, dst);

		std::cout << "[CopyFileIntoProjectUnique] Returning path: " << relativePath << std::endl;
		return relativePath;
	}

	// Move to trash
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

		// Fallback when rename fails (e.g., crossing filesystems).
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

	// List all .json files in a directory.
	// Returns relative paths.
	std::vector<std::string> ListJsonFiles(const std::string& dir, bool recursive) {
		return ListAssetsWithExt(dir, { ".json" }, recursive);
	}

	// List files with specific lowercase extensions (e.g., {".png",".jpg"}).
	// Returns sorted list of relative paths (relative to the provided directory).
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

			// Exclude any files in "trash" subdirectories at any level
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

	// Save a single-object prefab to JSON file. Ensures parent directory exists.
	// Adds ".json" if path has no extension. Returns success.
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

	// Load a single-object prefab from JSON file into 'out'. Returns success.
	bool LoadPrefabFromFile(const std::string& prefabPath, LevelObject& out) {
		LevelData one;

		if (!LevelSerializer::Load(prefabPath, one) || one.objects.empty()) {
			return false;
		}

		out = one.objects.front();
		return true;
	}

	// Apply prefab dimensions/collider/texture to an object but keep its position.
	// Also clamps to the scene's walk area after applying changes.
	void ApplyPrefabToObjectKeepPosition(const LevelObject& prefab, Scene& scene, GameObject* obj) {
		if (obj == nullptr) {
			return;
		}

		const int id = obj->GetID();
		const glm::vec3 keepPos = obj->GetPositionGLM();

		const float w = prefab.w;
		const float h = prefab.h;

		obj->SetScale(glm::vec3(w, h, 1.0f));
		obj->SetColliderSize({ prefab.colWidth, prefab.colHeight });
		obj->SetColliderOffset({ prefab.colOffsetX, prefab.colOffsetY });

		// Rotation is presumed to be in DEGREES from JSON/editor.
		scene.SetTransformFromLevel(id, obj->GetPositionGLM(), { w, h, 1.0f }, prefab.rotation);
		scene.SetObjectTexturePath(id, prefab.texture);

		if (auto* tex = ResourceManager::Instance().LoadTexture("sprite_" + prefab.texture, prefab.texture)) {
			obj->SetTexture(tex);
		}

		obj->SetPosition(keepPos);
		scene.ClampToWalkArea(obj);
	}

	// Load a texture with a unique cache key to bypass ResourceManager cache.
	// Useful for forcing a re-import of modified assets at runtime.
	Texture* LoadTextureBypassingCache(const std::string& path) {
		const std::uint64_t tick = static_cast<std::uint64_t>(ImGui::GetTime() * 1'000'000.0);
		const std::string key = "sprite_" + path + "#v" + std::to_string(tick);

		return ResourceManager::Instance().LoadTexture(key, path);
	}
}
