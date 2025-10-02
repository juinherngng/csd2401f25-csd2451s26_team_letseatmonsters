/*
----------------------------------------------------------------------------------------------------
FILE NAME:			AudioManager.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		Audio manager using FMOD for sound playback and management.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <algorithm>

#include "AudioManager.hpp"
#include "../Graphics/ResourceManager.h"

AudioManager::AudioManager() : system(nullptr), masterGroup(nullptr), bgmVolume(1.f), vfxVolume(1.f), muted(false) {}

AudioManager::~AudioManager() 
{
	Shutdown();
}

// Initialize system
void AudioManager::Initialize()
{
	if (InitializeSystem())
	{
		auto& rm = ResourceManager::Instance();

		// inject FMOD system instance into ResourceManager
		rm.SetAudioSystem(system);
		std::cout << "AudioManagerSystem initialized." << std::endl;

		// Load initial sounds here or later as needed
		auto* snd = rm.LoadAudio("boiling sound", "../assets/Audio/Boiling7.wav", true, true);
		if (!snd)
		{
			std::cerr << "Failed to load audio 'boiling sound'\n";
		}
		else
		{
			unsigned int lenMs = 0;
			int ch = 0, bits = 0;
			float freq = 0;
			if (rm.GetAudioInfo("boiling sound", lenMs, ch, bits, freq))
			{
				std::cout << "Audio 'boiling sound' info - Length: " << lenMs << " ms, Channels: " << ch << ", Bits: " << bits << ", Frequency: " << freq << " Hz\n";
			}
			else
			{
				std::cerr << "Failed to get audio info for 'boiling sound'\n";
			}
		}

		auto* snd1 = rm.LoadAudio("background music", "../assets/Audio/bgm.wav", true, true);
		if (!snd1)
		{
			std::cerr << "Failed to load audio 'background music'\n";
		}
		else
		{
			unsigned int lenMs = 0;
			int ch = 0, bits = 0;
			float freq = 0;
			if (rm.GetAudioInfo("background music", lenMs, ch, bits, freq))
			{
				std::cout << "Audio 'background music' info - Length: " << lenMs << " ms, Channels: " << ch << ", Bits: " << bits << ", Frequency: " << freq << " Hz\n";
			}
			else
			{
				std::cerr << "Failed to get audio info for 'background music'\n";
			}
		}
	}
	else
	{
		std::cerr << "AudioManagerSystem failed to initialize." << std::endl;
	}
}

void AudioManager::Update(float dt)
{
	// uncomment to check update calls
	//std::cout << "AudioManagerSystem updating with dt: " << dt << std::endl;

	if (!system) return;

	// process queued play requests
	if (!pendingPlays.empty())
	{
		for (const auto& playReq : pendingPlays)
		{
			PlaySound(playReq.name, playReq.volume, playReq.paused);
		}
		pendingPlays.clear();
	}

	// process active volume fades
	if (!activeFades.empty())
	{
		for (auto it = activeFades.begin(); it != activeFades.end(); )
		{
			auto chanIt = channels.find(it->first);
			if (chanIt == channels.end() || !chanIt->second)
			{
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

	// Clean up finished channels
	for (auto it = channels.begin(); it != channels.end(); )
	{
		bool playing = false;

		if (it->second)
			it->second->isPlaying(&playing);

		if (!playing) // remove finished
			it = channels.erase(it);
		else
			++it;
	}
}

void AudioManager::SendMessage(CoreFramework::Message* message)
{
	switch (message->MessageId)
	{
	case CoreFramework::MsgId::TOGGLE_DEBUG_INFO:
		// Could toggle audio debug overlay logging, etc.
		break;
	default:
		break;
	}
}

std::string AudioManager::GetName()
{
	return "AudioManagerSystem";
}

// AudioManager functions

bool AudioManager::InitializeSystem()
{
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
	SetBgmVolume(bgmVolume);
	SetVfxVolume(vfxVolume);
	muted = false;

	return true;
}

void AudioManager::Shutdown()
{
	// stop all sounds and clear channels
	StopAllSounds();
	channels.clear();

	// sounds released in ResourceManager, not here
	if (system)
	{
		// close and release FMOD system
		system->release();
		system = nullptr;
	}

	masterGroup = nullptr;
}

bool AudioManager::LoadSound(std::string const& name, std::string const& filepath, bool loop, bool stream)
{
	// Load sound via ResourceManager
	return ResourceManager::Instance().LoadAudio(name, filepath, loop, stream) != nullptr;
}

void AudioManager::UnloadSound(std::string const& name)
{
	// Stop if playing
	StopSound(name);
	ResourceManager::Instance().UnloadAudio(name);
}

void AudioManager::PlaySound(std::string const& name, float volume, bool paused)
{
	if (!system) return;

	// Check if already playing, stop first
	FMOD::Sound* sound = ResourceManager::Instance().GetAudio(name);

	if (!sound) return;

	FMOD::Channel* channel = nullptr;
	FMOD_RESULT result = system->playSound(sound, nullptr, paused, &channel);
	CheckError(result, "playSound: " + name);

	// set volume based on type
	if (result == FMOD_OK && channel) 
	{
		float finalVolume = volume;

		if (name.find("bgm") != std::string::npos) 
		{
			finalVolume = bgmVolume;
		}
		else if(name.find("sfx") != std::string::npos || name.find("vfx") != std::string::npos) 
		{
			finalVolume = vfxVolume;
		}

		channel->setVolume(finalVolume);
		channels[name] = channel;
	}
}

void AudioManager::StopSound(std::string const& name)
{
	// Stop and remove channel if exists
	auto it = channels.find(name);

	if (it != channels.end() && it->second) {
		it->second->stop();
		channels.erase(it);
	}
}

void AudioManager::StopAllSounds()
{
	// Stop all channels
	if (masterGroup)
		masterGroup->stop();

	channels.clear();
}

void AudioManager::SetBgmVolume(float volume)
{
	// Clamp volume between 0.0 and 1.0
	bgmVolume = volume;

	if (masterGroup && !muted)
		masterGroup->setVolume(bgmVolume);
}

void AudioManager::SetVfxVolume(float volume)
{
	// Clamp volume between 0.0 and 1.0
	vfxVolume = volume;

	if (masterGroup && !muted)
		masterGroup->setVolume(vfxVolume);
}

float AudioManager::GetBgmVolume() const
{
	return bgmVolume;
}

float AudioManager::GetVfxVolume() const
{
	return vfxVolume;
}

void AudioManager::Mute(bool shouldMute)
{
	// Mute or unmute all audio
	muted = shouldMute;

	if (masterGroup)
		masterGroup->setVolume(muted ? 0.f : bgmVolume);
}

bool AudioManager::IsMuted() const
{
	return muted;
}

void AudioManager::CheckError(FMOD_RESULT result, std::string const& context)
{
	// Log error if not OK
	if (result != FMOD_OK) {
		std::cerr << "[FMOD] Error in " << context << ": " << FMOD_ErrorString(result) << std::endl;
	}
}

// set volume from ConfigManager
void AudioManager::ApplySettings(ConfigManager::Settings const& settings) 
{
	// Apply audio settings
	SetBgmVolume(settings.bgmVolume);
	SetVfxVolume(settings.vfxVolume);
	std::cout << "Audio settings applied: BGM Volume = " << settings.bgmVolume << ", VFX Volume = " << settings.vfxVolume << std::endl;
}

void AudioManager::EnqueuePlay(std::string const& name, float volume, bool paused)
{
	// Add to pending plays queue
	pendingPlays.push_back({ name, volume, paused });
}

void AudioManager::FadeChannel(std::string const& name, float toVolume, float duration)
{
	// Start volume fade on channel if exists
	auto it = channels.find(name);

	if (it == channels.end() || !it->second || duration <= 0.f) return;

	// get current volume
	float currentVolume = 0.f;
	it->second->getVolume(&currentVolume);

	// set up fade
	activeFades[name] = VolumeFade{ currentVolume, toVolume, duration, 0.f };
}