/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			AudioManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		Audio manager using FMOD for sound playback and management.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <fmod.hpp>
#include <fmod_errors.h>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "EngineCore/ConfigManager.hpp"
#include "EngineCore/MessageBus.hpp"
#include "EngineCore/System.hpp"

class AudioManager : public CoreFramework::SystemInterface {
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
	\param bus
	Reference to the MessageBus for pub/sub messaging.
	*/
	/************************************************************************/
	AudioManager(CoreFramework::MessageBus& bus);
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
	\param stream
	Whether to stream the sound from disk (true) or load it fully into memory (false).
	\return
	Pointer to the loaded FMOD::Sound, or nullptr if loading failed.
	*/
	/************************************************************************/
	FMOD::Sound* LoadSound(std::string const& name, std::string const& filepath, bool loop = false, bool stream = false);
	/************************************************************************/
	/*!
	\brief
	Gets a previously loaded sound by name.
	\param name
	The name of the sound to retrieve.
	\return
	Pointer to the FMOD::Sound, or nullptr if not found.
	*/
	/************************************************************************/
	FMOD::Sound* GetSound(std::string const& name) const;
	/************************************************************************/
	/*!
	\brief
	Unloads a sound and releases its resources.
	\param name
	The name of the sound to unload.
	*/
	/************************************************************************/
	void UnloadSound(std::string const& name);
	/************************************************************************/
	/*!
	\brief
	Checks if a sound has been loaded.
	\param name
	The name of the sound to check.
	\return
	True if the sound exists, false otherwise.
	*/
	/************************************************************************/
	bool HasSound(std::string const& name) const;
	/************************************************************************/
	/*!
	\brief
	Retrieves information about a loaded sound.
	\param name
	The name of the sound.
	\param lengthMs
	Output: length of the sound in milliseconds.
	\param outChannels
	Output: number of audio channels.
	\param outBits
	Output: bits per sample.
	\param freq
	Output: default frequency in Hz.
	\return
	True if info was retrieved successfully, false otherwise.
	*/
	/************************************************************************/
	bool GetSoundInfo(std::string const& name, unsigned int& lengthMs, int& outChannels, int& outBits, float& freq) const;

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
	void StopOneSoundInstance(std::string const& name);
	bool IsSoundPlaying(std::string const& name);
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
	Sets the master volume.
	\param volume
	The new master volume (0.0 to 1.0).
	*/
	/************************************************************************/
	void SetMasterVolume(float volume);
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
	Gets the current master volume.
	\return
	The master volume (0.0 to 1.0).
	*/
	/************************************************************************/
	float GetMasterVolume() const;
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
	Sets the volume for a specific playing sound channel.
	\param name
	The name of the sound channel.
	\param volume
	The new volume (0.0 to 1.0).
	*/
	/************************************************************************/
	void SetVolume(std::string const& name, float volume);
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

	/************************************************************************/
	/*!
	\brief
	Gets the underlying FMOD system instance.
	\return
	Pointer to the FMOD::System instance.
	*/
	/************************************************************************/
	FMOD::System* GetSystem() const {
		return system;
	}

	/************************************************************************/
	/*!
	\brief
	Queues a request to play a sound next update rather than immediately.
	\details
	Useful when play requests arrive from multiple places (e.g., messages) and
	centralizing FMOD interaction on the audio thread/update. The request list
	is flushed at the start of Update().
	\param name
	Logical name of the sound (as loaded in AudioManager).
	\param volume
	Initial playback volume (0.f to 1.f).
	\param paused
	If true, starts the channel paused allowing further configuration before unpausing.
	*/
	/************************************************************************/
	void EnqueuePlay(std::string const& name, float volume = 1.f, bool paused = false);

	/************************************************************************/
	/*!
	\brief
	Schedules a volume fade on a currently playing channel.
	\details
	Interpolates from the channel's current volume to a target volume over the
	given duration. If duration <= 0, the volume is set instantly. Fades are
	processed each frame in Update(). If the channel stops, its fade is removed.
	\param name
	Logical name of the sound channel to fade.
	\param toVolume
	Target volume (0.f to 1.f).
	\param duration
	Fade time in seconds.
	*/
	/************************************************************************/
	void FadeChannel(std::string const& name, float toVolume, float duration);

	// UI Sound Effects

	/************************************************************************/
	/*!
	\brief
	Plays the UI click sound effect.
	\details
	Convenience method for playing button click sounds with appropriate volume.
	Uses VFX volume scaled down to 50% for subtle UI feedback.
	*/
	/************************************************************************/
	void PlayUIClickSound();

	// 3D Spatial Audio

