/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorFileIO.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu

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
		fs::path dst = dstDir / baseName;

		int suffix = 1;
		while (fs::exists(dst, ec)) {
			dst = dstDir / (baseName.stem().string() + " (" + std::to_string(suffix++) + ")" + baseName.extension().string());
		}

		fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
		if (ec) {
			return {};
		}

		// IMPORTANT: Don't use dst.generic_string() - it may resolve to absolute path
		// Instead, manually construct the relative path string from the original destinationDir
		std::string resultFilename = dst.filename().string();
		
		// Ensure destinationDir ends with forward slash for consistent concatenation
		std::string normalizedDir = destinationDir;
		std::replace(normalizedDir.begin(), normalizedDir.end(), '\\', '/');
		if (!normalizedDir.empty() && normalizedDir.back() != '/') {
			normalizedDir += '/';
		}
		
		std::string relativePath = normalizedDir + resultFilename;
		
		std::cout << "[CopyFileIntoProjectUnique] Returning path: " << relativePath << std::endl;
		return relativePath;
	}

	// Move to trash
	bool MoveToTrash(const std::string& filePath) {
		std::error_code ec;

		const fs::path src(filePath);
		if (!fs::exists(src, ec)) {
			return false;
		}

		const fs::path trashDir = src.parent_path() / "trash";
		if (!fs::exists(trashDir, ec)) {
			fs::create_directories(trashDir, ec);
		}

		const fs::path dst = trashDir / src.filename();
		fs::rename(src, dst, ec);

		return !ec;
	}

	// List all .json files (non-recursive) in a directory, sorted by name.
	// Returns relative paths.
	std::vector<std::string> ListJsonFiles(const std::string& dir) {
		std::vector<std::string> out;
		std::error_code ec;

		if (!fs::exists(dir, ec)) {
			return out;
		}

		// Normalize directory path
		std::string normalizedDir = dir;
		std::replace(normalizedDir.begin(), normalizedDir.end(), '\\', '/');
		if (!normalizedDir.empty() && normalizedDir.back() != '/') {
			normalizedDir += '/';
		}

		for (const auto& p : fs::directory_iterator(dir, ec)) {
			if (p.is_regular_file() && p.path().extension() == ".json") {
				// Manually construct relative path to avoid fs::path converting to absolute
				std::string filename = p.path().filename().string();
				std::string relativePath = normalizedDir + filename;
				out.push_back(relativePath);
			}
		}

		std::sort(out.begin(), out.end());
		return out;
	}

	// List files with specific lowercase extensions (e.g., {".png",".jpg"}).
	// Returns sorted list of relative paths (relative to current working directory).
	std::vector<std::string> ListAssetsWithExt(const std::string& dir, const std::vector<std::string>& extensions) {
		std::vector<std::string> out;
		std::error_code ec;

		if (!fs::exists(dir, ec)) {
			return out;
		}

		// Normalize directory path
		std::string normalizedDir = dir;
		std::replace(normalizedDir.begin(), normalizedDir.end(), '\\', '/');
		if (!normalizedDir.empty() && normalizedDir.back() != '/') {
			normalizedDir += '/';
		}

		for (const auto& p : fs::directory_iterator(dir, ec)) {
			if (!p.is_regular_file()) {
				continue;
			}

			std::string ext = p.path().extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(),
						   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

			for (const auto& e : extensions) {
				if (ext == e) {
					// Manually construct relative path to avoid fs::path converting to absolute
					std::string filename = p.path().filename().string();
					std::string relativePath = normalizedDir + filename;
					out.push_back(relativePath);
					break;
				}
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
