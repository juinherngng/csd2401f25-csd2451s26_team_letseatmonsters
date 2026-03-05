#include "Core/GameBootstrap.hpp"

#include "Core/FilePaths.hpp"
#include "Core/GameStateManager.hpp"

#include "MyoonchiDinerBindings.hpp"

void RegisterGameBindings(Scene& scene) {
	RegisterMyoonchiDinerBindings(scene);
}

void ConfigureGameStates(Framework::GameStateManager& gsm) {
	gsm.RegisterJsonState(Framework::GS_Level1, FilePaths::Levels::MAIN_MENU);
	gsm.RegisterJsonState(Framework::GS_Level2, FilePaths::Levels::KITCHEN_01);
}
