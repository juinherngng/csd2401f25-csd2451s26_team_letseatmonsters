/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			FileDropHandler.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		Implementation of the FileDropHandler system.
					Handles external file drops from Windows File Explorer, validates file types,
					copies files to appropriate project directories, and integrates with the
					audio catalog and resource manager.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "../Graphics/ResourceManager.hpp"

#include "AudioLoading.hpp"
#include "FileDropHandler.hpp"
#include "LevelEditorFileIO.hpp"

#include <algorithm>
#include <iostream>


FileDropHandler::FileDropHandler(CoreFramework::MessageBus& bus)
	: messageBus(bus) {
}


void FileDropHandler::Initialize() {
	std::cout << "[FileDropHandler] System initialized and ready to receive file drops." << std::endl;
}


void FileDropHandler::Update(float dt) {
	(void)dt; // Suppress unused parameter warning
}


void FileDropHandler::HandleGLFWDrop(int count, const char** paths) {
	// Validate input
	if (count <= 0 || !paths) {
		return;
	}

	std::cout << "[FileDropHandler] Received " << count << " dropped file(s)" << std::endl;

	// Process each dropped file
	for (int i = 0; i < count; ++i) {
		std::string droppedPath(paths[i]);
		std::cout << "[FileDropHandler] Processing file " << (i + 1) << ": " << droppedPath << std::endl;

		ProcessDroppedFile(droppedPath);
	}
}


bool FileDropHandler::ProcessDroppedFile(const std::string& droppedPath) {
	// Extract file extension
	std::string ext = GetFileExtension(droppedPath);

	// Reject files without extensions
	if (ext.empty()) {
		std::cerr << "[FileDropHandler] Skipping file without extension: " << droppedPath << std::endl;
		return false;
	}

	// Convert extension to lowercase for case-insensitive comparison
	std::transform(ext.begin(), ext.end(), ext.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	// Route to appropriate handler based on file type
	if (ext == ".wav" || ext == ".mp3") {
		return ProcessAudioFile(droppedPath);
	}
	else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
		return ProcessTextureFile(droppedPath);
	}
	else if (ext == ".json") {
		return ProcessPrefabFile(droppedPath);
	}
	else {
		// Unsupported file type
		std::cout << "[FileDropHandler] Unsupported file type: " << ext << std::endl;
		return false;
	}
}


bool FileDropHandler::ProcessAudioFile(const std::string& droppedPath) {
	std::cout << "[FileDropHandler] Processing audio file: " << droppedPath << std::endl;

	// Target directory: ../../assets/Audio (relative to build/Release)
	// This goes from build/Release ? project root ? assets/Audio (SOURCE directory)
	const std::string targetDir = "../../assets/Audio";

	// Copy file to project using helper function (handles duplicate filenames)
	const std::string projPath = LEFILEIO::CopyFileIntoProjectUnique(droppedPath, targetDir);

	if (projPath.empty()) {
		std::cerr << "[FileDropHandler] ERROR: Failed to copy audio file!" << std::endl;
		return false;
	}

	std::cout << "[FileDropHandler] Audio file copied to: " << projPath << std::endl;

	// Normalize path for FMOD (convert backslashes to forward slashes)
	std::string normalizedPath = projPath;
	std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

	// Validate audio file format (.wav or .mp3)
	if (!Audio::AudioCatalog::IsValidAudioFile(normalizedPath)) {
		std::cerr << "[FileDropHandler] ERROR: Invalid audio file format!" << std::endl;
		return false;
	}

	// Create a new audio asset entry
	Audio::AudioAsset newAsset;
	newAsset.filepath = normalizedPath;

	// Extract filename (without extension) to use as asset name
	size_t lastSlash = normalizedPath.find_last_of("/\\");
	size_t lastDot = normalizedPath.find_last_of('.');

	if (lastSlash != std::string::npos && lastDot != std::string::npos) {
		// Use filename without extension as asset name
		newAsset.name = normalizedPath.substr(lastSlash + 1, lastDot - lastSlash - 1);
	}
	else {
		// Fallback: generate unique name based on catalog size
		const auto& assets = Audio::AudioCatalog::GetAllAssets();
		newAsset.name = "audio_" + std::to_string(assets.size());
	}

	std::cout << "[FileDropHandler] Adding to catalog as: " << newAsset.name << std::endl;

	// Set default audio properties
	newAsset.loop = false;          // Don't loop by default
	newAsset.stream = false;        // Load into memory (not streamed)
	newAsset.category = "sfx";      // Default to sound effect category
	newAsset.volume = 1.0f;         // Full volume

	// Add asset to AudioCatalog (checks for duplicate names)
	if (!Audio::AudioCatalog::AddAudioAsset(newAsset)) {
		std::cerr << "[FileDropHandler] ERROR: Failed to add to catalog (duplicate name?)" << std::endl;
		return false;
	}

	std::cout << "[FileDropHandler] Successfully added to catalog" << std::endl;

	// Load the audio file into memory via ResourceManager
	ResourceManager::Instance().LoadAudio(
		newAsset.name,
		newAsset.filepath,
		newAsset.loop,
		newAsset.stream
	);

	// Auto-save catalog to SOURCE directory (../../assets from build/Release)
	const std::string catalogPath = "../../assets/Audio/AudioCatalog.json";
	if (Audio::AudioCatalog::SaveCatalogToFile(catalogPath)) {
		std::cout << "[FileDropHandler] Catalog auto-saved to: " << catalogPath << std::endl;
	}
	else {
		std::cerr << "[FileDropHandler] WARNING: Failed to auto-save catalog!" << std::endl;
	}

	return true;
}


bool FileDropHandler::ProcessTextureFile(const std::string& droppedPath) {
	std::cout << "[FileDropHandler] Texture import not yet implemented: " << droppedPath << std::endl;
	return false;
}


bool FileDropHandler::ProcessPrefabFile(const std::string& droppedPath) {
	std::cout << "[FileDropHandler] Prefab import not yet implemented: " << droppedPath << std::endl;
	return false;
}


std::string FileDropHandler::GetFileExtension(const std::string& path) const {
	size_t dotPos = path.find_last_of('.');
	if (dotPos == std::string::npos) {
		return ""; // No extension found
	}
	return path.substr(dotPos); // Return extension including the dot
}

