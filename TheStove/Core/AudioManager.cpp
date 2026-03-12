/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			AudioManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		Audio manager using FMOD for sound playback and management.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "AudioManager.hpp"

#include <algorithm>
#include <filesystem> // For checking file existence

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
}

AudioManager::~AudioManager() {
	// Unsubscribe from messages
	messageBus.Unsubscribe(CoreFramework::MessageType::TOGGLE_DEBUG_INFO, debugInfoSubId);
	messageBus.Unsubscribe(CoreFramework::MessageType::PLAY_AUDIO, playAudioSubId);
	messageBus.Unsubscribe(CoreFramework::MessageType::STOP_AUDIO, stopAudioSubId);

	Shutdown();
}

// Initialize system
void AudioManager::Initialize() {
	if (InitializeSystem()) {
		std::cout << "AudioManager system initialized." << std::endl;

		// Audio files should now be loaded through ResourceManager
	}
	else {
		std::cerr << "AudioManager system failed to initialize." << std::endl;
	}
}

void AudioManager::Update(float dt) {
	//std::cout << "AudioManager system updating with dt: " << dt << std::endl;

	if (!system) return;

	// process queued play requests
	if (!pendingPlays.empty()) {
		for (const auto& playReq : pendingPlays) {
			PlaySound(playReq.name, playReq.volume, playReq.paused);
		}
		pendingPlays.clear();
	}

	// process active volume fades
	if (!activeFades.empty()) {
		for (auto it = activeFades.begin(); it != activeFades.end(); ) {
			auto chanIt = channels.find(it->first);
			if (chanIt == channels.end() || !chanIt->second) {
				it = activeFades.erase(it);
				continue;
			}

			VolumeFade& f = it->second;
			f.elapsed += dt;

			float t = (f.duration > 0.f) ? std::min(f.elapsed / f.duration, 1.f) : 1.f;
			float newVol = f.fromVolume + (f.toVolume - f.fromVolume) * t;
			chanIt->second->setVolume(newVol);

			if (t >= 1.f)
				it = activeFades.erase(it);
			else
				++it;
		}
	}

	// Update FMOD system
	system->update();

	// Stop channels that were silenced last frame — FMOD has now mixed
	// at least one block of silence so stopping won't produce a click.
	for (FMOD::Channel* ch : pendingStops) {
		if (ch) {
			ch->stop();
		}
	}
	pendingStops.clear();

	// Clean up finished channels
	for (auto it = channels.begin(); it != channels.end(); ) {
		bool playing = false;

		if (it->second)
			it->second->isPlaying(&playing);

		if (!playing) // remove finished
			it = channels.erase(it);
		else
			++it;
	}

	// Perform deferred stop on channels
	for (auto& channel : pendingStops) {
		channel->stop();
	}
	pendingStops.clear();
}

void AudioManager::OnToggleDebugInfo(const CoreFramework::Message& msg) {
	// Could toggle audio debug overlay logging, etc.
	(void)msg; // Suppress unused parameter warning
}

void AudioManager::OnPlayAudio(const CoreFramework::Message& msg) {
	// Cast to specific message type
	const auto& playMsg = static_cast<const CoreFramework::PlayAudioMessage&>(msg);

	// Enqueue the audio playback request
	EnqueuePlay(playMsg.soundName, playMsg.volume, playMsg.paused);
}

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

std::string AudioManager::GetName() {
	return "AudioManager";
}

// AudioManager functions

