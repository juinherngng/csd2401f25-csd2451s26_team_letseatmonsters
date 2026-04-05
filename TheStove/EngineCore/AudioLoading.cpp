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

	/**
	 * @brief Normalizes an authored audio path to forward-slash form.
	 * @param path Raw path to normalize.
	 * @return Path with Windows-style separators converted to forward slashes.
	 */
	static std::string NormalizeAudioPath(const std::string& path) {
		std::string normalized = path;
		// Store paths in a consistent form so editor and runtime comparisons behave predictably.
		std::replace(normalized.begin(), normalized.end(), '\\', '/');
		return normalized;
	}

	/**
	 * @brief Reduces a full audio path to just its file name for logging.
	 * @param path Full or relative path to an audio file.
	 * @return File name portion of the path.
	 */
	static std::string CompactAudioFileLabel(const std::string& path) {
		if (path.empty()) {
			return path;
		}

		// Show only the filename in logs so long asset paths do not overwhelm the console.
		return std::filesystem::path(path).filename().string();
	}

	/**
	 * @brief Converts editor-authored asset paths into runtime paths for release builds.
	 * @param path Original path stored in the catalog.
	 * @return Path adjusted for the runtime working directory.
	 */
	static std::string ConvertToRuntimePath(const std::string& path) {
		std::string result = path;

		// Convert ../../assets/ to ../assets/ for release builds.
		const std::string editorPrefix = "../../assets/";
		const std::string runtimePrefix = "../assets/";

		if (result.find(editorPrefix) == 0) {
			result = runtimePrefix + result.substr(editorPrefix.length());
		}

		return result;
	}

	/**
	 * @brief Loads the audio catalog JSON from disk into memory.
	 * @param catalogPath Path to the catalog JSON file.
	 * @return True if the catalog was parsed successfully, otherwise false.
	 */
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

			// Replace any previous catalog contents with the freshly parsed data.
			s_AudioAssets.clear();

			// Parse version (for future compatibility)
			std::string version = catalogJson.value("version", "1.0");
			TS_LOG_DEBUG("Catalog version: " << version);

			// Deserialize each authored audio asset entry into the in-memory catalog.
			if (catalogJson.contains("audio_assets") && catalogJson["audio_assets"].is_array()) {
				for (const auto& assetJson : catalogJson["audio_assets"]) {
					AudioAsset asset;
					asset.name = assetJson.value("name", "");

					// Normalize the path immediately so later save/load passes stay consistent.
					std::string rawPath = assetJson.value("filepath", "");
					asset.filepath = NormalizeAudioPath(rawPath);

					// Release builds run from a different working directory, so patch the stored paths on load.
					if (convertPaths) {
						asset.filepath = ConvertToRuntimePath(asset.filepath);
					}

					asset.loop = assetJson.value("loop", false);
					asset.stream = assetJson.value("stream", false);
					asset.category = assetJson.value("category", "");
					asset.volume = assetJson.value("volume", 1.0f);

					// Skip malformed entries rather than aborting the whole catalog load.
					if (asset.name.empty() || asset.filepath.empty()) {
						TS_LOG_WARN("AudioCatalog: Skipping invalid audio asset entry");
						continue;
					}

					// Only allow file types currently supported by the audio pipeline.
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

	/**
	 * @brief Saves the current in-memory audio catalog back to JSON.
	 * @param catalogPath Path to the output catalog file.
	 * @return True if the file was written successfully, otherwise false.
	 */
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

			// Serialize every tracked asset so the JSON stays in sync with the in-memory editor state.
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

			// Pretty-print the JSON so the catalog remains readable in source control.
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

				// Read back a few lines for debugging so save failures are easier to diagnose.
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

	/**
	 * @brief Loads every audio asset currently stored in the catalog.
	 */
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

			// Load SFX/VFX sounds as 3D for spatial playback, while BGM/UI remain 2D.
			bool isSpatial = (asset.category == "sfx" || asset.category == "vfx");
			bool loaded = isSpatial
				? resMgr.LoadAudio3D(asset.name, asset.filepath, asset.loop, asset.stream)
				: resMgr.LoadAudio(asset.name, asset.filepath, asset.loop, asset.stream);

			if (loaded) {
				// Log decoded audio metadata so asset issues are easier to track down.
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

	/**
	 * @brief Unloads every audio asset currently tracked by the catalog.
	 */
	void AudioCatalog::UnloadAllAudio() {
		TS_LOG_INFO("AudioCatalog: Unloading all audio assets...");

		auto& resMgr = ResourceManager::Instance();

		for (const auto& asset : s_AudioAssets) {
			TS_LOG_DEBUG("Unloading: " << asset.name);
			resMgr.UnloadAudio(asset.name);
		}

		TS_LOG_INFO("AudioCatalog: All audio assets unloaded.");
	}

	/**
	 * @brief Adds a new audio asset entry to the in-memory catalog.
	 * @param asset Audio asset metadata to add.
	 * @return True if the asset was added, otherwise false.
	 */
	bool AudioCatalog::AddAudioAsset(const AudioAsset& asset) {
		// Normalize the filepath before storing it so editor and runtime code see the same format.
		AudioAsset normalizedAsset = asset;
		normalizedAsset.filepath = NormalizeAudioPath(asset.filepath);

		// Asset names are treated as unique logical identifiers by the audio system.
		for (const auto& existing : s_AudioAssets) {
			if (existing.name == normalizedAsset.name) {
				TS_LOG_ERROR("AudioCatalog: Asset with name '" << normalizedAsset.name << "' already exists");
				return false;
			}
		}

		// Reject unsupported formats before the catalog is mutated.
		if (!IsValidAudioFile(normalizedAsset.filepath)) {
			TS_LOG_ERROR("AudioCatalog: " << GetInvalidFormatMessage(normalizedAsset.filepath));
			return false;
		}

		s_AudioAssets.push_back(normalizedAsset);
		TS_LOG_INFO("AudioCatalog: Added asset: " << normalizedAsset.name << " (path: " << normalizedAsset.filepath << ")");
		return true;
	}

	/**
	 * @brief Removes an audio asset entry from the catalog and unloads it if necessary.
	 * @param name Logical name of the audio asset to remove.
	 * @return True if the asset was removed, otherwise false.
	 */
	bool AudioCatalog::RemoveAudioAsset(const std::string& name) {
		auto it = std::find_if(s_AudioAssets.begin(), s_AudioAssets.end(),
			[&name](const AudioAsset& asset) { return asset.name == name; });

		if (it != s_AudioAssets.end()) {
			s_AudioAssets.erase(it);
			TS_LOG_INFO("AudioCatalog: Removed asset: " << name);

			// Also unload from the runtime audio system so stale resources do not linger.
			ResourceManager::Instance().UnloadAudio(name);
			return true;
		}

		TS_LOG_WARN("AudioCatalog: Asset '" << name << "' not found");
		return false;
	}

	/**
	 * @brief Replaces an existing catalog entry with updated metadata.
	 * @param originalName Current name of the asset being replaced.
	 * @param updatedAsset Replacement asset metadata.
	 * @return True if the asset was replaced successfully, otherwise false.
	 */
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

	/**
	 * @brief Finds an audio asset entry by logical name.
	 * @param name Logical asset name to search for.
	 * @return Pointer to the matching asset, or nullptr if absent.
	 */
	const AudioAsset* AudioCatalog::GetAudioAsset(const std::string& name) {
		auto it = std::find_if(s_AudioAssets.begin(), s_AudioAssets.end(),
			[&name](const AudioAsset& asset) { return asset.name == name; });

		if (it != s_AudioAssets.end()) {
			// Return a stable pointer into the catalog vector when the asset is present.
			return &(*it);
		}

		return nullptr;
	}

	/**
	 * @brief Returns the full in-memory audio catalog.
	 * @return Immutable reference to the catalog asset list.
	 */
	const std::vector<AudioAsset>& AudioCatalog::GetAllAssets() {
		// Expose the live catalog so editor tools can inspect the current audio entries.
		return s_AudioAssets;
	}

	/**
	 * @brief Checks whether a file path uses a supported audio extension.
	 * @param filepath File path to validate.
	 * @return True for supported formats, otherwise false.
	 */
	bool AudioCatalog::IsValidAudioFile(const std::string& filepath) {
		if (filepath.empty()) {
			return false;
		}

		// Extract the extension first so validation works for arbitrary directory names.
		size_t dotPos = filepath.find_last_of('.');
		if (dotPos == std::string::npos) {
			return false;
		}

		std::string ext = filepath.substr(dotPos);

		// Normalize casing so .WAV and .wav are treated the same.
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		// Only the formats currently supported by FMOD in this pipeline are allowed here.
		return (ext == ".wav" || ext == ".mp3");
	}

	/**
	 * @brief Builds a readable validation error for an unsupported audio file path.
	 * @param filepath File path that failed validation.
	 * @return Human-readable error message.
	 */
	std::string AudioCatalog::GetInvalidFormatMessage(const std::string& filepath) {
		size_t dotPos = filepath.find_last_of('.');
		std::string ext = (dotPos != std::string::npos) ? filepath.substr(dotPos) : "unknown";

		// Format the message once here so editor/UI code can reuse the same validation wording.
		return "Unsupported audio file type: \"" + ext + "\". Only .wav and .mp3 files are supported.";
	}
}
