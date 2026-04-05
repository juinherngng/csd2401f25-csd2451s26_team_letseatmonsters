/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			FileDropHandler.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (80%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (20%)

 DESCRIPTION:		System for handling external file drops from Windows File Explorer.
					Integrates with the CoreEngine as a system to process dropped files
					(audio, textures, prefabs) and automatically import them into the project.

					Usage:
					1. Register as system: coreEngine->AddSystem(std::make_unique<FileDropHandler>(messageBus));
					2. In GLFW drop callback: dropHandler->HandleGLFWDrop(count, paths);
					3. Files are automatically copied to project directories and loaded

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <string>

#include "EngineCore/MessageBus.hpp"
#include "EngineCore/System.hpp"

/**
 * @brief CoreEngine system for handling external file drops from the OS file explorer.
 */
class FileDropHandler : public CoreFramework::SystemInterface {
public:
	/**
	 * @brief Constructs the file-drop handler system.
	 * @param bus Reference to the CoreEngine message bus.
	 */
	explicit FileDropHandler(CoreFramework::MessageBus& bus);

	/**
	 * @brief Destroys the file-drop handler.
	 */
	~FileDropHandler() = default;

	/**
	 * @brief Initializes the file-drop handler system.
	 */
	void Initialize() override;

	/**
	 * @brief Updates the file-drop handler system.
	 * @param dt Delta time in seconds.
	 */
	void Update(float dt) override;

	/**
	 * @brief Returns the debug-facing name of this system.
	 * @return The string `"FileDropHandler"`.
	 */
	std::string GetName() override {
		// Keep the system name stable for logs and runtime inspection tools.
		return "FileDropHandler";
	}

	/**
	 * @brief Processes multiple files dropped from the OS file explorer.
	 * @param count Number of dropped files.
	 * @param paths Array of dropped file paths provided by GLFW.
	 */
	void HandleGLFWDrop(int count, const char** paths);

	/**
	 * @brief Routes a dropped file to the correct importer based on its extension.
	 * @param droppedPath Absolute file path provided by the operating system.
	 * @return True if the file was successfully imported, otherwise false.
	 */
	bool ProcessDroppedFile(const std::string& droppedPath);

private:
	//! Reference to the message bus for posting success/error events (future use)
	CoreFramework::MessageBus& messageBus;

	/**
	 * @brief Imports a dropped audio file into the project.
	 * @param droppedPath Absolute path to the audio file.
	 * @return True if the audio was imported successfully.
	 */
	bool ProcessAudioFile(const std::string& droppedPath);

	/**
	 * @brief Imports a dropped texture file into the project.
	 * @param droppedPath Absolute path to the texture file.
	 * @return True if the texture was imported successfully.
	 */
	bool ProcessTextureFile(const std::string& droppedPath);

	/**
	 * @brief Imports a dropped prefab JSON file into the project.
	 * @param droppedPath Absolute path to the prefab file.
	 * @return True if the prefab was imported successfully.
	 */
	bool ProcessPrefabFile(const std::string& droppedPath);

	/**
	 * @brief Extracts the file extension from a path.
	 * @param path File path to inspect.
	 * @return File extension including the dot, or an empty string if none exists.
	 */
	std::string GetFileExtension(const std::string& path) const;
};
