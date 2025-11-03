/*
----------------------------------------------------------------------------------------------------
FILE NAME:			AudioLoading.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		Implementation of centralized audio catalog for loading all game audio assets.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "AudioLoading.hpp"
#include "../Graphics/ResourceManager.hpp"
#include <iostream>

namespace Audio
{
	void AudioCatalog::LoadAllAudio()
	{
		std::cout << "AudioCatalog: Loading all audio assets..." << std::endl;
		
		LoadUISounds();
		LoadGameSounds();
		LoadMusic();
		
		std::cout << "AudioCatalog: All audio assets loaded successfully." << std::endl;
	}

	void AudioCatalog::UnloadAllAudio()
	{
		std::cout << "AudioCatalog: Unloading all audio assets..." << std::endl;
		
		auto& resMgr = ResourceManager::Instance();
		
		// Unload UI sounds
		resMgr.UnloadAudio("ui_click");

		// TODO: Uncomment when files are added
		//resMgr.UnloadAudio("ui_hover");
		//resMgr.UnloadAudio("ui_confirm");
		//resMgr.UnloadAudio("ui_cancel");
		//resMgr.UnloadAudio("ui_error");
		
		// Unload game sounds
		resMgr.UnloadAudio("boiling_sound");
		resMgr.UnloadAudio("grilling_sound");

		// TODO: Uncomment when files are added
		//resMgr.UnloadAudio("cooking_sizzle");
		//resMgr.UnloadAudio("chop_sound");
		
		// Unload music
		resMgr.UnloadAudio("bgm_menu");

		// TODO: Uncomment when files are added
		//resMgr.UnloadAudio("bgm_gameplay");
		//resMgr.UnloadAudio("bgm_credits");
		
		std::cout << "AudioCatalog: All audio assets unloaded." << std::endl;
	}

	void AudioCatalog::LoadUISounds()
	{
		std::cout << "AudioCatalog: Loading UI sounds..." << std::endl;
		
		auto& resMgr = ResourceManager::Instance();
		
		// UI Click Sound
		// Properties: Not looping, not streamed (short sound effect loaded into memory)
		if (resMgr.LoadAudio("ui_click", "../assets/Audio/Cute Pop Sound Effects1 0.1.wav", false, false))
		{
			std::cout << "  - Loaded: ui_click" << std::endl;
		}
		else
		{
			std::cerr << "  - Failed to load: ui_click" << std::endl;
		}
		
		// TODO: Add more UI sounds here as needed
		
		std::cout << "AudioCatalog: UI sounds loaded." << std::endl;
	}

	void AudioCatalog::LoadGameSounds()
	{
		std::cout << "AudioCatalog: Loading game sounds..." << std::endl;
		
		auto& resMgr = ResourceManager::Instance();
		
		// Boiling Sound
		// Properties: Looping, not streamed (loaded into memory for quick playback)
		if (resMgr.LoadAudio("boiling_sound", "../assets/Audio/Boiling7.wav", true, false))
		{
			std::cout << "  - Loaded: boiling_sound" << std::endl;
			
			// Get and display audio info
			unsigned int lenMs = 0;
			int ch = 0, bits = 0;
			float freq = 0;
			if (resMgr.GetAudioInfo("boiling_sound", lenMs, ch, bits, freq))
			{
				std::cout << "    Length: " << lenMs << "ms, Channels: " << ch 
						  << ", Bits: " << bits << ", Freq: " << freq << "Hz" << std::endl;
			}
		}
		else
		{
			std::cerr << "  - Failed to load: boiling_sound" << std::endl;
		}

		// Grilling Sound
		// Properties: Looping, not streamed (loaded into memory for quick playback)
		if (resMgr.LoadAudio("grilling_sound", "../assets/Audio/Grill SFX5.wav", true, false))
		{
			std::cout << "  - Loaded: grilling_sound" << std::endl;

			// Get and display audio info
			unsigned int lenMs = 0;
			int ch = 0, bits = 0;
			float freq = 0;
			if (resMgr.GetAudioInfo("grilling_sound", lenMs, ch, bits, freq))
			{
				std::cout << "    Length: " << lenMs << "ms, Channels: " << ch
					<< ", Bits: " << bits << ", Freq: " << freq << "Hz" << std::endl;
			}
		}
		else
		{
			std::cerr << "  - Failed to load: grilling_sound" << std::endl;
		}
		
		// TODO: Add more game sounds here as needed
		
		std::cout << "AudioCatalog: Game sounds loaded." << std::endl;
	}

	void AudioCatalog::LoadMusic()
	{
		std::cout << "AudioCatalog: Loading music tracks..." << std::endl;
		
		auto& resMgr = ResourceManager::Instance();
		
		// Menu Background Music
		// Properties: Looping, streamed (saves memory for long music tracks)
		if (resMgr.LoadAudio("background_music", "../assets/Audio/bgm.wav", true, true))
		{
			std::cout << "  - Loaded: background_music" << std::endl;
			
			// Get and display audio info
			unsigned int lenMs = 0;
			int ch = 0, bits = 0;
			float freq = 0;
			if (resMgr.GetAudioInfo("background_music", lenMs, ch, bits, freq))
			{
				std::cout << "    Length: " << lenMs << "ms, Channels: " << ch 
						  << ", Bits: " << bits << ", Freq: " << freq << "Hz" << std::endl;
			}
		}
		else
		{
			std::cerr << "  - Failed to load: background_music" << std::endl;
		}

		// TODO: Add more music tracks here as needed
		
		std::cout << "AudioCatalog: Music tracks loaded." << std::endl;
	}
}
