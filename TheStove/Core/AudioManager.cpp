/*
----------------------------------------------------------------------------------------------------
FILE NAME:			AudioManager.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		Audio manager using FMOD for sound playback and management.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

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

		rm.SetAudioSystem(system);
		std::cout << "AudioManagerSystem initialized." << std::endl;

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

	(void)dt; // Suppress unused parameter warning

	if (system)
	{
		system->update();
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
	FMOD_RESULT result = FMOD::System_Create(&system);
	CheckError(result, "System_Create");

	if (result != FMOD_OK) return false;

	result = system->init(512, FMOD_INIT_NORMAL, nullptr);
	CheckError(result, "system->init");

	if (result != FMOD_OK) return false;

	result = system->getMasterChannelGroup(&masterGroup);
	CheckError(result, "getMasterChannelGroup");

	if (result != FMOD_OK) return false;

	SetBgmVolume(bgmVolume);
	SetVfxVolume(vfxVolume);
	muted = false;

	return true;
}

void AudioManager::Shutdown()
{
	StopAllSounds();
	channels.clear();

	// sounds released in ResourceManager, not here
	if (system)
	{
		system->release();
		system = nullptr;
	}

	masterGroup = nullptr;
}

bool AudioManager::LoadSound(std::string const& name, std::string const& filepath, bool loop, bool stream)
{
	return ResourceManager::Instance().LoadAudio(name, filepath, loop, stream) != nullptr;
}

void AudioManager::UnloadSound(std::string const& name)
{
	StopSound(name);
	ResourceManager::Instance().UnloadAudio(name);
}

void AudioManager::PlaySound(std::string const& name, float volume, bool paused)
{
	if (!system) return;

	FMOD::Sound* sound = ResourceManager::Instance().GetAudio(name);

	if (!sound) return;

	FMOD::Channel* channel = nullptr;
	FMOD_RESULT result = system->playSound(sound, nullptr, paused, &channel);
	CheckError(result, "playSound: " + name);

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
	auto it = channels.find(name);

	if (it != channels.end() && it->second) {
		it->second->stop();
		channels.erase(it);
	}
}

void AudioManager::StopAllSounds()
{
	if (masterGroup)
		masterGroup->stop();

	channels.clear();
}

void AudioManager::SetBgmVolume(float volume)
{
	bgmVolume = volume;

	if (masterGroup && !muted)
		masterGroup->setVolume(bgmVolume);
}

void AudioManager::SetVfxVolume(float volume)
{
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
	if (result != FMOD_OK) {
		std::cerr << "[FMOD] Error in " << context << ": " << FMOD_ErrorString(result) << std::endl;
	}
}

// set volume from ConfigManager
void AudioManager::ApplySettings(ConfigManager::Settings const& settings) 
{
	SetBgmVolume(settings.bgmVolume);
	SetVfxVolume(settings.vfxVolume);
	std::cout << "Audio settings applied: BGM Volume = " << settings.bgmVolume << ", VFX Volume = " << settings.vfxVolume << std::endl;
}