/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			FileDropHandler.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		System for handling external file drops from Windows File Explorer.
					Integrates with the CoreEngine as a system to process dropped files
					(audio, textures, prefabs) and automatically import them into the project.
					
					Usage:
					1. Register as system: coreEngine->AddSystem(std::make_unique<FileDropHandler>(messageBus));
					2. In GLFW drop callback: dropHandler->HandleGLFWDrop(count, paths);
					3. Files are automatically copied to project directories and loaded

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <string>

#include "System.hpp"
#include "MessageBus.hpp"

/************************************************************************/
/*!
\class FileDropHandler
\brief
	CoreEngine system for handling external file drops from OS file explorer.
	
	This system processes files dragged from Windows File Explorer into the
	application window. It automatically:
	- Validates file types (.wav, .mp3, .png, .jpg, .json)
	- Copies files to appropriate project directories
	- Adds audio files to the AudioCatalog
	- Loads resources into the ResourceManager
	
	Supported file types:
	- Audio: .wav, .mp3 ? copied to ../../assets/Audio/
*/
/************************************************************************/
class FileDropHandler : public CoreFramework::SystemInterface {
public:
	/************************************************************************/
	/*!
	\brief
		Constructs the FileDropHandler system.
	\param bus
		Reference to the CoreEngine's MessageBus for future event notifications.
	*/
	/************************************************************************/
	explicit FileDropHandler(CoreFramework::MessageBus& bus);
	
	/************************************************************************/
	/*!
	\brief
		Default destructor.
	*/
	/************************************************************************/
	~FileDropHandler() = default;

	/************************************************************************/
	/*!
	\brief
		Initializes the file drop handler system.
		Called once by CoreEngine during startup.
	*/
	/************************************************************************/
	void Initialize() override;
	
	/************************************************************************/
	/*!
	\brief
		Updates the file drop handler system each frame.
		This system is event-driven, so Update does nothing.
	\param dt
		Delta time in seconds (unused).
	*/
	/************************************************************************/
	void Update(float dt) override;
	
	/************************************************************************/
	/*!
	\brief
		Returns the name of this system for debugging.
	\return
		"FileDropHandler"
	*/
	/************************************************************************/
	std::string GetName() override { return "FileDropHandler"; }

	/************************************************************************/
	/*!
	\brief
		Processes multiple files dropped from Windows File Explorer.
		Called by the GLFW drop callback in Main.cpp.
	\param count
		Number of files dropped.
	\param paths
		Array of C-string file paths from GLFW.
	
	Example:
		glfwSetDropCallback(window, [](GLFWwindow*, int count, const char** paths) {
			if (auto* handler = coreEngine->GetSystem<FileDropHandler>()) {
				handler->HandleGLFWDrop(count, paths);
			}
		});
	*/
	/************************************************************************/
	void HandleGLFWDrop(int count, const char** paths);
	
	/************************************************************************/
	/*!
	\brief
		Processes a single dropped file by routing to appropriate handler
		based on file extension.
	\param droppedPath
		Absolute file path from the operating system.
	\return
		True if the file was successfully imported, false otherwise.
	*/
	/************************************************************************/
	bool ProcessDroppedFile(const std::string& droppedPath);

private:
	//! Reference to the message bus for posting success/error events (future use)
	CoreFramework::MessageBus& messageBus;
	
	/************************************************************************/
	/*!
	\brief
		Processes audio files (.wav, .mp3).
		Copies to ../../assets/Audio/, adds to AudioCatalog, and loads into memory.
	\param droppedPath
		Absolute path to the audio file.
	\return
		True if successfully imported and added to catalog.
	*/
	/************************************************************************/
	bool ProcessAudioFile(const std::string& droppedPath);
	
	/************************************************************************/
	/*!
	\brief
		Processes texture files (.png, .jpg, .jpeg).
		Currently not implemented - returns false.
	\param droppedPath
		Absolute path to the texture file.
	\return
		False (not yet implemented).
	*/
	/************************************************************************/
	bool ProcessTextureFile(const std::string& droppedPath);
	
	/************************************************************************/
	/*!
	\brief
		Processes prefab files (.json).
		Currently not implemented - returns false.
	\param droppedPath
		Absolute path to the prefab file.
	\return
		False (not yet implemented).
	*/
	/************************************************************************/
	bool ProcessPrefabFile(const std::string& droppedPath);
	
	/************************************************************************/
	/*!
	\brief
		Extracts the file extension from a path (including the dot).
	\param path
		File path to extract extension from.
	\return
		File extension (e.g., ".wav", ".mp3"), or empty string if no extension.
	*/
	/************************************************************************/
	std::string GetFileExtension(const std::string& path) const;
};
