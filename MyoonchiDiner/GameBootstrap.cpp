/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameBootstrap.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (70%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu		(20%)
					Seah Wang Hua, wanghua.seah@digipen.edu (10%)

 DESCRIPTION:		Implements the engine-facing bootstrap interface declared in
					GameBootstrap.hpp. Wires the Myoonchi Diner game module into
					the engine by registering scene bindings, mapping game states
					to JSON levels, and defining per-state audio playback and
					pause/resume policies.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <string>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/GameBootstrap.hpp"
#include "EngineCore/GameStateManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "MyoonchiDiner/GamePaths.hpp"
#include "MyoonchiDiner/MyoonchiDinerBindings.hpp"

namespace {
	// Tracks the currently playing background music channel name so it can be
	// stopped cleanly on the next state transition.
	std::string gCurrentAudio;

	// Tracks the currently playing ambience channel name (used in gameplay).
	std::string gCurrentAmbience;

	constexpr const char* kStartupSplashTexturePath = "../assets/Backgrounds/DigiPen_Singapore_WEB_RED.png";

	// Splash screen duration in seconds. Values <= 0 disable timed display and require manual dismissal.
	constexpr float kStartupSplashDurationSeconds = 3.5f;

	/**
	 * @brief Stops any tracked music and ambience channels from the previous state.
	 *
	 * @param audioManager The audio manager that owns the active channels.
	 */
	void StopCurrentAudio(AudioManager* audioManager) {
		if (!audioManager) {
			return;
		}
		// Stop the last registered BGM channel before the next state starts its own audio.
		if (!gCurrentAudio.empty()) {
			audioManager->StopSound(gCurrentAudio);
			gCurrentAudio.clear();
		}
		// Gameplay ambience is tracked separately so it can be cleaned up alongside BGM.
		if (!gCurrentAmbience.empty()) {
			audioManager->StopSound(gCurrentAmbience);
			gCurrentAmbience.clear();
		}
	}
}

/**
 * @brief Registers the game-specific scene bindings required by the engine.
 *
 * @param scene The scene receiving Myoonchi Diner hook registrations.
 */
void RegisterGameBindings(Scene& scene) {
	// Forward bootstrap registration into the game layer's single binding entry point.
	RegisterMyoonchiDinerBindings(scene);
}

/**
 * @brief Maps engine game states to the JSON levels used by Myoonchi Diner.
 *
 * @param gsm The game state manager being configured during bootstrap.
 */
void ConfigureGameStates(Framework::GameStateManager& gsm) {
	// MainMenu = main menu, Kitchen01 = first kitchen gameplay level, Tutorial = tutorial walkthrough
	gsm.RegisterJsonState(Framework::GameState::MainMenu, MyoonchiPaths::Levels::MAIN_MENU);
	gsm.RegisterJsonState(Framework::GameState::Kitchen01, MyoonchiPaths::Levels::KITCHEN_01);
	gsm.RegisterJsonState(Framework::GameState::Tutorial, MyoonchiPaths::Levels::TUTORIAL);
}

/**
 * @brief Installs per-state audio enter and pause policies for the game module.
 *
 * @param gsm The game state manager that owns the audio policy callbacks.
 */
void ConfigureGameStateAudioPolicy(Framework::GameStateManager& gsm) {
	// Called by the GSM each time a new state is entered. Responsible for
	// stopping the previous state's audio and starting the new state's BGM.
	gsm.SetStateAudioPolicy([](Framework::GameState state, Scene& scene, AudioManager* audioManager) {
		(void)scene;
		if (!scene.ShouldUseRuntimeParityMode()) {
			return;
		}

		if (!audioManager) {
			return;
		}

		StopCurrentAudio(audioManager);

		if (state == Framework::GameState::MainMenu) {
			// Main menu: play menu BGM at full volume immediately
			gCurrentAudio = MyoonchiPaths::Audio::BGM_MAIN_MENU;
			audioManager->PlaySound(gCurrentAudio, audioManager->GetBgmVolume(), false);
		}
		else if (state == Framework::GameState::Kitchen01) {
			// Gameplay: fade in the level theme over 1 second
			gCurrentAudio = MyoonchiPaths::Audio::BGM_LEVEL_THEME;
			audioManager->PlaySound(gCurrentAudio, 0.0f, false);
			audioManager->FadeChannel(gCurrentAudio, audioManager->GetBgmVolume(), 1.0f);


			// Also fade in kitchen ambience at half BGM volume
			gCurrentAmbience = MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE;
			audioManager->PlaySound(gCurrentAmbience, 0.0f, false);
			audioManager->FadeChannel(gCurrentAmbience, audioManager->GetBgmVolume() * 0.5f, 1.0f);
		}
		});


	// Only active during gameplay (GS_Kitchen01). Pauses all audio channels
	// when the simulation is paused and resumes them when un-paused.
	gsm.SetPauseAudioPolicy([](bool isPaused, bool wasPaused, Framework::GameState state, Scene& scene, AudioManager* audioManager) {
		(void)scene;
		if (!audioManager) {
			return;
		}
		// Only handle pause/resume for the gameplay state
		if (state != Framework::GameState::Kitchen01 || gCurrentAudio.empty()) {
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
	
   // Returns the startup splash texture path 
	const char* GetStartupSplashTexturePath() {
		return kStartupSplashTexturePath;
	}

	// Returns the startup splash duration in seconds. Values <= 0 disable timed display.
	float GetStartupSplashDurationSeconds() {
		return kStartupSplashDurationSeconds;
	}
