/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameBootstrap.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		Implements the engine-facing bootstrap interface declared in
					GameBootstrap.hpp. Wires the Myoonchi Diner game module into
					the engine by registering scene bindings, mapping game states
					to JSON levels, and defining per-state audio playback and
					pause/resume policies.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "Core/GameBootstrap.hpp"
#include "Core/AudioManager.hpp"
#include "Core/GameStateManager.hpp"

#include "GamePaths.hpp"
#include "MyoonchiDinerBindings.hpp"

#include <string>

namespace {
// Tracks the currently playing background music channel name so it can be
// stopped cleanly on the next state transition.
std::string gCurrentAudio;

// Tracks the currently playing ambience channel name (used in gameplay).
std::string gCurrentAmbience;

/************************************************************************/
/*!
\brief
Stops any currently playing background music and ambience channels,
then clears the tracking strings. Safe to call with a null pointer.
\param audioManager
Pointer to the AudioManager.
*/
/************************************************************************/
void StopCurrentAudio(AudioManager* audioManager) {
		if (!audioManager) {
			return;
		}
		if (!gCurrentAudio.empty()) {
			audioManager->StopSound(gCurrentAudio);
			gCurrentAudio.clear();
		}
		if (!gCurrentAmbience.empty()) {
			audioManager->StopSound(gCurrentAmbience);
			gCurrentAmbience.clear();
		}
	}
}

void RegisterGameBindings(Scene& scene) {
	RegisterMyoonchiDinerBindings(scene);
}

void ConfigureGameStates(Framework::GameStateManager& gsm) {
// GS_Level1 = main menu, GS_Level2 = first kitchen gameplay level
	gsm.RegisterJsonState(Framework::GS_Level1, MyoonchiPaths::Levels::MAIN_MENU);
	gsm.RegisterJsonState(Framework::GS_Level2, MyoonchiPaths::Levels::KITCHEN_01);
	gsm.RegisterJsonState(Framework::GS_Tutorial, MyoonchiPaths::Levels::TUTORIAL); 
}

void ConfigureGameStateAudioPolicy(Framework::GameStateManager& gsm) {
	// Called by the GSM each time a new state is entered. Responsible for
	// stopping the previous state's audio and starting the new state's BGM.
	gsm.SetStateAudioPolicy([](int state, Scene& scene, AudioManager* audioManager) {
		(void)scene;
#ifndef _DEBUG
		if (!audioManager) {
			return;
		}

		StopCurrentAudio(audioManager);

		if (state == Framework::GS_Level1) {
			// Main menu: play menu BGM at full volume immediately
			gCurrentAudio = MyoonchiPaths::Audio::BGM_MAIN_MENU;
			audioManager->PlaySound(gCurrentAudio, audioManager->GetBgmVolume(), false);
		}
		else if (state == Framework::GS_Level2) {
			// Gameplay: fade in the level theme over 1 second
			gCurrentAudio = MyoonchiPaths::Audio::BGM_LEVEL_THEME;
			audioManager->PlaySound(gCurrentAudio, 0.0f, false);
			audioManager->FadeChannel(gCurrentAudio, audioManager->GetBgmVolume(), 1.0f);


			// Also fade in kitchen ambience at half BGM volume
			gCurrentAmbience = MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE;
			audioManager->PlaySound(gCurrentAmbience, 0.0f, false);
			audioManager->FadeChannel(gCurrentAmbience, audioManager->GetBgmVolume() * 0.5f, 1.0f);
		}
#else
		(void)audioManager;
		(void)state;
#endif
	});


	// Only active during gameplay (GS_Level2). Pauses all audio channels
	// when the simulation is paused and resumes them when un-paused.
	gsm.SetPauseAudioPolicy([](bool isPaused, bool wasPaused, int state, Scene& scene, AudioManager* audioManager) {
		(void)scene;
		if (!audioManager) {
			return;
		}
		// Only handle pause/resume for the gameplay state
		if (state != Framework::GS_Level2 || gCurrentAudio.empty()) {
			return;
		}
		if (isPaused && !wasPaused) {
			audioManager->PauseAll();
		}
		else if (!isPaused && wasPaused) {
			audioManager->ResumeAll();
		}
	});
}
