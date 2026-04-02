/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			AudioLoading.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		Audio catalog with JSON-based serialization for dynamic audio loading.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <string>
#include <vector>

namespace Audio {
	/************************************************************************/
	/*!
	\struct AudioAsset
	\brief
		Represents a single audio asset with all its properties.
	*/
	/************************************************************************/
	struct AudioAsset {
		std::string name;       // Logical name for referencing the audio
		std::string filepath;   // Path to the audio file
		bool loop;              // Whether the sound should loop
		bool stream;            // Whether to stream (true) or load into memory (false)
		std::string category;   // Category: "ui", "sfx", "bgm", etc.
		float volume;           // Default volume (0.0 to 1.0)

		AudioAsset()
			: name(""), filepath(""), loop(false), stream(false), category(""), volume(1.0f) {
		}
	};

	/************************************************************************/
	/*!
	\class AudioCatalog
	\brief
		Centralized manager for loading and managing audio assets via JSON.
		Provides serialization/deserialization and dynamic loading without C++ code changes.
	*/
	/************************************************************************/
	class AudioCatalog {
	public:
		/************************************************************************/
		/*!
		\brief
			Loads the audio catalog from a JSON file.
		\param catalogPath
			Path to the JSON catalog file.
		\return
			True if loaded successfully, false otherwise.
		*/
		/************************************************************************/
		static bool LoadCatalogFromFile(const std::string& catalogPath = "../assets/Audio/AudioCatalog.json");

		/************************************************************************/
		/*!
		\brief
			Saves the current audio catalog to a JSON file.
		\param catalogPath
			Path to save the JSON catalog file.
		\return
			True if saved successfully, false otherwise.
		*/
		/************************************************************************/
		static bool SaveCatalogToFile(const std::string& catalogPath = "../assets/Audio/AudioCatalog.json");

		/************************************************************************/
		/*!
		\brief
			Loads all audio assets from the currently loaded catalog.
		\details
			Should be called after LoadCatalogFromFile() during application initialization.
		*/
		/************************************************************************/
		static void LoadAllAudio();

		/************************************************************************/
		/*!
		\brief
			Unloads all audio assets.
		\details
			Should be called during application shutdown to clean up audio resources.
		*/
		/************************************************************************/
		static void UnloadAllAudio();

		/************************************************************************/
		/*!
		\brief
			Adds a new audio asset to the catalog.
		\param asset
			The audio asset to add.
		\return
			True if added successfully, false if an asset with the same name already exists.
		*/
		/************************************************************************/
		static bool AddAudioAsset(const AudioAsset& asset);

		/************************************************************************/
		/*!
		\brief
			Removes an audio asset from the catalog by name.
		\param name
			The name of the audio asset to remove.
		\return
			True if removed successfully, false if not found.
		*/
		/************************************************************************/
		static bool RemoveAudioAsset(const std::string& name);

		/************************************************************************/
		/*!
		\brief
			Replaces an existing audio asset with updated metadata atomically.
		\param originalName
			The current name of the audio asset being edited.
		\param updatedAsset
			The replacement metadata to store.
		\return
			True if the asset was replaced successfully, false otherwise.
		*/
		/************************************************************************/
		static bool ReplaceAudioAsset(const std::string& originalName, const AudioAsset& updatedAsset);

		/************************************************************************/
		/*!
		\brief
			Gets an audio asset from the catalog by name.
		\param name
			The name of the audio asset to retrieve.
		\return
			Pointer to the audio asset, or nullptr if not found.
		*/
		/************************************************************************/
		static const AudioAsset* GetAudioAsset(const std::string& name);

		/************************************************************************/
		/*!
		\brief
			Gets all audio assets in the catalog.
		\return
			Vector of all audio assets.
		*/
		/************************************************************************/
		static const std::vector<AudioAsset>& GetAllAssets();

		/************************************************************************/
		/*!
		\brief
			Validates an audio file path to ensure it's a supported format.
		\param filepath
			The file path to validate.
		\return
			True if the file format is supported (.wav or .mp3), false otherwise.
		*/
		/************************************************************************/
		static bool IsValidAudioFile(const std::string& filepath);

		/************************************************************************/
		/*!
		\brief
			Gets a user-friendly error message for unsupported audio formats.
		\param filepath
			The file path that was invalid.
		\return
			Error message string.
		*/
		/************************************************************************/
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
