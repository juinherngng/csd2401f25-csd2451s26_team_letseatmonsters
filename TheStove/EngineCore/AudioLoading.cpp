/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			AudioLoading.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (60%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu		(40%)

 DESCRIPTION:		Implementation of JSON-based audio catalog with serialization.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

#include "EngineCore/AudioLoading.hpp"
#include "EngineCore/JSONInclude.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineGraphics/ResourceManager.hpp"

using nlohmann::json;

namespace Audio {
	// Static storage for audio assets
	std::vector<AudioAsset> AudioCatalog::s_AudioAssets;

	// Helper function to normalize paths (convert backslashes to forward slashes)
	static std::string NormalizeAudioPath(const std::string& path) {
		std::string normalized = path;
		std::replace(normalized.begin(), normalized.end(), '\\', '/');
		return normalized;
	}

	static std::string CompactAudioFileLabel(const std::string& path) {
		if (path.empty()) {
			return path;
		}

		return std::filesystem::path(path).filename().string();
	}

	// Helper function to convert editor paths to runtime paths for release builds
	// Editor paths: ../../assets/Audio/file.mp3 (from build/Release)
	// Runtime paths: ../assets/Audio/file.mp3 (from release/ folder)
	static std::string ConvertToRuntimePath(const std::string& path) {
		std::string result = path;

		// Convert ../../assets/ to ../assets/ for release builds
		const std::string editorPrefix = "../../assets/";
		const std::string runtimePrefix = "../assets/";

		if (result.find(editorPrefix) == 0) {
			result = runtimePrefix + result.substr(editorPrefix.length());
		}

		return result;
	}

	bool AudioCatalog::LoadCatalogFromFile(const std::string& catalogPath) {
		TS_LOG_INFO("AudioCatalog: Loading catalog from " << catalogPath << "...");

		// Show current working directory for debugging
		std::filesystem::path cwd = std::filesystem::current_path();
		TS_LOG_DEBUG("[LoadCatalog] Current working directory: " << cwd.string());

		// Show absolute path
		std::filesystem::path absolutePath = std::filesystem::absolute(catalogPath);
		TS_LOG_DEBUG("[LoadCatalog] Absolute path: " << absolutePath.string());
		TS_LOG_DEBUG("[LoadCatalog] File exists: " << (std::filesystem::exists(absolutePath) ? "YES" : "NO"));

		std::ifstream file(catalogPath);
		if (!file.is_open()) {
			TS_LOG_ERROR("AudioCatalog: Failed to open catalog file: " << catalogPath);
			TS_LOG_ERROR("[LoadCatalog] Attempted to read from: " << absolutePath.string());
			return false;
		}

		// Determine if we need to convert paths (release builds use shorter paths)
#ifdef _DEBUG
		const bool convertPaths = false;
#else
		const bool convertPaths = true;
#endif

		try {
			json catalogJson;
			file >> catalogJson;

			// Clear existing assets
			s_AudioAssets.clear();

			// Parse version (for future compatibility)
			std::string version = catalogJson.value("version", "1.0");
			TS_LOG_DEBUG("Catalog version: " << version);

			// Parse audio assets
			if (catalogJson.contains("audio_assets") && catalogJson["audio_assets"].is_array()) {
				for (const auto& assetJson : catalogJson["audio_assets"]) {
					AudioAsset asset;
					asset.name = assetJson.value("name", "");

					// Normalize the filepath when loading from JSON
					std::string rawPath = assetJson.value("filepath", "");
					asset.filepath = NormalizeAudioPath(rawPath);

					// Convert editor paths to runtime paths for release builds
					if (convertPaths) {
						asset.filepath = ConvertToRuntimePath(asset.filepath);
					}

					asset.loop = assetJson.value("loop", false);
					asset.stream = assetJson.value("stream", false);
					asset.category = assetJson.value("category", "");
					asset.volume = assetJson.value("volume", 1.0f);

					// Validate the asset
					if (asset.name.empty() || asset.filepath.empty()) {
						TS_LOG_WARN("AudioCatalog: Skipping invalid audio asset entry");
						continue;
					}

					// Validate file format
					if (!IsValidAudioFile(asset.filepath)) {
						TS_LOG_WARN("AudioCatalog: Unsupported audio format for " << asset.filepath
							<< " (only .wav and .mp3 are supported)");
						continue;
					}

					s_AudioAssets.push_back(asset);
					TS_LOG_DEBUG("Loaded asset: " << asset.name << " (path: " << asset.filepath << ")");
				}
			}
			else {
				TS_LOG_INFO("AudioCatalog: No 'audio_assets' array found in catalog file (empty catalog)");
			}

			TS_LOG_INFO("AudioCatalog: Successfully loaded " << s_AudioAssets.size() << " audio assets from file.");
			return true;
		}
		catch (const json::exception& e) {
			TS_LOG_ERROR("AudioCatalog: JSON parsing error: " << e.what());
			return false;
		}
	}

	bool AudioCatalog::SaveCatalogToFile(const std::string& catalogPath) {
		TS_LOG_INFO("AudioCatalog: Saving catalog to " << catalogPath << "...");

		try {
			// Show current working directory for debugging
			std::filesystem::path cwd = std::filesystem::current_path();
			TS_LOG_DEBUG("[SaveCatalog] Current working directory: " << cwd.string());

			// Ensure parent directory exists
			std::filesystem::path filePath(catalogPath);
			std::filesystem::path parentDir = filePath.parent_path();

			// Log the absolute path where we're actually writing
			std::filesystem::path absolutePath = std::filesystem::absolute(filePath);
			TS_LOG_DEBUG("[SaveCatalog] Absolute path: " << absolutePath.string());

			if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
				TS_LOG_INFO("AudioCatalog: Creating directory: " << parentDir.string());
				std::filesystem::create_directories(parentDir);
			}

			json catalogJson;
			catalogJson["version"] = "1.0";
			catalogJson["audio_assets"] = json::array();

			// Serialize all audio assets
			for (const auto& asset : s_AudioAssets) {
				TS_LOG_DEBUG("[SaveCatalog] Saving asset '" << asset.name << "' with filepath: " << asset.filepath);

				json assetJson;
				assetJson["name"] = asset.name;
				assetJson["filepath"] = asset.filepath;
				assetJson["loop"] = asset.loop;
				assetJson["stream"] = asset.stream;
				assetJson["category"] = asset.category;
				assetJson["volume"] = asset.volume;

				catalogJson["audio_assets"].push_back(assetJson);
			}

			// Write to file with pretty printing
			std::ofstream file(catalogPath);
			if (!file.is_open()) {
				TS_LOG_ERROR("AudioCatalog: Failed to open file for writing: " << catalogPath);
				return false;
			}

			file << catalogJson.dump(2); // 2-space indentation
			file.flush(); // Explicitly flush the buffer
			file.close();

			// Verify the file was actually written by checking its existence and size
			if (std::filesystem::exists(absolutePath)) {
				auto fileSize = std::filesystem::file_size(absolutePath);
				TS_LOG_INFO("AudioCatalog: Successfully saved " << s_AudioAssets.size() << " audio assets to " << catalogPath);
				TS_LOG_DEBUG("[SaveCatalog] File size: " << fileSize << " bytes");
				TS_LOG_DEBUG("[SaveCatalog] File written to: " << absolutePath.string());

				// Read back the file to verify it contains the data
				std::ifstream verifyFile(catalogPath);
				if (verifyFile.is_open()) {
					std::string line;
					int lineCount = 0;
					while (std::getline(verifyFile, line) && lineCount < 5) {
						TS_LOG_DEBUG("[SaveCatalog] Line " << lineCount++ << ": " << line);
					}
					verifyFile.close();
				}

				return true;
			}
			else {
				TS_LOG_ERROR("AudioCatalog: File does not exist after writing");
				return false;
			}
		}
		catch (const json::exception& e) {
			TS_LOG_ERROR("AudioCatalog: JSON serialization error: " << e.what());
			return false;
		}
	}

	void AudioCatalog::LoadAllAudio() {
		TS_LOG_INFO("AudioCatalog: Loading all audio assets into memory...");

		auto& resMgr = ResourceManager::Instance();
		int successCount = 0;
		int failCount = 0;

		for (const auto& asset : s_AudioAssets) {
			TS_LOG_DEBUG("[LoadCatalog] Loading '" << asset.name << "' from '" << CompactAudioFileLabel(asset.filepath) << "'.");
			TS_LOG_DEBUG("[LoadCatalog] loop=" << (asset.loop ? "true" : "false")
				<< ", stream=" << (asset.stream ? "true" : "false")
				<< ", category=" << asset.category
				<< ", volume=" << asset.volume);

			// Load SFX/VFX sounds as 3D for spatial audio; BGM and UI stay 2D
			bool isSpatial = (asset.category == "sfx" || asset.category == "vfx");
			bool loaded = isSpatial
				? resMgr.LoadAudio3D(asset.name, asset.filepath, asset.loop, asset.stream)
				: resMgr.LoadAudio(asset.name, asset.filepath, asset.loop, asset.stream);

			if (loaded) {
				// Get and display audio info
				unsigned int lenMs = 0;
				int channels = 0, bits = 0;
				float freq = 0.0f;

				if (resMgr.GetAudioInfo(asset.name, lenMs, channels, bits, freq)) {
					TS_LOG_DEBUG("Info: " << lenMs << "ms, " << channels << " channels, "
						<< bits << " bits, " << freq << "Hz");
				}

				successCount++;
			}
			else {
				TS_LOG_ERROR("AudioCatalog: Failed to load asset '" << asset.name << "'");
				failCount++;
			}
		}

		TS_LOG_INFO("AudioCatalog: Loaded " << successCount << " audio assets successfully"
			<< (failCount > 0 ? " (" + std::to_string(failCount) + " failed)" : std::string{}) << ".");
	}

	void AudioCatalog::UnloadAllAudio() {
		TS_LOG_INFO("AudioCatalog: Unloading all audio assets...");

		auto& resMgr = ResourceManager::Instance();

		for (const auto& asset : s_AudioAssets) {
			TS_LOG_DEBUG("Unloading: " << asset.name);
			resMgr.UnloadAudio(asset.name);
		}

		TS_LOG_INFO("AudioCatalog: All audio assets unloaded.");
	}

	bool AudioCatalog::AddAudioAsset(const AudioAsset& asset) {
		// Normalize the filepath before adding
		AudioAsset normalizedAsset = asset;
		normalizedAsset.filepath = NormalizeAudioPath(asset.filepath);

		// Check if asset with same name already exists
		for (const auto& existing : s_AudioAssets) {
			if (existing.name == normalizedAsset.name) {
				TS_LOG_ERROR("AudioCatalog: Asset with name '" << normalizedAsset.name << "' already exists");
				return false;
			}
		}

		// Validate file format
		if (!IsValidAudioFile(normalizedAsset.filepath)) {
			TS_LOG_ERROR("AudioCatalog: " << GetInvalidFormatMessage(normalizedAsset.filepath));
			return false;
		}

		s_AudioAssets.push_back(normalizedAsset);
		TS_LOG_INFO("AudioCatalog: Added asset: " << normalizedAsset.name << " (path: " << normalizedAsset.filepath << ")");
		return true;
	}

	bool AudioCatalog::RemoveAudioAsset(const std::string& name) {
		auto it = std::find_if(s_AudioAssets.begin(), s_AudioAssets.end(),
			[&name](const AudioAsset& asset) { return asset.name == name; });

		if (it != s_AudioAssets.end()) {
			s_AudioAssets.erase(it);
			TS_LOG_INFO("AudioCatalog: Removed asset: " << name);

			// Also unload from audio system
			ResourceManager::Instance().UnloadAudio(name);
			return true;
		}

		TS_LOG_WARN("AudioCatalog: Asset '" << name << "' not found");
		return false;
	}

	bool AudioCatalog::ReplaceAudioAsset(const std::string& originalName, const AudioAsset& updatedAsset) {
		AudioAsset normalizedAsset = updatedAsset;
		normalizedAsset.filepath = NormalizeAudioPath(updatedAsset.filepath);

		if (!IsValidAudioFile(normalizedAsset.filepath)) {
			TS_LOG_ERROR("AudioCatalog: " << GetInvalidFormatMessage(normalizedAsset.filepath));
			return false;
		}

		auto existingIt = std::find_if(s_AudioAssets.begin(), s_AudioAssets.end(),
			[&originalName](const AudioAsset& asset) { return asset.name == originalName; });
		if (existingIt == s_AudioAssets.end()) {
			TS_LOG_WARN("AudioCatalog: Asset '" << originalName << "' not found");
			return false;
		}

		for (const auto& existing : s_AudioAssets) {
			if (existing.name == normalizedAsset.name && existing.name != originalName) {
				TS_LOG_ERROR("AudioCatalog: Asset with name '" << normalizedAsset.name << "' already exists");
				return false;
			}
		}

		*existingIt = normalizedAsset;
		TS_LOG_INFO("AudioCatalog: Replaced asset '" << originalName << "' with '" << normalizedAsset.name << "'");
		return true;
	}

	const AudioAsset* AudioCatalog::GetAudioAsset(const std::string& name) {
		auto it = std::find_if(s_AudioAssets.begin(), s_AudioAssets.end(),
			[&name](const AudioAsset& asset) { return asset.name == name; });

		if (it != s_AudioAssets.end()) {
			return &(*it);
		}

		return nullptr;
	}

	const std::vector<AudioAsset>& AudioCatalog::GetAllAssets() {
		return s_AudioAssets;
	}

	bool AudioCatalog::IsValidAudioFile(const std::string& filepath) {
		if (filepath.empty()) {
			return false;
		}

		// Extract extension
		size_t dotPos = filepath.find_last_of('.');
		if (dotPos == std::string::npos) {
			return false;
		}

		std::string ext = filepath.substr(dotPos);

		// Convert to lowercase for case-insensitive comparison
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		// Check if extension is supported
		return (ext == ".wav" || ext == ".mp3");
	}

	std::string AudioCatalog::GetInvalidFormatMessage(const std::string& filepath) {
		size_t dotPos = filepath.find_last_of('.');
		std::string ext = (dotPos != std::string::npos) ? filepath.substr(dotPos) : "unknown";

		return "Unsupported audio file type: \"" + ext + "\". Only .wav and .mp3 files are supported.";
	}
}