bool AudioManager::InitializeSystem() {
	// Initialize FMOD system
	FMOD_RESULT result = FMOD::System_Create(&system);
	CheckError(result, "System_Create");

	if (result != FMOD_OK) return false;

	// default init with 512 channels
	result = system->init(512, FMOD_INIT_NORMAL, nullptr);
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
			std::cout << "Released sound: " << name << std::endl;
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

FMOD::Sound* AudioManager::LoadSound(std::string const& name, std::string const& filePath, bool loop, bool stream) {
	// Ensure audio system is set
	if (!system) {
		std::cerr << "Audio system not initialized!" << std::endl;
		return nullptr;
	}

	// Check if already loaded
	if (auto it = sounds.find(name); it != sounds.end()) {
		std::cout << "Audio '" << name << "' already loaded, returning existing." << std::endl;
		return it->second;
	}

	// Debug: Print the path we're trying to load
	std::cout << "[AudioManager] Attempting to load: " << name << std::endl;
	std::cout << "  Relative path: " << filePath << std::endl;

	// Check if file exists
	if (!std::filesystem::exists(filePath)) {
		std::cerr << "[AudioManager] File does not exist at path: " << filePath << std::endl;

		// Try to get absolute path for debugging
		try {
			std::filesystem::path absPath = std::filesystem::absolute(filePath);
			std::cerr << "  Absolute path would be: " << absPath.string() << std::endl;
			std::cerr << "  Current working directory: " << std::filesystem::current_path().string() << std::endl;
		}
		catch (...) {
			std::cerr << "  Could not determine absolute path" << std::endl;
		}

		return nullptr;
	}

	std::cout << "  File exists, proceeding with FMOD load..." << std::endl;

	// Set FMOD mode flags
	FMOD_MODE mode = FMOD_DEFAULT | (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) |
		(stream ? FMOD_CREATESTREAM : FMOD_CREATESAMPLE);

	// Load sound
	FMOD::Sound* sound = nullptr;
	FMOD_RESULT result = system->createSound(filePath.c_str(), mode, nullptr, &sound);

	// Check for errors
	if (result != FMOD_OK) {
		std::cerr << "Failed to load audio '" << name << "': " << FMOD_ErrorString(result) << std::endl;
		return nullptr;
	}

	// Store sound
	sounds.emplace(name, sound);

	std::cout << "Loaded audio: " << name << std::endl;
	return sound;
}

FMOD::Sound* AudioManager::GetSound(std::string const& name) const {
	// Check if audio exists
	if (auto it = sounds.find(name); it != sounds.end()) return it->second;

	std::cerr << "Audio '" << name << "' not found!" << std::endl;
	return nullptr;
}

void AudioManager::UnloadSound(std::string const& name) {
	// Stop if playing
	StopSound(name);

	// Check if audio exists and release
	if (auto it = sounds.find(name); it != sounds.end()) {
		if (it->second) {
			it->second->release();
		}
		sounds.erase(it);
		std::cout << "Unloaded audio: " << name << std::endl;
	}
}

bool AudioManager::HasSound(std::string const& name) const {
	// Check if audio exists
	return sounds.find(name) != sounds.end();
}

bool AudioManager::GetSoundInfo(std::string const& name, unsigned int& lengthMs, int& outChannels, int& outBits, float& freq) const {
	auto it = sounds.find(name);
	if (it == sounds.end() || !it->second) {
		std::cerr << "Audio '" << name << "' not found!" << std::endl;
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

void AudioManager::PlaySound(std::string const& name, float volume, bool paused) {
	if (!system) return;

	// Get the sound
	FMOD::Sound* sound = GetSound(name);

	if (!sound) {
		std::cerr << "[AudioManager] PlaySound: Sound '" << name << "' not found in loaded sounds!" << std::endl;
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
		float finalVolume = volume * masterVolume;

		// Only override with category volume if caller used default volume (1.0f)
		// This allows explicit volume control when needed (e.g., sfx_gameover at 50%)
		if (volume >= 0.999f && volume <= 1.001f) {
			if (name.find("bgm") != std::string::npos) {
				finalVolume = bgmVolume * masterVolume;
			}
			else if (name.find("sfx") != std::string::npos || name.find("vfx") != std::string::npos) {
				finalVolume = vfxVolume * masterVolume;
			}
		}

		channel->setVolume(finalVolume);
		channels[name] = channel;

		// Unpause now that volume is set, unless caller requested paused start
		if (!paused) {
			channel->setPaused(false);
		}

		std::cout << "[AudioManager] Playing sound '" << name << "' at volume " << finalVolume << std::endl;
	}
}

void AudioManager::StopSound(std::string const& name) {
	// Stop and remove channel if exists
	auto it = channels.find(name);

	if (it != channels.end() && it->second) {
		// Silence the channel immediately and defer the actual stop to the
		// next Update() so FMOD's mixer processes at least one silent block
		// before the channel is destroyed — this prevents an audible click
		// from cutting the waveform at a non-zero sample.
		it->second->setVolume(0.0f);
		pendingStops.push_back(it->second);
		// Clear any pending software fade
		activeFades.erase(name);
		channels.erase(it);
	}
}

void AudioManager::StopAllSounds() {
	// Silence all tracked channels and defer stop (same as StopSound)
	for (auto& [name, channel] : channels) {
		if (channel) {
			channel->setVolume(0.0f);
			pendingStops.push_back(channel);
		}
	}

	activeFades.clear();
	channels.clear();
}

// Temporarily pause all audio without destroying it
void AudioManager::PauseAll() {
	if (masterGroup) {
		masterGroup->setPaused(true);
	}
}

// Resume audio after PauseAll()
void AudioManager::ResumeAll() {
	if (masterGroup) {
		masterGroup->setPaused(false);
	}
}

// Pause a specific channel by name
void AudioManager::PauseChannel(std::string const& name) {
	auto it = channels.find(name);
	if (it != channels.end() && it->second) {
		it->second->setPaused(true);
		std::cout << "[AudioManager] Paused channel: " << name << std::endl;
	}
}

// Resume a specific channel by name
void AudioManager::ResumeChannel(std::string const& name) {
	auto it = channels.find(name);
	if (it != channels.end() && it->second) {
		it->second->setPaused(false);
		std::cout << "[AudioManager] Resumed channel: " << name << std::endl;
	}
}

void AudioManager::SetMasterVolume(float volume) {
	// Clamp volume between 0.0 and 1.0
	masterVolume = std::clamp(volume, 0.0f, 1.0f);

	if (masterGroup && !muted)
		masterGroup->setVolume(masterVolume);
}

void AudioManager::SetBgmVolume(float volume) {
	// Clamp volume between 0.0 and 1.0
	bgmVolume = std::clamp(volume, 0.0f, 1.0f);

	// BGM volume is relative to master volume
	// Note: Individual channel volumes are set during playback in PlaySound()
}

void AudioManager::SetVfxVolume(float volume) {
	// Clamp volume between 0.0 and 1.0
	vfxVolume = std::clamp(volume, 0.0f, 1.0f);

	// VFX volume is relative to master volume
	// Note: Individual channel volumes are set during playback in PlaySound()
}

float AudioManager::GetMasterVolume() const {
	return masterVolume;
}

float AudioManager::GetBgmVolume() const {
	return bgmVolume;
}

float AudioManager::GetVfxVolume() const {
	return vfxVolume;
}

void AudioManager::SetVolume(std::string const& name, float volume) {
	// Find the channel and set its volume
	auto it = channels.find(name);
	if (it != channels.end() && it->second) {
		float clampedVolume = std::clamp(volume, 0.0f, 1.0f);
		it->second->setVolume(clampedVolume);
	}
}

void AudioManager::Mute(bool shouldMute) {
	// Mute or unmute all audio
	muted = shouldMute;

	if (masterGroup)
		masterGroup->setVolume(muted ? 0.f : masterVolume);
}

bool AudioManager::IsMuted() const {
	return muted;
}

void AudioManager::CheckError(FMOD_RESULT result, std::string const& context) {
	// Log error if not OK
	if (result != FMOD_OK) {
		std::cerr << "[FMOD] Error in " << context << ": " << FMOD_ErrorString(result) << std::endl;
	}
}

// set volume from ConfigManager
void AudioManager::ApplySettings(ConfigManager::Settings const& settings) {
	// Apply audio settings
	SetMasterVolume(settings.masterVolume);
	SetBgmVolume(settings.bgmVolume);
	SetVfxVolume(settings.vfxVolume);
	std::cout << "Audio settings applied: Master Volume = " << settings.masterVolume << ", BGM Volume = " << settings.bgmVolume << ", VFX Volume = " << settings.vfxVolume << std::endl;
}

void AudioManager::EnqueuePlay(std::string const& name, float volume, bool paused) {
	// Add to pending plays queue
	pendingPlays.push_back({ name, volume, paused });
}

void AudioManager::FadeChannel(std::string const& name, float toVolume, float duration) {
	// Start volume fade on channel if exists
	auto it = channels.find(name);

	if (it == channels.end() || !it->second || duration <= 0.f) return;

	// get current volume
	float currentVolume = 0.f;
	it->second->getVolume(&currentVolume);

	// set up fade
	activeFades[name] = VolumeFade{ currentVolume, toVolume, duration, 0.f };
}

void AudioManager::PlayUIClickSound() {
	// Play UI click sound with appropriate volume for subtle feedback
	float clickVolume = GetVfxVolume() * 0.5f; // 50% of VFX volume for subtle UI sounds
	PlaySound("ui_click", clickVolume, false);
}