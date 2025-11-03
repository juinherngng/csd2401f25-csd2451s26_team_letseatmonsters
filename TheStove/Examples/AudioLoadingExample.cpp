/*
----------------------------------------------------------------------------------------------------
FILE NAME:			AudioLoadingExample.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				GitHub Copilot

DESCRIPTION:		Example code demonstrating how to load audio using ResourceManager.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

// Example usage of ResourceManager for audio loading
// This file is for reference only and should not be compiled into the project

#include "../Graphics/ResourceManager.hpp"

void AudioLoadingExample()
{
    // Get the ResourceManager instance
    auto& resMgr = ResourceManager::Instance();
    
    // Load audio files through ResourceManager
    // The ResourceManager delegates to AudioManager internally
    
    // Load a looping background music (streamed from disk)
    resMgr.LoadAudio("background_music", "../assets/Audio/bgm.wav", true, true);
    
    // Load a sound effect (loaded into memory, not looping)
    resMgr.LoadAudio("boiling_sound", "../assets/Audio/Boiling7.wav", true, false);
    
    // Load a one-shot sound effect (not looping, loaded into memory)
    resMgr.LoadAudio("button_click", "../assets/Audio/click.wav", false, false);
    
    // Check if audio is loaded
    if (resMgr.HasAudio("background_music"))
    {
        // Get audio information
        unsigned int lengthMs = 0;
        int channels = 0, bits = 0;
        float freq = 0;
        
        if (resMgr.GetAudioInfo("background_music", lengthMs, channels, bits, freq))
        {
            // Audio info retrieved successfully
            // Length: lengthMs, Channels: channels, Bits: bits, Frequency: freq
        }
    }
    
    // Unload audio when no longer needed
    resMgr.UnloadAudio("button_click");
    
    // Note: To play audio, you still need to use the AudioManager directly
    // through CoreEngine->GetSystem<AudioManager>()->PlaySound("audio_name")
}
