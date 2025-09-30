#pragma once

#include <iostream>
#include <fmod.hpp>
#include <fmod_errors.h>
#include <string>
#include <map>

#include "System.hpp"
#include "ConfigManager.hpp"

class AudioManager : public CoreFramework::SystemInterface
{
public:
    // Audio System functions for Core Engine

    /************************************************************************/
    /*!
    \brief
    Initializes the audio system and resources.
    */
    /************************************************************************/
    void Initialize() override;
    /************************************************************************/
    /*!
    \brief
    Updates the audio system each frame.
    \param dt
    Delta time since last update.
    */
    /************************************************************************/
    void Update(float dt) override;
    /************************************************************************/
    /*!
    \brief
    Handles messages sent to the audio system.
    \param message
    Pointer to the message object.
    */
    /************************************************************************/
    void SendMessage(CoreFramework::Message* message) override;
    /************************************************************************/
    /*!
    \brief
    Returns the name of the system.
    \return
    The system name as a string.
    */
    /************************************************************************/
    std::string GetName() override;

    // AudioManager functions

    /************************************************************************/
    /*!
    \brief
    Constructs the AudioManager and initializes member variables.
    */
    /************************************************************************/
    AudioManager();
    /************************************************************************/
    /*!
    \brief
    Destroys the AudioManager and releases resources.
    */
    /************************************************************************/
    ~AudioManager();

    // Initialization & Shutdown

    /************************************************************************/
    /*!
    \brief
    Initializes the FMOD audio system.
    \return
    True if initialization succeeded, false otherwise.
    */
    /************************************************************************/
    bool InitializeSystem();
    /************************************************************************/
    /*!
    \brief
    Shuts down the FMOD audio system and releases resources.
    */
    /************************************************************************/
    void Shutdown();

    // Sound Loading & Unloading

    /************************************************************************/
    /*!
    \brief
    Loads a sound file into the audio system.
    \param name
    The name to reference the sound.
    \param filepath
    The file path to the sound file.
    \param loop
    Whether the sound should loop.
    \return
    True if the sound was loaded successfully, false otherwise.
    */
    /************************************************************************/
    bool LoadSound(std::string const& name, std::string const& filepath, bool loop = false);
    /************************************************************************/
    /*!
    \brief
    Unloads a sound from the audio system.
    \param name
    The name of the sound to unload.
    */
    /************************************************************************/
    void UnloadSound(std::string const& name);

    // Playback Control

    /************************************************************************/
    /*!
    \brief
    Plays a loaded sound.
    \param name
    The name of the sound to play.
    \param volume
    Playback volume (0.0 to 1.0).
    \param paused
    Whether to start the sound paused.
    */
    /************************************************************************/
    void PlaySound(std::string const& name, float volume = 1.f, bool paused = false);
    /************************************************************************/
    /*!
    \brief
    Stops playback of a sound.
    \param name
    The name of the sound to stop.
    */
    /************************************************************************/
    void StopSound(std::string const& name);
    /************************************************************************/
    /*!
    \brief
    Stops all currently playing sounds.
    */
    /************************************************************************/
    void StopAllSounds();

    // Volume & Mute Control

    /************************************************************************/
    /*!
    \brief
    Sets the bgm volume.
    \param volume
    The new bgm volume (0.0 to 1.0).
    */
    /************************************************************************/
    void SetBgmVolume(float volume);
    /************************************************************************/
    /*!
    \brief
    Sets the vfx volume.
    \param volume
    The new vfx volume (0.0 to 1.0).
    */
    /************************************************************************/
	void SetVfxVolume(float volume);
    /************************************************************************/
    /*!
    \brief
    Gets the current bgm volume.
    \return
    The bgm volume (0.0 to 1.0).
    */
    /************************************************************************/
    float GetBgmVolume() const;
    /************************************************************************/
    /*!
    \brief
    Gets the current vfx volume.
    \return
    The vfx volume (0.0 to 1.0).
    */
    /************************************************************************/
    float GetVfxVolume() const;
    /************************************************************************/
    /*!
    \brief
    Mutes or unmutes all audio.
    \param shouldMute
    True to mute, false to unmute.
    */
    /************************************************************************/
    void Mute(bool shouldMute);
    /************************************************************************/
    /*!
    \brief
    Checks if the audio is currently muted.
    \return
    True if muted, false otherwise.
    */
    /************************************************************************/
    bool IsMuted() const;

	// Receive settings from ConfigManager

    /************************************************************************/
    /*!
    \brief
    Applies audio-related settings from the configuration manager.
    \param settings
    The settings to apply.
    */
    /************************************************************************/
    void ApplySettings(ConfigManager::Settings const& settings);

private:
    // Error handling
    void CheckError(FMOD_RESULT result, std::string const& context);

    // FMOD System and resources
    FMOD::System*                         system;
    FMOD::ChannelGroup*                   masterGroup;
    std::map<std::string, FMOD::Sound*>   sounds;
    std::map<std::string, FMOD::Channel*> channels;
    float                                 bgmVolume, vfxVolume;
    bool                                  muted;

};