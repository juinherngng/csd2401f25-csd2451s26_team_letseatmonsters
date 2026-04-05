/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			AudioManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (60%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu		(40%)

 DESCRIPTION:		Audio manager using FMOD for sound playback and management.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <algorithm>
#include <cctype>
#include <filesystem> // For checking file existence

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/Logger.hpp"

namespace {
	/**
	 * @brief Converts engine world coordinates into FMOD's 3D coordinate space.
	 * @param worldX Engine-space X coordinate.
	 * @param worldY Engine-space Y coordinate.
	 * @param worldZ Engine-space Z coordinate.
	 * @return FMOD vector in the listener/source coordinate system.
	 */
	FMOD_VECTOR ToFmodWorld(float worldX, float worldY, float worldZ) {
		// Map game 2D plane (X,Y) onto FMOD ground plane (X,Z).
		// Keep FMOD Y as vertical-up so up/down movement in game affects depth/distance.
		FMOD_VECTOR out{};
		out.x = -worldX;
		out.y = worldZ;
		out.z = -worldY;
		return out;
	}

	/**
	 * @brief Returns a lowercase copy of a string.
	 * @param value Input string.
	 * @return Lowercased copy of the input.
	 */
	std::string ToLowerCopy(std::string value) {
		// Normalize case once so later name checks can stay simple and case-insensitive.
		std::transform(value.begin(), value.end(), value.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
		return value;
	}

	/**
	 * @brief Checks whether a channel name should be treated as background music.
	 * @param name Logical sound name.
	 * @return True when the name looks like a BGM channel.
	 */
	bool IsBgmChannelName(const std::string& name) {
		return ToLowerCopy(name).find("bgm") != std::string::npos;
	}

	/**
	 * @brief Reduces a full path to just its file name for cleaner logs.
	 * @param path File path to shorten.
	 * @return File name portion of the path.
	 */
	std::string CompactPathLabel(const std::string& path) {
		if (path.empty()) {
			return path;
		}

		return std::filesystem::path(path).filename().string();
	}

	/**
	 * @brief Returns any extra volume multiplier applied to special BGM channels.
	 * @param name Logical sound name.
	 * @return Additional multiplier for the named BGM channel.
	 */
	float GetBgmChannelVolumeMultiplier(const std::string& name) {
		const std::string lower = ToLowerCopy(name);
		if (lower.find("ambience") != std::string::npos) {
			return 0.5f;
		}
		if (lower.find("introcutscene") != std::string::npos) {
			return 1.6f;
		}
		return 1.0f;
	}
}

/**
 * @brief Stops one tracked instance of a named sound.
 * @param name Logical sound name.
 */
void AudioManager::StopOneSoundInstance(std::string const& name) {
	auto it = channels.find(name);
	if (it == channels.end()) {
		return;
	}

	// Drop any channels that have already ended before choosing one to stop.
	RemoveStoppedChannels(it->second);
	if (it->second.empty()) {
		activeFades.erase(name);
		channels.erase(it);
		return;
	}

	FMOD::Channel* channel = it->second.front();
	it->second.erase(it->second.begin());
	if (channel) {
		// Fade to silence immediately, then stop on the next update to avoid clicks.
		channel->setVolume(0.0f);
		QueueDeferredStop(channel);
	}

	if (it->second.empty()) {
		activeFades.erase(name);
		channels.erase(it);
	}
}

/**
 * @brief Constructs the audio manager and subscribes to audio-related messages.
 * @param bus Message bus used for engine-wide pub/sub communication.
 */
AudioManager::AudioManager(CoreFramework::MessageBus& bus) : messageBus(bus), system(nullptr), masterGroup(nullptr), masterVolume(1.f), bgmVolume(1.f), vfxVolume(1.f), muted(false) {
	// Subscribe to messages
	debugInfoSubId = messageBus.Subscribe(
		CoreFramework::MessageType::TOGGLE_DEBUG_INFO,
		[this](const CoreFramework::Message& msg) { OnToggleDebugInfo(msg); }
	);

	// Subscribe to PLAY_AUDIO messages
	playAudioSubId = messageBus.Subscribe(
		CoreFramework::MessageType::PLAY_AUDIO,
		[this](const CoreFramework::Message& msg) { OnPlayAudio(msg); }
	);

	// Subscribe to STOP_AUDIO messages
	stopAudioSubId = messageBus.Subscribe(
		CoreFramework::MessageType::STOP_AUDIO,
		[this](const CoreFramework::Message& msg) { OnStopAudio(msg); }
	);

	// Subscribe to PLAY_AUDIO_3D messages
	playAudio3DSubId = messageBus.Subscribe(
		CoreFramework::MessageType::PLAY_AUDIO_3D,
		[this](const CoreFramework::Message& msg) { OnPlayAudio3D(msg); }
	);
}

/**
 * @brief Destroys the audio manager and releases all owned resources.
 */
AudioManager::~AudioManager() {
	// Unsubscribe from messages
	messageBus.Unsubscribe(CoreFramework::MessageType::TOGGLE_DEBUG_INFO, debugInfoSubId);
	messageBus.Unsubscribe(CoreFramework::MessageType::PLAY_AUDIO, playAudioSubId);
	messageBus.Unsubscribe(CoreFramework::MessageType::STOP_AUDIO, stopAudioSubId);
	messageBus.Unsubscribe(CoreFramework::MessageType::PLAY_AUDIO_3D, playAudio3DSubId);

	Shutdown();
}

/**
 * @brief Initializes the FMOD-backed audio manager system.
 */
void AudioManager::Initialize() {
	if (InitializeSystem()) {
		TS_LOG_INFO("AudioManager system initialized.");

		// Audio files should now be loaded through ResourceManager
	}
	else {
		TS_LOG_ERROR("AudioManager system failed to initialize.");
	}
}

/**
 * @brief Updates deferred audio work, fades, and the FMOD system for this frame.
 * @param dt Frame delta time in seconds.
 */
void AudioManager::Update(float dt) {
	if (!system) return;

	// Flush queued 2D play requests first so message-driven playback stays centralized here.
	if (!pendingPlays.empty()) {
		for (const auto& playReq : pendingPlays) {
			PlaySound(playReq.name, playReq.volume, playReq.paused);
		}
		pendingPlays.clear();
	}

	// Then flush queued 3D play requests using the same update-thread FMOD access pattern.
	if (!pendingPlays3D.empty()) {
		for (const auto& playReq : pendingPlays3D) {
			PlaySound3D(playReq.name, playReq.posX, playReq.posY, playReq.posZ,
				playReq.volume, playReq.minDistance, playReq.maxDistance, playReq.paused);
		}
		pendingPlays3D.clear();
	}

	// Advance any active fades and prune entries whose channels have disappeared.
	if (!activeFades.empty()) {
		for (auto it = activeFades.begin(); it != activeFades.end(); ) {
			auto chanIt = channels.find(it->first);
			if (chanIt == channels.end()) {
				it = activeFades.erase(it);
				continue;
			}

			RemoveStoppedChannels(chanIt->second);
			if (chanIt->second.empty()) {
				channels.erase(chanIt);
				it = activeFades.erase(it);
				continue;
			}

			VolumeFade& f = it->second;
			f.elapsed += dt;

			// Interpolate linearly from the captured start volume to the requested target.
			float t = (f.duration > 0.f) ? std::min(f.elapsed / f.duration, 1.f) : 1.f;
			float newVol = f.fromVolume + (f.toVolume - f.fromVolume) * t;
			for (FMOD::Channel* channel : chanIt->second) {
				if (channel) {
					channel->setVolume(newVol);
				}
			}

			if (t >= 1.f)
				it = activeFades.erase(it);
			else
				++it;
		}
	}

	// Commit the accumulated channel/state changes to FMOD.
	system->update();

	// Stop channels that were silenced last frame FMOD has now mixed
	// at least one block of silence so stopping won't produce a click.
	for (FMOD::Channel* ch : pendingStops) {
		if (ch) {
			ch->stop();
		}
	}
	pendingStops.clear();

	PruneFinishedChannels();
}

/**
 * @brief Handles debug-toggle messages relevant to the audio manager.
 * @param msg Generic message payload.
 */
void AudioManager::OnToggleDebugInfo(const CoreFramework::Message& msg) {
	// Could toggle audio debug overlay logging, etc.
	(void)msg; // Suppress unused parameter warning
}

/**
 * @brief Handles PLAY_AUDIO messages by queueing a 2D play request.
 * @param msg Generic message payload.
 */
void AudioManager::OnPlayAudio(const CoreFramework::Message& msg) {
	// Cast to specific message type
	const auto& playMsg = static_cast<const CoreFramework::PlayAudioMessage&>(msg);

	// Enqueue the audio playback request
	EnqueuePlay(playMsg.soundName, playMsg.volume, playMsg.paused);
}

/**
 * @brief Handles STOP_AUDIO messages by stopping one sound or all sounds.
 * @param msg Generic message payload.
 */
void AudioManager::OnStopAudio(const CoreFramework::Message& msg) {
	// Cast to specific message type
	const auto& stopMsg = static_cast<const CoreFramework::StopAudioMessage&>(msg);

	// If sound name is empty, stop all sounds
	if (stopMsg.soundName.empty()) {
		StopAllSounds();
	}
	else {
		StopSound(stopMsg.soundName);
	}
}

/**
 * @brief Handles PLAY_AUDIO_3D messages by queueing a spatial play request.
 * @param msg Generic message payload.
 */
void AudioManager::OnPlayAudio3D(const CoreFramework::Message& msg) {
	const auto& playMsg = static_cast<const CoreFramework::PlayAudio3DMessage&>(msg);

	EnqueuePlay3D(playMsg.soundName, playMsg.posX, playMsg.posY, playMsg.posZ,
		playMsg.volume, playMsg.minDistance, playMsg.maxDistance, playMsg.paused);
}

/**
 * @brief Returns the engine-facing system name.
 * @return System name string.
 */
std::string AudioManager::GetName() {
	// Match the identifier used when systems are logged or inspected by the core engine.
	return "AudioManager";
}

/**
 * @brief Initializes the FMOD system and default audio state.
 * @return True if the system initialized successfully, otherwise false.
 */
bool AudioManager::InitializeSystem() {
	// Initialize FMOD system
	FMOD_RESULT result = FMOD::System_Create(&system);
	CheckError(result, "System_Create");

	if (result != FMOD_OK) return false;

	// default init with 512 channels, enable 3D spatial audio
	result = system->init(512, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, nullptr);
	CheckError(result, "system->init");

	if (result != FMOD_OK) return false;

	// get master channel group
	result = system->getMasterChannelGroup(&masterGroup);
	CheckError(result, "getMasterChannelGroup");

	if (result != FMOD_OK) return false;

	// set initial volumes
	SetMasterVolume(masterVolume);
	SetBgmVolume(bgmVolume);
	SetVfxVolume(vfxVolume);
	muted = false;

	return true;
}

/**
 * @brief Shuts down FMOD and releases all tracked channels and sounds.
 */
void AudioManager::Shutdown() {
	// stop all sounds and clear channels
	StopAllSounds();
	// Flush any deferred stops immediately since the system is shutting down
	for (FMOD::Channel* ch : pendingStops) {
		if (ch) ch->stop();
	}
	pendingStops.clear();
	channels.clear();

	// Release all loaded sounds
	for (auto& [name, sound] : sounds) {
		if (sound) {
			sound->release();
			TS_LOG_DEBUG("Released sound: " << name);
		}
	}
	sounds.clear();

	if (system) {
		// close and release FMOD system
		system->release();
		system = nullptr;
	}

	masterGroup = nullptr;
}

/**
 * @brief Loads a 2D sound into FMOD under a logical name.
 * @param name Logical sound name.
 * @param filePath Path to the audio file.
 * @param loop True when the sound should loop.
 * @param stream True when the sound should stream from disk.
 * @return Pointer to the loaded FMOD sound, or nullptr on failure.
 */
FMOD::Sound* AudioManager::LoadSound(std::string const& name, std::string const& filePath, bool loop, bool stream) {
	// Ensure audio system is set
	if (!system) {
		TS_LOG_ERROR("Audio system not initialized!");
		return nullptr;
	}

	// Check if already loaded
	if (auto it = sounds.find(name); it != sounds.end()) {
		TS_LOG_DEBUG("Audio '" << name << "' already loaded, returning existing.");
		return it->second;
	}

	TS_LOG_DEBUG("[AudioManager] Loading '" << name << "' from '" << CompactPathLabel(filePath) << "'.");

	// Check if file exists
	if (!std::filesystem::exists(filePath)) {
		TS_LOG_ERROR("[AudioManager] File does not exist at path: " << filePath);

		// Try to get absolute path for debugging
		try {
			std::filesystem::path absPath = std::filesystem::absolute(filePath);
			TS_LOG_ERROR("Absolute path would be: " << absPath.string());
			TS_LOG_ERROR("Current working directory: " << std::filesystem::current_path().string());
		}
		catch (...) {
			TS_LOG_ERROR("Could not determine absolute path");
		}

		return nullptr;
	}

	TS_LOG_DEBUG("[AudioManager] File found, continuing with FMOD load.");

	// Build FMOD mode flags from the engine-facing looping and streaming options.
	FMOD_MODE mode = FMOD_DEFAULT | (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) |
		(stream ? FMOD_CREATESTREAM : FMOD_CREATESAMPLE);

	// Load sound
	FMOD::Sound* sound = nullptr;
	FMOD_RESULT result = system->createSound(filePath.c_str(), mode, nullptr, &sound);

	// Check for errors
	if (result != FMOD_OK) {
		TS_LOG_ERROR("Failed to load audio '" << name << "': " << FMOD_ErrorString(result));
		return nullptr;
	}

	// Store sound
	sounds.emplace(name, sound);

	TS_LOG_INFO("Loaded audio: " << name);
	return sound;
}

/**
 * @brief Looks up a loaded sound by logical name.
 * @param name Logical sound name.
 * @return Pointer to the loaded FMOD sound, or nullptr if not found.
 */
FMOD::Sound* AudioManager::GetSound(std::string const& name) const {
	// Check if audio exists
	if (auto it = sounds.find(name); it != sounds.end()) return it->second;

	TS_LOG_WARN("Audio '" << name << "' not found!");
	return nullptr;
}

/**
 * @brief Unloads a sound and stops any playing instances of it.
 * @param name Logical sound name to unload.
 */
void AudioManager::UnloadSound(std::string const& name) {
	// Stop if playing
	StopSound(name);

	// Check if audio exists and release
	if (auto it = sounds.find(name); it != sounds.end()) {
		if (it->second) {
			it->second->release();
		}
		sounds.erase(it);
		TS_LOG_INFO("Unloaded audio: " << name);
	}
}

/**
 * @brief Checks whether a logical sound name is currently loaded.
 * @param name Logical sound name.
 * @return True if the sound exists in the loaded map, otherwise false.
 */
bool AudioManager::HasSound(std::string const& name) const {
	// Check if audio exists
	return sounds.find(name) != sounds.end();
}

/**
 * @brief Retrieves metadata for a loaded sound.
 * @param name Logical sound name.
 * @param lengthMs Output sound length in milliseconds.
 * @param outChannels Output channel count.
 * @param outBits Output bits per sample.
 * @param freq Output default frequency.
 * @return True if the metadata was retrieved successfully, otherwise false.
 */
bool AudioManager::GetSoundInfo(std::string const& name, unsigned int& lengthMs, int& outChannels, int& outBits, float& freq) const {
	auto it = sounds.find(name);
	if (it == sounds.end() || !it->second) {
		TS_LOG_WARN("Audio '" << name << "' not found!");
		return false;
	}

	// Retrieve sound info
	FMOD::Sound* snd = it->second;

	// Get length in milliseconds
	if (snd->getLength(&lengthMs, FMOD_TIMEUNIT_MS) != FMOD_OK) return false;

	// Get format info
	FMOD_SOUND_TYPE type;
	FMOD_SOUND_FORMAT format;
	int numChannels = 0;
	int numBits = 0;
	// outBits and outChannels are output parameters
	if (snd->getFormat(&type, &format, &numChannels, &numBits) != FMOD_OK) return false;

	// Get default frequency
	if (snd->getDefaults(&freq, nullptr) != FMOD_OK) freq = 0;

	outChannels = numChannels;
	outBits = numBits;

	return true;
}

/**
 * @brief Plays a previously loaded sound as 2D audio.
 * @param name Logical sound name.
 * @param volume Requested playback volume.
 * @param paused True to keep the channel paused after creation.
 */
void AudioManager::PlaySound(std::string const& name, float volume, bool paused) {
	if (!system) return;

	// Get the sound
	FMOD::Sound* sound = GetSound(name);

	if (!sound) {
		TS_LOG_WARN("[AudioManager] PlaySound: Sound '" << name << "' not found in loaded sounds!");
		return;
	}

	// Always start paused so we can set volume before any audio is mixed,
	// preventing a brief burst at default volume (FMOD best practice).
	FMOD::Channel* channel = nullptr;
	FMOD_RESULT result = system->playSound(sound, nullptr, true, &channel);
	CheckError(result, "playSound: " + name);

	// set volume based on type, multiplied by master volume
	// If caller provided a specific volume (not default 1.0f), use it directly
	// Otherwise, use category-based volume
	if (result == FMOD_OK && channel) {
		// Force non-spatial playback for the 2D API path even if the underlying
		// sound asset was loaded with 3D flags.
		channel->setMode(FMOD_2D);

		// Resolve the final volume after category scaling and global audio settings.
		const float finalVolume = ComputePlaybackVolume(name, volume);

		channel->setVolume(finalVolume);
		channels[name].push_back(channel);

		// Unpause now that volume is set, unless caller requested paused start
		if (!paused) {
			channel->setPaused(false);
		}

		TS_LOG_DEBUG("[AudioManager] Playing sound '" << name << "' at volume " << finalVolume);
	}
}

/**
 * @brief Stops all tracked channels for a named sound.
 * @param name Logical sound name.
 */
void AudioManager::StopSound(std::string const& name) {
	// Stop and remove channel if exists
	auto it = channels.find(name);

	if (it != channels.end()) {
		StopTrackedChannels(it->second);
		activeFades.erase(name);
		channels.erase(it);
	}
}

/**
 * @brief Checks whether any tracked channel for a named sound is still playing.
 * @param name Logical sound name.
 * @return True if any channel is still active, otherwise false.
 */
bool AudioManager::IsSoundPlaying(std::string const& name) {
	auto it = channels.find(name);
	if (it == channels.end()) {
		return false;
	}

	RemoveStoppedChannels(it->second);
	if (it->second.empty()) {
		return false;
	}

	for (FMOD::Channel* channel : it->second) {
		if (!channel) {
			continue;
		}

		bool isPlaying = false;
		if (channel->isPlaying(&isPlaying) == FMOD_OK && isPlaying) {
			return true;
		}
	}

	return false;
}

/**
 * @brief Stops every tracked channel in the audio manager.
 */
void AudioManager::StopAllSounds() {
	// Silence all tracked channels and defer stop (same as StopSound)
	for (auto& [name, channelList] : channels) {
		StopTrackedChannels(channelList);
	}

	activeFades.clear();
	channels.clear();
}

/**
 * @brief Temporarily pauses all audio without destroying channel state.
 */
void AudioManager::PauseAll() {
	if (masterGroup) {
		// FMOD channel groups let the engine pause everything with one call.
		masterGroup->setPaused(true);
	}
}

/**
 * @brief Resumes all audio paused through PauseAll().
 */
void AudioManager::ResumeAll() {
	if (masterGroup) {
		masterGroup->setPaused(false);
	}
}

/**
 * @brief Pauses every tracked channel for a named sound.
 * @param name Logical sound name.
 */
void AudioManager::PauseChannel(std::string const& name) {
	auto it = channels.find(name);
	if (it != channels.end()) {
		for (FMOD::Channel* channel : it->second) {
			if (channel) {
				channel->setPaused(true);
			}
		}
		TS_LOG_DEBUG("[AudioManager] Paused channel: " << name);
	}
}

/**
 * @brief Resumes every tracked channel for a named sound.
 * @param name Logical sound name.
 */
void AudioManager::ResumeChannel(std::string const& name) {
	auto it = channels.find(name);
	if (it != channels.end()) {
		for (FMOD::Channel* channel : it->second) {
			if (channel) {
				channel->setPaused(false);
			}
		}
		TS_LOG_DEBUG("[AudioManager] Resumed channel: " << name);
	}
}

/**
 * @brief Sets the master output volume.
 * @param volume New master volume.
 */
void AudioManager::SetMasterVolume(float volume) {
	// Clamp volume between 0.0 and 1.0
	masterVolume = std::clamp(volume, 0.0f, 1.0f);

	if (masterGroup && !muted)
		masterGroup->setVolume(masterVolume);
}

/**
 * @brief Sets the BGM volume and reapplies it to active BGM channels.
 * @param volume New BGM volume.
 */
void AudioManager::SetBgmVolume(float volume) {
	// Clamp volume between 0.0 and 1.0
	bgmVolume = std::clamp(volume, 0.0f, 1.0f);

	// Retune currently playing BGM channels immediately so settings UI changes
	// affect the active menu/game music without needing to restart playback.
	for (auto it = channels.begin(); it != channels.end(); ++it) {
		if (!IsBgmChannelName(it->first)) {
			continue;
		}

		RemoveStoppedChannels(it->second);
		if (it->second.empty()) {
			continue;
		}

		const float targetVolume = bgmVolume * GetBgmChannelVolumeMultiplier(it->first);
		for (FMOD::Channel* channel : it->second) {
			if (channel) {
				channel->setVolume(targetVolume);
			}
		}
	}

	// If the user is dragging the settings slider, the new value should win
	// immediately instead of being overwritten by an old fade target next frame.
	for (auto it = activeFades.begin(); it != activeFades.end(); ) {
		if (IsBgmChannelName(it->first)) {
			it = activeFades.erase(it);
		}
		else {
			++it;
		}
	}
}

/**
 * @brief Sets the VFX volume category.
 * @param volume New VFX volume.
 */
void AudioManager::SetVfxVolume(float volume) {
	// Clamp volume between 0.0 and 1.0
	vfxVolume = std::clamp(volume, 0.0f, 1.0f);

	// VFX volume is applied when sounds are played; active channels are not retroactively retuned here.
}

/**
 * @brief Returns the current master volume.
 * @return Master volume in normalized range.
 */
float AudioManager::GetMasterVolume() const {
	// Return the cached engine-side master volume value.
	return masterVolume;
}

/**
 * @brief Returns the current BGM volume.
 * @return BGM volume in normalized range.
 */
float AudioManager::GetBgmVolume() const {
	// Return the cached BGM slider value.
	return bgmVolume;
}

/**
 * @brief Returns the current VFX volume.
 * @return VFX volume in normalized range.
 */
float AudioManager::GetVfxVolume() const {
	// Return the cached VFX/SFX slider value.
	return vfxVolume;
}

/**
 * @brief Sets the volume on all tracked channels for a named sound.
 * @param name Logical sound name.
 * @param volume New channel volume.
 */
void AudioManager::SetVolume(std::string const& name, float volume) {
	// Find the channel and set its volume
	auto it = channels.find(name);
	if (it != channels.end()) {
		float clampedVolume = std::clamp(volume, 0.0f, 1.0f);
		for (FMOD::Channel* channel : it->second) {
			if (channel) {
				channel->setVolume(clampedVolume);
			}
		}
	}
}

/**
 * @brief Mutes or unmutes the master output.
 * @param shouldMute True to mute, false to restore master volume.
 */
void AudioManager::Mute(bool shouldMute) {
	// Mute or unmute all audio
	muted = shouldMute;

	if (masterGroup)
		masterGroup->setVolume(muted ? 0.f : masterVolume);
}

/**
 * @brief Checks whether the audio manager is currently muted.
 * @return True if muted, otherwise false.
 */
bool AudioManager::IsMuted() const {
	// Mute state is tracked separately from volume so settings can be restored cleanly.
	return muted;
}

/**
 * @brief Logs an FMOD error when an operation fails.
 * @param result FMOD result code to inspect.
 * @param context Context string describing the failing operation.
 */
void AudioManager::CheckError(FMOD_RESULT result, std::string const& context) {
	// Log error if not OK
	if (result != FMOD_OK) {
		TS_LOG_ERROR("[FMOD] Error in " << context << ": " << FMOD_ErrorString(result));
	}
}

/**
 * @brief Applies audio volumes from the configuration system.
 * @param settings Configuration settings containing volume values.
 */
void AudioManager::ApplySettings(ConfigManager::Settings const& settings) {
	// Apply audio settings
	SetMasterVolume(settings.masterVolume);
	SetBgmVolume(settings.bgmVolume);
	SetVfxVolume(settings.vfxVolume);
	TS_LOG_INFO("Audio settings applied: Master Volume = " << settings.masterVolume << ", BGM Volume = " << settings.bgmVolume << ", VFX Volume = " << settings.vfxVolume);
}

/**
 * @brief Queues a 2D play request for the next update.
 * @param name Logical sound name.
 * @param volume Requested playback volume.
 * @param paused True to start paused.
 */
void AudioManager::EnqueuePlay(std::string const& name, float volume, bool paused) {
	// Add to pending plays queue
	pendingPlays.push_back({ name, volume, paused });
}

/**
 * @brief Schedules a fade for all tracked channels of a named sound.
 * @param name Logical sound name.
 * @param toVolume Target volume.
 * @param duration Fade duration in seconds.
 */
void AudioManager::FadeChannel(std::string const& name, float toVolume, float duration) {
	// Start volume fade on channel if exists
	auto it = channels.find(name);

	if (it == channels.end() || duration <= 0.f) return;

	RemoveStoppedChannels(it->second);
	if (it->second.empty()) return;

	// get current volume
	float currentVolume = 0.f;
	it->second.front()->getVolume(&currentVolume);

	// set up fade
	activeFades[name] = VolumeFade{ currentVolume, toVolume, duration, 0.f };
}

/**
 * @brief Plays the standard UI click sound.
 */
void AudioManager::PlayUIClickSound() {
	// Play UI click sound with appropriate volume for subtle feedback
	float clickVolume = GetVfxVolume() * 0.5f; // 50% of VFX volume for subtle UI sounds
	PlaySound("ui_click", clickVolume, false);
}

/**
 * @brief Loads a spatial 3D sound into FMOD.
 * @param name Logical sound name.
 * @param filePath Path to the audio file.
 * @param loop True when the sound should loop.
 * @param stream True when the sound should stream from disk.
 * @return Pointer to the loaded FMOD sound, or nullptr on failure.
 */
FMOD::Sound* AudioManager::LoadSound3D(std::string const& name, std::string const& filePath, bool loop, bool stream) {
	if (!system) {
		TS_LOG_ERROR("Audio system not initialized!");
		return nullptr;
	}

	// Check if already loaded
	if (auto it = sounds.find(name); it != sounds.end()) {
		TS_LOG_DEBUG("Audio '" << name << "' already loaded, returning existing.");
		return it->second;
	}

	TS_LOG_DEBUG("[AudioManager] Loading 3D sound '" << name << "' from '" << CompactPathLabel(filePath) << "'.");

	if (!std::filesystem::exists(filePath)) {
		TS_LOG_ERROR("[AudioManager] File does not exist at path: " << filePath);
		try {
			std::filesystem::path absPath = std::filesystem::absolute(filePath);
			TS_LOG_ERROR("Absolute path would be: " << absPath.string());
			TS_LOG_ERROR("Current working directory: " << std::filesystem::current_path().string());
		}
		catch (...) {
			TS_LOG_ERROR("Could not determine absolute path");
		}
		return nullptr;
	}

	TS_LOG_DEBUG("[AudioManager] File found, continuing with FMOD 3D load.");

	// Set FMOD mode flags with FMOD_3D for spatial audio.
	// Use inverse rolloff to avoid abrupt drop-offs that feel like early cut-outs.
	FMOD_MODE mode = FMOD_3D | FMOD_3D_INVERSEROLLOFF
		| (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF)
		| (stream ? FMOD_CREATESTREAM : FMOD_CREATESAMPLE);

	FMOD::Sound* sound = nullptr;
	FMOD_RESULT result = system->createSound(filePath.c_str(), mode, nullptr, &sound);

	if (result != FMOD_OK) {
		TS_LOG_ERROR("Failed to load 3D audio '" << name << "': " << FMOD_ErrorString(result));
		return nullptr;
	}

	// Set default 3D min/max distances
	sound->set3DMinMaxDistance(1.0f, 50.0f);

	sounds.emplace(name, sound);
	TS_LOG_INFO("Loaded 3D audio: " << name);
	return sound;
}

/**
 * @brief Plays a previously loaded sound with 3D positioning.
 * @param name Logical sound name.
 * @param posX Sound source X position.
 * @param posY Sound source Y position.
 * @param posZ Sound source Z position.
 * @param volume Requested playback volume.
 * @param minDistance Near attenuation distance.
 * @param maxDistance Far attenuation distance.
 * @param paused True to keep the channel paused after setup.
 */
void AudioManager::PlaySound3D(std::string const& name, float posX, float posY, float posZ,
	float volume, float minDistance, float maxDistance, bool paused) {
	if (!system) return;

	FMOD::Sound* sound = GetSound(name);
	if (!sound) {
		TS_LOG_WARN("[AudioManager] PlaySound3D: Sound '" << name << "' not found in loaded sounds!");
		return;
	}

	// Start paused so we can configure 3D attributes before any audio is mixed
	FMOD::Channel* channel = nullptr;
	FMOD_RESULT result = system->playSound(sound, nullptr, true, &channel);
	CheckError(result, "playSound3D: " + name);

	if (result == FMOD_OK && channel) {
		// Configure spatial attributes before the sound becomes audible.
		FMOD_VECTOR pos = ToFmodWorld(posX, posY, posZ);
		FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
		channel->set3DAttributes(&pos, &vel);

		const bool isCustomerOneShot =
			(name.find("customer") != std::string::npos) ||
			(name.find("vo_customer") != std::string::npos) ||
			(name.find("sfx_payment") != std::string::npos) ||
			(name.find("sfx_wrong_order") != std::string::npos);

		// Set 3D min/max distance for attenuation
		// Keep distance response subtle: audible near/far change, but not drastic.
		const float distanceScale = isCustomerOneShot ? 8.0f : 1.5f;
		const float effectiveMinDistance = std::max(1.0f, minDistance * distanceScale);
		const float effectiveMaxDistance = std::max(effectiveMinDistance + 1.0f, maxDistance * distanceScale);
		channel->set3DMinMaxDistance(effectiveMinDistance, effectiveMaxDistance);

		// Keep customer one-shots from sounding like they are abruptly cut by attenuation.
		channel->set3DLevel(isCustomerOneShot ? 0.55f : 1.0f);
		channel->set3DSpread(isCustomerOneShot ? 60.0f : 20.0f);

		// Set volume with category and master scaling, then boost 3D audibility.
		const float finalVolume = ComputePlaybackVolume(name, volume);
		const float gainBoost = isCustomerOneShot ? 1.25f : 1.5f;
		const float boostedVolume = std::clamp(finalVolume * gainBoost, 0.0f, 1.0f);

		channel->setVolume(boostedVolume);
		channels[name].push_back(channel);

		// Unpause now that 3D attributes and volume are set
		if (!paused) {
			channel->setPaused(false);
		}

		TS_LOG_DEBUG("[AudioManager] Playing 3D sound '" << name << "' at position ("
			<< posX << ", " << posY << ", " << posZ << ") volume " << boostedVolume);
	}
}

/**
 * @brief Updates the FMOD listener transform.
 * @param posX Listener X position.
 * @param posY Listener Y position.
 * @param posZ Listener Z position.
 */
void AudioManager::SetListenerPosition(float posX, float posY, float posZ) {
	if (!system) return;

	// Keep FMOD's listener aligned with the active camera/player position in engine space.
	FMOD_VECTOR listenerPos = ToFmodWorld(posX, posY, posZ);
	FMOD_VECTOR listenerVel = { 0.0f, 0.0f, 0.0f };
	FMOD_VECTOR forward = { 0.0f, 0.0f, 1.0f };
	FMOD_VECTOR up = { 0.0f, 1.0f, 0.0f };

	FMOD_RESULT result = system->set3DListenerAttributes(0, &listenerPos, &listenerVel, &forward, &up);
	CheckError(result, "set3DListenerAttributes");
}

/**
 * @brief Updates the 3D position of all tracked channels for a named sound.
 * @param name Logical sound name.
 * @param posX New X position.
 * @param posY New Y position.
 * @param posZ New Z position.
 */
void AudioManager::Set3DChannelPosition(std::string const& name, float posX, float posY, float posZ) {
	auto it = channels.find(name);
	if (it == channels.end()) return;

	// Update every active instance so looping spatial sounds follow their world object.
	FMOD_VECTOR pos = ToFmodWorld(posX, posY, posZ);
	FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
	for (FMOD::Channel* channel : it->second) {
		if (channel) {
			channel->set3DAttributes(&pos, &vel);
		}
	}
}

/**
 * @brief Queues a 3D play request for the next update.
 * @param name Logical sound name.
 * @param posX X position of the source.
 * @param posY Y position of the source.
 * @param posZ Z position of the source.
 * @param volume Requested playback volume.
 * @param minDistance Near attenuation distance.
 * @param maxDistance Far attenuation distance.
 * @param paused True to start paused.
 */
void AudioManager::EnqueuePlay3D(std::string const& name, float posX, float posY, float posZ,
	float volume, float minDistance, float maxDistance, bool paused) {
	// Store the full spatial request so Update() can execute it on the audio system thread.
	pendingPlays3D.push_back({ name, posX, posY, posZ, volume, minDistance, maxDistance, paused });
}

/**
 * @brief Computes the effective playback volume for a sound request.
 * @param name Logical sound name.
 * @param requestedVolume Caller-requested volume.
 * @return Effective volume after category and master scaling.
 */
float AudioManager::ComputePlaybackVolume(const std::string& name, float requestedVolume) const {
	float finalVolume = requestedVolume * masterVolume;

	if (requestedVolume >= 0.999f && requestedVolume <= 1.001f) {
		// Treat default-volume calls as category-driven so BGM and SFX honor their dedicated sliders.
		if (name.find("bgm") != std::string::npos) {
			finalVolume = bgmVolume * masterVolume;
		}
		else if (name.find("sfx") != std::string::npos || name.find("vfx") != std::string::npos) {
			finalVolume = vfxVolume * masterVolume;
		}
	}

	return finalVolume;
}

/**
 * @brief Queues a channel for deferred stopping after it has mixed silence.
 * @param channel Channel to stop later.
 */
void AudioManager::QueueDeferredStop(FMOD::Channel* channel) {
	if (!channel) {
		return;
	}

	// Avoid queueing the same channel multiple times across repeated stop requests.
	if (std::find(pendingStops.begin(), pendingStops.end(), channel) == pendingStops.end()) {
		pendingStops.push_back(channel);
	}
}

/**
 * @brief Silences and defers stopping for all channels in a tracked list.
 * @param channelList Channel list to stop.
 */
void AudioManager::StopTrackedChannels(ChannelList& channelList) {
	for (FMOD::Channel* channel : channelList) {
		if (!channel) {
			continue;
		}

		// Silence first, then defer the actual stop call until the next FMOD update.
		channel->setVolume(0.0f);
		QueueDeferredStop(channel);
	}
}

/**
 * @brief Removes null or no-longer-playing channels from a tracking list.
 * @param channelList Channel list to prune in place.
 */
void AudioManager::RemoveStoppedChannels(ChannelList& channelList) const {
	channelList.erase(
		std::remove_if(channelList.begin(), channelList.end(),
			[](FMOD::Channel* channel) {
				if (!channel) {
					return true;
				}

				bool playing = false;
				if (channel->isPlaying(&playing) != FMOD_OK) {
					return true;
				}

				// Drop channels that have naturally finished playback.
				return !playing;
			}),
		channelList.end());
}

/**
 * @brief Removes empty channel entries and stale fades from the tracking maps.
 */
void AudioManager::PruneFinishedChannels() {
	for (auto it = channels.begin(); it != channels.end(); ) {
		// Keep the tracking maps compact so lookups only visit live channel groups.
		RemoveStoppedChannels(it->second);
		if (it->second.empty()) {
			activeFades.erase(it->first);
			it = channels.erase(it);
		}
		else {
			++it;
		}
	}
}
