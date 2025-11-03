/*
----------------------------------------------------------------------------------------------------
FILE NAME:			AudioLoading.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		Centralized audio catalog for loading all game audio assets.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <string>

namespace Audio
{
	/************************************************************************/
	/*!
	\class AudioCatalog
	\brief
		Centralized manager for loading and unloading all audio assets.
		Provides a single source of truth for all audio files in the game.
	*/
	/************************************************************************/
	class AudioCatalog
	{
	public:
		/************************************************************************/
		/*!
		\brief
			Loads all audio assets for the entire game.
		\details
			Calls LoadUISounds(), LoadGameSounds(), and LoadMusic() in sequence.
			Should be called during application initialization after AudioManager
			is set in ResourceManager.
		*/
		/************************************************************************/
		static void LoadAllAudio();

		/************************************************************************/
		/*!
		\brief
			Unloads all audio assets.
		\details
			Should be called during application shutdown to clean up audio resources.
		*/
		/************************************************************************/
		static void UnloadAllAudio();

		/************************************************************************/
		/*!
		\brief
			Loads UI sound effects.
		\details
			Loads all user interface sounds (clicks, hovers, confirmations, etc.).
			These are typically short, non-looping sounds loaded into memory.
		*/
		/************************************************************************/
		static void LoadUISounds();

		/************************************************************************/
		/*!
		\brief
			Loads gameplay sound effects.
		\details
			Loads all in-game sound effects (cooking sounds, actions, ambient, etc.).
			These may be looping or one-shot sounds, typically loaded into memory.
		*/
		/************************************************************************/
		static void LoadGameSounds();

		/************************************************************************/
		/*!
		\brief
			Loads background music tracks.
		\details
			Loads all music tracks for menus, gameplay, and other scenes.
			These are typically looping and streamed from disk to save memory.
		*/
		/************************************************************************/
		static void LoadMusic();

	private:
		// Prevent instantiation - this is a static utility class
		AudioCatalog() = delete;
		~AudioCatalog() = delete;
		AudioCatalog(const AudioCatalog&) = delete;
		AudioCatalog& operator=(const AudioCatalog&) = delete;
	};
}
