#pragma once

class Scene;

namespace Framework {
	class GameStateManager;
}

void RegisterGameBindings(Scene& scene);
void ConfigureGameStates(Framework::GameStateManager& gsm);
void ConfigureGameStateAudioPolicy(Framework::GameStateManager& gsm);
