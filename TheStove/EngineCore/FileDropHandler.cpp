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

#include <algorithm>
#include <filesystem>

#include "EngineCore/AudioLoading.hpp"
#include "EngineCore/FileDropHandler.hpp"
#include "EngineCore/FilePaths.hpp"
#include "EngineCore/LevelEditorFileIO.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineGraphics/ResourceManager.hpp"

FileDropHandler::FileDropHandler(CoreFramework::MessageBus& bus)
	: messageBus(bus) {}


void FileDropHandler::Initialize() {
	TS_LOG_INFO("[FileDropHandler] System initialized and ready to receive file drops.");
}


void FileDropHandler::Update(float dt) {
	(void)dt; // Suppress unused parameter warning
}


void FileDropHandler::HandleGLFWDrop(int count, const char** paths) {
	// Validate input
	if (count <= 0 || !paths) {
		return;
	}

	TS_LOG_INFO("[FileDropHandler] Received " << count << " dropped file(s)");

	// Process each dropped file
	for (int i = 0; i < count; ++i) {
		std::string droppedPath(paths[i]);
		TS_LOG_INFO("[FileDropHandler] Processing file " << (i + 1) << ": " << droppedPath);

		ProcessDroppedFile(droppedPath);
	}
}


bool FileDropHandler::ProcessDroppedFile(const std::string& droppedPath) {
	// Extract file extension
	std::string ext = GetFileExtension(droppedPath);

	// Reject files without extensions
	if (ext.empty()) {
		TS_LOG_WARN("[FileDropHandler] Skipping file without extension: " << droppedPath);
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
		TS_LOG_WARN("[FileDropHandler] Unsupported file type: " << ext);
		return false;
	}
}


bool FileDropHandler::ProcessAudioFile(const std::string& droppedPath) {
	TS_LOG_INFO("[FileDropHandler] Processing audio file: " << droppedPath);

	// Target directory: editor-facing source audio directory.
	const std::string targetDir = FilePaths::Dirs::AUDIO_EDITOR;

	// Copy file to project using helper function (handles duplicate filenames)
	const std::string projPath = LEFILEIO::CopyFileIntoProjectUnique(droppedPath, targetDir);

	if (projPath.empty()) {
		TS_LOG_ERROR("[FileDropHandler] Failed to copy audio file!");
		return false;
	}

	TS_LOG_INFO("[FileDropHandler] Audio file copied to: " << projPath);

	// Normalize path for FMOD (convert backslashes to forward slashes)
	std::string normalizedPath = projPath;
	std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

	// Validate audio file format (.wav or .mp3)
	if (!Audio::AudioCatalog::IsValidAudioFile(normalizedPath)) {
		TS_LOG_ERROR("[FileDropHandler] Invalid audio file format!");
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

	TS_LOG_INFO("[FileDropHandler] Adding to catalog as: " << newAsset.name);

	// Set default audio properties
	newAsset.loop = false;          // Don't loop by default
	newAsset.stream = false;        // Load into memory (not streamed)
	newAsset.category = "sfx";      // Default to sound effect category
	newAsset.volume = 1.0f;         // Full volume

	// Add asset to AudioCatalog (checks for duplicate names)
	if (!Audio::AudioCatalog::AddAudioAsset(newAsset)) {
		TS_LOG_ERROR("[FileDropHandler] Failed to add to catalog (duplicate name?)");
		return false;
	}

	TS_LOG_INFO("[FileDropHandler] Successfully added to catalog");

	// Load the audio file into memory via ResourceManager
	ResourceManager::Instance().LoadAudio(
		newAsset.name,
		newAsset.filepath,
		newAsset.loop,
		newAsset.stream
	);

	// Auto-save catalog to SOURCE directory (../../assets from build/Release)
	const std::string catalogPath = FilePaths::Audio::CATALOG_EDITOR;
	if (Audio::AudioCatalog::SaveCatalogToFile(catalogPath)) {
		TS_LOG_INFO("[FileDropHandler] Catalog auto-saved to: " << catalogPath);
	}
	else {
		TS_LOG_WARN("[FileDropHandler] Failed to auto-save catalog!");
	}

	return true;
}


bool FileDropHandler::ProcessTextureFile(const std::string& droppedPath) {
	TS_LOG_INFO("[FileDropHandler] Processing texture file: " << droppedPath);

	const std::string projectPath = LEFILEIO::CopyFileIntoProjectUnique(droppedPath, FilePaths::Dirs::ASSETS_EDITOR);
	if (projectPath.empty()) {
		TS_LOG_ERROR("[FileDropHandler] Failed to copy texture file!");
		return false;
	}

	if (Texture* texture = LEFILEIO::LoadTextureBypassingCache(projectPath)) {
		TS_LOG_INFO("[FileDropHandler] Texture imported and loaded: " << projectPath);
		(void)texture;
		return true;
	}

	TS_LOG_WARN("[FileDropHandler] Texture copied but failed to load immediately: " << projectPath);
	return true;
}


bool FileDropHandler::ProcessPrefabFile(const std::string& droppedPath) {
	TS_LOG_INFO("[FileDropHandler] Processing prefab file: " << droppedPath);

	const std::string projectPath = LEFILEIO::CopyFileIntoProjectUnique(droppedPath, FilePaths::Dirs::PREFABS_EDITOR);
	if (projectPath.empty()) {
		TS_LOG_ERROR("[FileDropHandler] Failed to copy prefab file!");
		return false;
	}

	TS_LOG_INFO("[FileDropHandler] Prefab imported: " << projectPath);
	return true;
}


std::string FileDropHandler::GetFileExtension(const std::string& path) const {
	size_t dotPos = path.find_last_of('.');
	if (dotPos == std::string::npos) {
		return ""; // No extension found
	}
	return path.substr(dotPos); // Return extension including the dot
}