	/************************************************************************/
	/*!
	\brief
	Loads a sound file with 3D spatial attributes.
	\param name
	The name to reference the sound.
	\param filepath
	The file path to the sound file.
	\param loop
	Whether the sound should loop.
	\param stream
	Whether to stream the sound from disk (true) or load it fully into memory (false).
	\return
	Pointer to the loaded FMOD::Sound, or nullptr if loading failed.
	*/
	/************************************************************************/
	FMOD::Sound* LoadSound3D(std::string const& name, std::string const& filepath, bool loop = false, bool stream = false);
	/************************************************************************/
	/*!
	\brief
	Plays a loaded sound at a 3D position in the world.
	\param name
	The name of the sound to play.
	\param posX
	X world position of the sound source.
	\param posY
	Y world position of the sound source.
	\param posZ
	Z world position of the sound source (default 0 for 2D games).
	\param volume
	Playback volume (0.0 to 1.0).
	\param minDistance
	Distance at which sound starts to attenuate.
	\param maxDistance
	Distance at which sound is fully attenuated.
	\param paused
	Whether to start the sound paused.
	*/
	/************************************************************************/
	void PlaySound3D(std::string const& name, float posX, float posY, float posZ = 0.0f,
		float volume = 1.0f, float minDistance = 1.0f, float maxDistance = 50.0f, bool paused = false);
	/************************************************************************/
	/*!
	\brief
	Sets the 3D listener position (typically the camera or player position).
	\param posX
	X world position of the listener.
	\param posY
	Y world position of the listener.
	\param posZ
	Z world position of the listener (default 0 for 2D games).
	*/
	/************************************************************************/
	void SetListenerPosition(float posX, float posY, float posZ = 0.0f);
	/************************************************************************/
	/*!
	\brief
	Updates the 3D position of an already-playing sound channel.
	\param name
	The name of the sound channel to update.
	\param posX
	New X world position.
	\param posY
	New Y world position.
	\param posZ
	New Z world position (default 0 for 2D games).
	*/
	/************************************************************************/
	void Set3DChannelPosition(std::string const& name, float posX, float posY, float posZ = 0.0f);
	/************************************************************************/
	/*!
	\brief
	Queues a request to play a 3D sound next update rather than immediately.
	\param name
	Logical name of the sound (as loaded in AudioManager).
	\param posX
	X world position of the sound source.
	\param posY
	Y world position of the sound source.
	\param posZ
	Z world position of the sound source.
	\param volume
	Initial playback volume (0.f to 1.f).
	\param minDistance
	Distance at which sound starts to attenuate.
	\param maxDistance
	Distance at which sound is fully attenuated.
	\param paused
	If true, starts the channel paused.
	*/
	/************************************************************************/
	void EnqueuePlay3D(std::string const& name, float posX, float posY, float posZ = 0.0f,
		float volume = 1.0f, float minDistance = 1.0f, float maxDistance = 50.0f, bool paused = false);

	void PauseAll();   // pause all currently playing sounds/music
	void ResumeAll();  // resume everything that was paused

	/************************************************************************/
	/*!
	\brief
	Pauses a specific sound channel by name.
	\param name
	The name of the sound channel to pause.
	*/
	/************************************************************************/
	void PauseChannel(std::string const& name);

	/************************************************************************/
	/*!
	\brief
	Resumes a specific sound channel by name.
	\param name
	The name of the sound channel to resume.
	*/
	/************************************************************************/
	void ResumeChannel(std::string const& name);

private:
	using ChannelList = std::vector<FMOD::Channel*>;

	/************************************************************************/
	/*!
	\brief
	Error handling for FMOD operations.
	\param result
	The FMOD_RESULT to check.
	\param context
	Contextual information for the error.
	*/
	/************************************************************************/
	void CheckError(FMOD_RESULT result, std::string const& context);

	// Message handlers
	void OnToggleDebugInfo(const CoreFramework::Message& msg);
	void OnPlayAudio(const CoreFramework::Message& msg);
	void OnStopAudio(const CoreFramework::Message& msg);
	void OnPlayAudio3D(const CoreFramework::Message& msg);
	float ComputePlaybackVolume(const std::string& name, float requestedVolume) const;
	void QueueDeferredStop(FMOD::Channel* channel);
	void StopTrackedChannels(ChannelList& channelList);
	void RemoveStoppedChannels(ChannelList& channelList) const;
	void PruneFinishedChannels();

	// FMOD System and resources
	FMOD::System* system;
	FMOD::ChannelGroup* masterGroup;
	std::map<std::string, FMOD::Sound*>   sounds;
	std::map<std::string, ChannelList>    channels;
	float                                 masterVolume, bgmVolume, vfxVolume;
	bool                                  muted;

	struct PendingPlay {
		std::string name;
		float volume;
		bool paused;
	};

	struct PendingPlay3D {
		std::string name;
		float posX, posY, posZ;
		float volume;
		float minDistance;
		float maxDistance;
		bool paused;
	};

	struct VolumeFade {
		float fromVolume;
		float toVolume;
		float duration;
		float elapsed;
	};

	std::vector<PendingPlay> pendingPlays;                      // queued play requests
	std::vector<PendingPlay3D> pendingPlays3D;                  // queued 3D play requests
	std::unordered_map<std::string, VolumeFade> activeFades;    // per-sound active fades
	std::vector<FMOD::Channel*> pendingStops;                   // channels silenced this frame, stopped next frame

	// Pub/sub
	CoreFramework::MessageBus& messageBus;
	CoreFramework::SubscriberId debugInfoSubId;
	CoreFramework::SubscriberId playAudioSubId;
	CoreFramework::SubscriberId stopAudioSubId;
	CoreFramework::SubscriberId playAudio3DSubId;
};
