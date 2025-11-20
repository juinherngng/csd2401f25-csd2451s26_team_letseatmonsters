/*
----------------------------------------------------------------------------------------------------
FILE NAME:			AudioLoading.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		Implementation of JSON-based audio catalog with serialization.

        All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "AudioLoading.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "JSONInclude.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>

using nlohmann::json;

namespace Audio
{
	// Static storage for audio assets
	std::vector<AudioAsset> AudioCatalog::s_AudioAssets;

	// Helper function to normalize paths (convert backslashes to forward slashes)
	static std::string NormalizeAudioPath(const std::string& path) {
		std::string normalized = path;
		std::replace(normalized.begin(), normalized.end(), '\\', '/');
		return normalized;
	}

	bool AudioCatalog::LoadCatalogFromFile(const std::string& catalogPath)
	{
		std::cout << "AudioCatalog: Loading catalog from " << catalogPath << "..." << std::endl;

		std::ifstream file(catalogPath);
		if (!file.is_open())
		{
			std::cerr << "AudioCatalog: Failed to open catalog file: " << catalogPath << std::endl;
			return false;
		}

		try
		{
			json catalogJson;
			file >> catalogJson;

			// Clear existing assets
			s_AudioAssets.clear();

			// Parse version (for future compatibility)
			std::string version = catalogJson.value("version", "1.0");
			std::cout << "  Catalog version: " << version << std::endl;

			// Parse audio assets
			if (catalogJson.contains("audio_assets") && catalogJson["audio_assets"].is_array())
			{
				for (const auto& assetJson : catalogJson["audio_assets"])
				{
					AudioAsset asset;
					asset.name = assetJson.value("name", "");
					
					// Normalize the filepath when loading from JSON
					std::string rawPath = assetJson.value("filepath", "");
					asset.filepath = NormalizeAudioPath(rawPath);
					
					asset.loop = assetJson.value("loop", false);
					asset.stream = assetJson.value("stream", false);
					asset.category = assetJson.value("category", "");
					asset.volume = assetJson.value("volume", 1.0f);

					// Validate the asset
					if (asset.name.empty() || asset.filepath.empty())
					{
						std::cerr << "  Warning: Skipping invalid audio asset entry" << std::endl;
						continue;
					}

					// Validate file format
					if (!IsValidAudioFile(asset.filepath))
					{
						std::cerr << "  Warning: Unsupported audio format for " << asset.filepath 
							      << " (only .wav and .mp3 are supported)" << std::endl;
						continue;
					}

					s_AudioAssets.push_back(asset);
					std::cout << "  Loaded asset: " << asset.name << " (path: " << asset.filepath << ")" << std::endl;
				}
			}

			std::cout << "AudioCatalog: Successfully loaded " << s_AudioAssets.size() << " audio assets." << std::endl;
			return true;
		}
		catch (const json::exception& e)
		{
			std::cerr << "AudioCatalog: JSON parsing error: " << e.what() << std::endl;
			return false;
		}
	}

	bool AudioCatalog::SaveCatalogToFile(const std::string& catalogPath)
	{
		std::cout << "AudioCatalog: Saving catalog to " << catalogPath << "..." << std::endl;

		try
		{
			json catalogJson;
			catalogJson["version"] = "1.0";
			catalogJson["audio_assets"] = json::array();

			// Serialize all audio assets
			for (const auto& asset : s_AudioAssets)
			{
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
			if (!file.is_open())
			{
				std::cerr << "AudioCatalog: Failed to open file for writing: " << catalogPath << std::endl;
				return false;
			}

			file << catalogJson.dump(2); // 2-space indentation
			file.close();

			std::cout << "AudioCatalog: Successfully saved " << s_AudioAssets.size() << " audio assets." << std::endl;
			return true;
		}
		catch (const json::exception& e)
		{
			std::cerr << "AudioCatalog: JSON serialization error: " << e.what() << std::endl;
			return false;
		}
	}

	void AudioCatalog::LoadAllAudio()
	{
		std::cout << "AudioCatalog: Loading all audio assets into memory..." << std::endl;

		auto& resMgr = ResourceManager::Instance();
		int successCount = 0;
		int failCount = 0;

		for (const auto& asset : s_AudioAssets)
		{
			std::cout << "  Loading: " << asset.name << " from " << asset.filepath << std::endl;
			std::cout << "    Properties: loop=" << (asset.loop ? "true" : "false")
				      << ", stream=" << (asset.stream ? "true" : "false")
				      << ", category=" << asset.category
				      << ", volume=" << asset.volume << std::endl;

			if (resMgr.LoadAudio(asset.name, asset.filepath, asset.loop, asset.stream))
			{
				// Get and display audio info
				unsigned int lenMs = 0;
				int channels = 0, bits = 0;
				float freq = 0.0f;

				if (resMgr.GetAudioInfo(asset.name, lenMs, channels, bits, freq))
				{
					std::cout << "    Info: " << lenMs << "ms, " << channels << " channels, "
						      << bits << " bits, " << freq << "Hz" << std::endl;
				}

				successCount++;
			}
			else
			{
				std::cerr << "  Failed to load: " << asset.name << std::endl;
				failCount++;
			}
		}

		std::cout << "AudioCatalog: Loaded " << successCount << " audio assets successfully";
		if (failCount > 0)
		{
			std::cout << " (" << failCount << " failed)";
		}
		std::cout << "." << std::endl;
	}

	void AudioCatalog::UnloadAllAudio()
	{
		std::cout << "AudioCatalog: Unloading all audio assets..." << std::endl;

		auto& resMgr = ResourceManager::Instance();

		for (const auto& asset : s_AudioAssets)
		{
			std::cout << "  Unloading: " << asset.name << std::endl;
			resMgr.UnloadAudio(asset.name);
		}

		std::cout << "AudioCatalog: All audio assets unloaded." << std::endl;
	}

	bool AudioCatalog::AddAudioAsset(const AudioAsset& asset)
	{
		// Normalize the filepath before adding
		AudioAsset normalizedAsset = asset;
		normalizedAsset.filepath = NormalizeAudioPath(asset.filepath);

		// Check if asset with same name already exists
		for (const auto& existing : s_AudioAssets)
		{
			if (existing.name == normalizedAsset.name)
			{
				std::cerr << "AudioCatalog: Asset with name '" << normalizedAsset.name << "' already exists!" << std::endl;
				return false;
			}
		}

		// Validate file format
		if (!IsValidAudioFile(normalizedAsset.filepath))
		{
			std::cerr << "AudioCatalog: " << GetInvalidFormatMessage(normalizedAsset.filepath) << std::endl;
			return false;
		}

		s_AudioAssets.push_back(normalizedAsset);
		std::cout << "AudioCatalog: Added asset: " << normalizedAsset.name << " (path: " << normalizedAsset.filepath << ")" << std::endl;
		return true;
	}

	bool AudioCatalog::RemoveAudioAsset(const std::string& name)
	{
		auto it = std::find_if(s_AudioAssets.begin(), s_AudioAssets.end(),
			[&name](const AudioAsset& asset) { return asset.name == name; });

		if (it != s_AudioAssets.end())
		{
			s_AudioAssets.erase(it);
			std::cout << "AudioCatalog: Removed asset: " << name << std::endl;

			// Also unload from audio system
			ResourceManager::Instance().UnloadAudio(name);
			return true;
		}

		std::cerr << "AudioCatalog: Asset '" << name << "' not found!" << std::endl;
		return false;
	}

	const AudioAsset* AudioCatalog::GetAudioAsset(const std::string& name)
	{
		auto it = std::find_if(s_AudioAssets.begin(), s_AudioAssets.end(),
			[&name](const AudioAsset& asset) { return asset.name == name; });

		if (it != s_AudioAssets.end())
		{
			return &(*it);
		}

		return nullptr;
	}

	const std::vector<AudioAsset>& AudioCatalog::GetAllAssets()
	{
		return s_AudioAssets;
	}

	bool AudioCatalog::IsValidAudioFile(const std::string& filepath)
	{
		if (filepath.empty())
		{
			return false;
		}

		// Extract extension
		size_t dotPos = filepath.find_last_of('.');
		if (dotPos == std::string::npos)
		{
			return false;
		}

		std::string ext = filepath.substr(dotPos);
		
		// Convert to lowercase for case-insensitive comparison
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		// Check if extension is supported
		return (ext == ".wav" || ext == ".mp3");
	}

	std::string AudioCatalog::GetInvalidFormatMessage(const std::string& filepath)
	{
		size_t dotPos = filepath.find_last_of('.');
		std::string ext = (dotPos != std::string::npos) ? filepath.substr(dotPos) : "unknown";

		return "Unsupported audio file type: \"" + ext + "\". Only .wav and .mp3 files are supported.";
	}
}
