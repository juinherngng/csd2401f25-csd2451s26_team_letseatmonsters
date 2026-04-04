/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			AudioManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (80%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu		(20%)

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

	/**
	 * @brief Initializes the audio manager system and its resources.
	 */
	void Initialize() override;

	/**
	 * @brief Updates queued playback, fades, and FMOD runtime state each frame.
	 * @param dt Delta time since the previous update.
	 */
	void Update(float dt) override;

	/**
	 * @brief Returns the system name used by the engine.
	 * @return System name string.
	 */
	std::string GetName() override;

	// AudioManager functions

	/**
	 * @brief Constructs the audio manager and subscribes it to message-bus events.
	 * @param bus Message bus used for pub/sub communication.
	 */
	AudioManager(CoreFramework::MessageBus& bus);

	/**
	 * @brief Destroys the audio manager and releases FMOD resources.
	 */
	~AudioManager();

	// Initialization & Shutdown

	/**
	 * @brief Initializes the underlying FMOD system.
	 * @return True if initialization succeeded, otherwise false.
	 */
	bool InitializeSystem();

	/**
	 * @brief Shuts down FMOD and releases all loaded audio resources.
	 */
	void Shutdown();

	// Sound Loading & Unloading

	/**
	 * @brief Loads a 2D sound file into the FMOD audio system.
	 * @param name Logical name used to reference the sound.
	 * @param filepath Path to the sound file.
	 * @param loop True when the sound should loop.
	 * @param stream True to stream from disk, false to fully load into memory.
	 * @return Pointer to the loaded FMOD sound, or nullptr on failure.
	 */
	FMOD::Sound* LoadSound(std::string const& name, std::string const& filepath, bool loop = false, bool stream = false);

	/**
	 * @brief Retrieves a previously loaded sound by name.
	 * @param name Logical sound name.
	 * @return Pointer to the FMOD sound, or nullptr if not found.
	 */
	FMOD::Sound* GetSound(std::string const& name) const;

	/**
	 * @brief Unloads a sound and releases its resources.
	 * @param name Logical sound name to unload.
	 */
	void UnloadSound(std::string const& name);

	/**
	 * @brief Checks whether a sound has already been loaded.
	 * @param name Logical sound name to query.
	 * @return True if the sound exists, otherwise false.
	 */
	bool HasSound(std::string const& name) const;

	/**
	 * @brief Retrieves metadata about a loaded sound.
	 * @param name Logical sound name.
	 * @param lengthMs Output length in milliseconds.
	 * @param outChannels Output number of channels.
	 * @param outBits Output bits per sample.
	 * @param freq Output default playback frequency in hertz.
	 * @return True if metadata was retrieved successfully, otherwise false.
	 */
	bool GetSoundInfo(std::string const& name, unsigned int& lengthMs, int& outChannels, int& outBits, float& freq) const;

	// Playback Control

	/**
	 * @brief Plays a previously loaded 2D sound.
	 * @param name Logical sound name to play.
	 * @param volume Playback volume in normalized range.
	 * @param paused True to start the channel paused.
	 */
	void PlaySound(std::string const& name, float volume = 1.f, bool paused = false);

	/**
	 * @brief Stops playback of all tracked channels for a named sound.
	 * @param name Logical sound name to stop.
	 */
	void StopSound(std::string const& name);

	/**
	 * @brief Stops only one currently playing instance of a named sound.
	 * @param name Logical sound name whose oldest tracked instance should stop.
	 */
	void StopOneSoundInstance(std::string const& name);

	/**
	 * @brief Checks whether any tracked instance of a sound is currently playing.
	 * @param name Logical sound name to query.
	 * @return True if at least one channel is still playing, otherwise false.
	 */
	bool IsSoundPlaying(std::string const& name);

	/**
	 * @brief Stops all currently tracked sounds.
	 */
	void StopAllSounds();

	// Volume & Mute Control

	/**
	 * @brief Sets the master output volume.
	 * @param volume New master volume in normalized range.
	 */
	void SetMasterVolume(float volume);

	/**
	 * @brief Sets the background-music volume.
	 * @param volume New BGM volume in normalized range.
	 */
	void SetBgmVolume(float volume);

	/**
	 * @brief Sets the VFX and SFX volume.
	 * @param volume New VFX volume in normalized range.
	 */
	void SetVfxVolume(float volume);

	/**
	 * @brief Returns the current master volume.
	 * @return Master volume in normalized range.
	 */
	float GetMasterVolume() const;

	/**
	 * @brief Returns the current BGM volume.
	 * @return BGM volume in normalized range.
	 */
	float GetBgmVolume() const;

	/**
	 * @brief Returns the current VFX volume.
	 * @return VFX volume in normalized range.
	 */
	float GetVfxVolume() const;

	/**
	 * @brief Sets the volume for all currently tracked channels of a named sound.
	 * @param name Logical sound name.
	 * @param volume New volume in normalized range.
	 */
	void SetVolume(std::string const& name, float volume);

	/**
	 * @brief Mutes or unmutes all audio output.
	 * @param shouldMute True to mute, false to unmute.
	 */
	void Mute(bool shouldMute);

	/**
	 * @brief Checks whether the audio output is currently muted.
	 * @return True if muted, otherwise false.
	 */
	bool IsMuted() const;

	// Receive settings from ConfigManager

	/**
	 * @brief Applies audio-related settings from the configuration manager.
	 * @param settings Settings object containing volume values.
	 */
	void ApplySettings(ConfigManager::Settings const& settings);

	/**
	 * @brief Returns the underlying FMOD system instance.
	 * @return Pointer to the FMOD system, or nullptr if not initialized.
	 */
	FMOD::System* GetSystem() const {
		// Expose the raw FMOD handle for low-level integrations that need it.
		return system;
	}

	/**
	 * @brief Queues a 2D play request to be flushed during the next update.
	 * @param name Logical sound name.
	 * @param volume Initial playback volume in normalized range.
	 * @param paused True to start the channel paused.
	 */
	void EnqueuePlay(std::string const& name, float volume = 1.f, bool paused = false);

	/**
	 * @brief Schedules a fade on a currently playing sound channel group.
	 * @param name Logical sound name whose channels should fade.
	 * @param toVolume Target volume in normalized range.
	 * @param duration Fade duration in seconds.
	 */
	void FadeChannel(std::string const& name, float toVolume, float duration);

	// UI Sound Effects

	/**
	 * @brief Plays the standard UI click sound effect.
	 */
	void PlayUIClickSound();

	// 3D Spatial Audio

	/**
	 * @brief Loads a sound file with FMOD 3D spatial settings enabled.
	 * @param name Logical sound name.
	 * @param filepath Path to the sound file.
	 * @param loop True when the sound should loop.
	 * @param stream True to stream from disk, false to fully load into memory.
	 * @return Pointer to the loaded FMOD sound, or nullptr on failure.
	 */
	FMOD::Sound* LoadSound3D(std::string const& name, std::string const& filepath, bool loop = false, bool stream = false);

	/**
	 * @brief Plays a loaded sound at a 3D world position.
	 * @param name Logical sound name to play.
	 * @param posX X world position of the sound source.
	 * @param posY Y world position of the sound source.
	 * @param posZ Z world position of the sound source.
	 * @param volume Playback volume in normalized range.
	 * @param minDistance Distance where attenuation starts.
	 * @param maxDistance Distance where attenuation reaches its far limit.
	 * @param paused True to start the channel paused.
	 */
	void PlaySound3D(std::string const& name, float posX, float posY, float posZ = 0.0f,
		float volume = 1.0f, float minDistance = 1.0f, float maxDistance = 50.0f, bool paused = false);

	/**
	 * @brief Sets the FMOD 3D listener position.
	 * @param posX Listener X world position.
	 * @param posY Listener Y world position.
	 * @param posZ Listener Z world position.
	 */
	void SetListenerPosition(float posX, float posY, float posZ = 0.0f);

	/**
	 * @brief Updates the 3D position of all tracked channels for a named sound.
	 * @param name Logical sound name.
	 * @param posX New X world position.
	 * @param posY New Y world position.
	 * @param posZ New Z world position.
	 */
	void Set3DChannelPosition(std::string const& name, float posX, float posY, float posZ = 0.0f);

	/**
	 * @brief Queues a 3D play request to be processed during the next update.
	 * @param name Logical sound name.
	 * @param posX X world position of the sound source.
	 * @param posY Y world position of the sound source.
	 * @param posZ Z world position of the sound source.
	 * @param volume Initial playback volume in normalized range.
	 * @param minDistance Distance where attenuation starts.
	 * @param maxDistance Distance where attenuation reaches its far limit.
	 * @param paused True to start the channel paused.
	 */
	void EnqueuePlay3D(std::string const& name, float posX, float posY, float posZ = 0.0f,
		float volume = 1.0f, float minDistance = 1.0f, float maxDistance = 50.0f, bool paused = false);

	/**
	 * @brief Pauses all currently playing sounds and music.
	 */
	void PauseAll();

	/**
	 * @brief Resumes all sounds and music paused by PauseAll().
	 */
	void ResumeAll();

	/**
	 * @brief Pauses all tracked channels for a specific sound.
	 * @param name Logical sound name to pause.
	 */
	void PauseChannel(std::string const& name);

	/**
	 * @brief Resumes all tracked channels for a specific sound.
	 * @param name Logical sound name to resume.
	 */
	void ResumeChannel(std::string const& name);

private:
	using ChannelList = std::vector<FMOD::Channel*>;

	/**
	 * @brief Logs FMOD errors with contextual information.
	 * @param result FMOD result code to inspect.
	 * @param context Context string describing the operation that was attempted.
	 */
	void CheckError(FMOD_RESULT result, std::string const& context);

	/**
	 * @brief Handles debug-toggle messages received from the message bus.
	 * @param msg Generic message payload.
	 */
	void OnToggleDebugInfo(const CoreFramework::Message& msg);

	/**
	 * @brief Handles 2D play-audio messages received from the message bus.
	 * @param msg Generic message payload.
	 */
	void OnPlayAudio(const CoreFramework::Message& msg);

	/**
	 * @brief Handles stop-audio messages received from the message bus.
	 * @param msg Generic message payload.
	 */
	void OnStopAudio(const CoreFramework::Message& msg);

	/**
	 * @brief Handles 3D play-audio messages received from the message bus.
	 * @param msg Generic message payload.
	 */
	void OnPlayAudio3D(const CoreFramework::Message& msg);

	/**
	 * @brief Computes the effective playback volume after category scaling.
	 * @param name Logical sound name.
	 * @param requestedVolume Caller-requested volume.
	 * @return Effective playback volume.
	 */
	float ComputePlaybackVolume(const std::string& name, float requestedVolume) const;

	/**
	 * @brief Queues a channel for deferred stopping after a silent mix block.
	 * @param channel FMOD channel to stop later.
	 */
	void QueueDeferredStop(FMOD::Channel* channel);

	/**
	 * @brief Silences and queues all channels in a tracked list for stopping.
	 * @param channelList Channel list to stop.
	 */
	void StopTrackedChannels(ChannelList& channelList);

	/**
	 * @brief Removes null or finished channels from a tracked list.
	 * @param channelList Channel list to prune.
	 */
	void RemoveStoppedChannels(ChannelList& channelList) const;

	/**
	 * @brief Removes empty channel buckets and stale fades from the tracking tables.
	 */
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
