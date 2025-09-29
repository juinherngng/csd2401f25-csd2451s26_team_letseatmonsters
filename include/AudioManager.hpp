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
    void Initialize() override;
    void Update(float dt) override;
    void SendMessage(CoreFramework::Message* message) override;
    std::string GetName() override;

    // AudioManager functions
    AudioManager();
    ~AudioManager();

    // Initialization & Shutdown
    bool InitializeSystem();
    void Shutdown();

    // Sound Loading & Unloading
    bool LoadSound(std::string const& name, std::string const& filepath, bool loop = false);
    void UnloadSound(std::string const& name);

    // Playback Control
    void PlaySound(std::string const& name, float volume = 1.f, bool paused = false);
    void StopSound(std::string const& name);
    void StopAllSounds();

    // Volume & Mute Control
    void SetBgmVolume(float volume);
	void SetVfxVolume(float volume);
    float GetBgmVolume() const;
    float GetVfxVolume() const;
    void Mute(bool shouldMute);
    bool IsMuted() const;

	// Receive settings from ConfigManager
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