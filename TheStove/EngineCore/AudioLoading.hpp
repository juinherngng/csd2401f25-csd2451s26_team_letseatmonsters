/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			AudioLoading.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (85%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu		(15%)

 DESCRIPTION:		Audio catalog with JSON-based serialization for dynamic audio loading.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <string>
#include <vector>

namespace Audio {
	/**
	 * @brief Stores the authored metadata for a single audio asset entry.
	 */
	struct AudioAsset {
		std::string name;       // Logical name for referencing the audio
		std::string filepath;   // Path to the audio file
		bool loop;              // Whether the sound should loop
		bool stream;            // Whether to stream (true) or load into memory (false)
		std::string category;   // Category: "ui", "sfx", "bgm", etc.
		float volume;           // Default volume (0.0 to 1.0)

		/**
		 * @brief Constructs an empty audio asset with safe defaults.
		 */
		AudioAsset()
			: name(""), filepath(""), loop(false), stream(false), category(""), volume(1.0f) {
			// Start with blank metadata so editor code can fill fields incrementally.
		}
	};

	/**
	 * @brief Static utility for loading, storing, and validating authored audio catalog data.
	 */
	class AudioCatalog {
	public:
		/**
		 * @brief Loads the audio catalog from a JSON file.
		 * @param catalogPath Path to the JSON catalog file.
		 * @return True if loaded successfully, otherwise false.
		 */
		static bool LoadCatalogFromFile(const std::string& catalogPath = "../assets/Audio/AudioCatalog.json");

		/**
		 * @brief Saves the current audio catalog to a JSON file.
		 * @param catalogPath Path to the output JSON catalog file.
		 * @return True if saved successfully, otherwise false.
		 */
		static bool SaveCatalogToFile(const std::string& catalogPath = "../assets/Audio/AudioCatalog.json");

		/**
		 * @brief Loads every audio asset currently stored in the catalog into the resource manager.
		 */
		static void LoadAllAudio();

		/**
		 * @brief Unloads every audio asset currently tracked by the catalog.
		 */
		static void UnloadAllAudio();

		/**
		 * @brief Adds a new audio asset to the in-memory catalog.
		 * @param asset The audio asset metadata to add.
		 * @return True when the asset was added successfully, otherwise false.
		 */
		static bool AddAudioAsset(const AudioAsset& asset);

		/**
		 * @brief Removes an audio asset from the catalog by name.
		 * @param name Logical name of the audio asset to remove.
		 * @return True when the asset was removed, otherwise false.
		 */
		static bool RemoveAudioAsset(const std::string& name);

		/**
		 * @brief Replaces an existing audio asset with updated metadata.
		 * @param originalName Current logical name of the audio asset being edited.
		 * @param updatedAsset Replacement metadata to store.
		 * @return True when the asset was replaced successfully, otherwise false.
		 */
		static bool ReplaceAudioAsset(const std::string& originalName, const AudioAsset& updatedAsset);

		/**
		 * @brief Finds an audio asset in the catalog by name.
		 * @param name Logical name of the audio asset to retrieve.
		 * @return Pointer to the audio asset, or nullptr if not found.
		 */
		static const AudioAsset* GetAudioAsset(const std::string& name);

		/**
		 * @brief Returns all audio assets currently stored in the catalog.
		 * @return Immutable reference to the catalog asset list.
		 */
		static const std::vector<AudioAsset>& GetAllAssets();

		/**
		 * @brief Checks whether an audio file path uses a supported extension.
		 * @param filepath File path to validate.
		 * @return True for supported audio formats, otherwise false.
		 */
		static bool IsValidAudioFile(const std::string& filepath);

		/**
		 * @brief Builds a user-facing error string for an unsupported audio path.
		 * @param filepath File path that failed validation.
		 * @return Human-readable validation error string.
		 */
		static std::string GetInvalidFormatMessage(const std::string& filepath);

	private:
		// Prevent instantiation - this is a static utility class
		AudioCatalog() = delete;
		~AudioCatalog() = delete;
		AudioCatalog(const AudioCatalog&) = delete;
		AudioCatalog& operator=(const AudioCatalog&) = delete;

		// Internal storage for the audio catalog
		static std::vector<AudioAsset> s_AudioAssets;
	};
}
