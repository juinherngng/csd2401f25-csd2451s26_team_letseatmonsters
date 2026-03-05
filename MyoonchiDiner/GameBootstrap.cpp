#include "Core/GameBootstrap.hpp"

#include "Core/AudioManager.hpp"
#include "Core/GameStateManager.hpp"

#include "GamePaths.hpp"
#include "MyoonchiDinerBindings.hpp"

#include <string>

namespace {
	std::string gCurrentAudio;
	std::string gCurrentAmbience;

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
	gsm.RegisterJsonState(Framework::GS_Level1, MyoonchiPaths::Levels::MAIN_MENU);
	gsm.RegisterJsonState(Framework::GS_Level2, MyoonchiPaths::Levels::KITCHEN_01);
}

void ConfigureGameStateAudioPolicy(Framework::GameStateManager& gsm) {
	gsm.SetStateAudioPolicy([](int state, Scene& scene, AudioManager* audioManager) {
		(void)scene;
#ifndef _DEBUG
		if (!audioManager) {
			return;
		}

		StopCurrentAudio(audioManager);

		if (state == Framework::GS_Level1) {
			gCurrentAudio = "bgm_MyoonchiDiner_MainMenu";
			audioManager->PlaySound(gCurrentAudio, audioManager->GetBgmVolume(), false);
		}
		else if (state == Framework::GS_Level2) {
			gCurrentAudio = "bgm_MyoonchiDiner_LevelTheme";
			audioManager->PlaySound(gCurrentAudio, 0.0f, false);
			audioManager->FadeChannel(gCurrentAudio, audioManager->GetBgmVolume(), 1.0f);

			gCurrentAmbience = "bgm_KitchenAmbience";
			audioManager->PlaySound(gCurrentAmbience, 0.0f, false);
			audioManager->FadeChannel(gCurrentAmbience, audioManager->GetBgmVolume() * 0.5f, 1.0f);
		}
#else
		(void)audioManager;
		(void)state;
#endif
	});

	gsm.SetPauseAudioPolicy([](bool isPaused, bool wasPaused, int state, Scene& scene, AudioManager* audioManager) {
		(void)scene;
		if (!audioManager) {
			return;
		}
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
