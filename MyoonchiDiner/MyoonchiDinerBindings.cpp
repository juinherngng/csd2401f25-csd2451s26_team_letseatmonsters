#include "MyoonchiDinerBindings.hpp"

#include "Core/AudioManager.hpp"
#include "Core/HowToPlayButtonLogic.hpp"
#include "Core/IngredientBoxLogic.hpp"
#include "Core/MenuButtonLogic.hpp"
#include "Core/OrderUILogic.hpp"
#include "Core/PauseButtonLogic.hpp"
#include "Core/PlayerLogic.hpp"
#include "Core/TableLogic.hpp"
#include "Core/TrashCanLogic.hpp"
#include "Core/WorkTableLogic.hpp"
#include "Core/CustomerTableLogic.hpp"
#include "Core/ExitGateLogic.hpp"
#include "Core/CustomerManagerLogic.hpp"
#include "Core/Quota.hpp"
#include "Graphics/SceneManager.hpp"

#include "GamePaths.hpp"

#include <memory>
#include <string>

namespace {
	void ApplyRuntimeObjectSetup(Scene& scene, int id, const std::string& tag, const std::string& texturePath, bool animated, const std::string& animName, float speedX, float speedY) {
		(void)speedX;
		(void)speedY;

		if (!animated) {
			return;
		}

		if (tag == "menu_anim") {
			scene.AttachMenuAnimations(id);
			scene.SetAnimation(id, animName.empty() ? "FULL" : animName);
		}
		else if (texturePath.find("dino") != std::string::npos || tag == "dino") {
			scene.AttachDinoAnimations(id);
			scene.SetAnimation(id, animName.empty() ? "IDLE" : animName);
		}
	}

	void UpdateSimulationPolicy(float dt, Scene& scene) {
		float prevTime = Economy::gTimeRemaining;
		Economy::Update(dt, scene);

#ifndef _DEBUG
		if (AudioManager* audioManager = scene.GetAudioManager()) {
			float currentTime = Economy::gTimeRemaining;

			if (!Economy::gPlayed10SecWarning && prevTime > 10.0f && currentTime <= 10.0f) {
				Economy::gPlayed10SecWarning = true;
				if (audioManager->HasSound("sfx_remaining_time")) {
					audioManager->PlaySound("sfx_remaining_time", audioManager->GetVfxVolume(), false);
				}
			}

			if (!Economy::gPlayed3SecBeep && prevTime > 3.0f && currentTime <= 3.0f) {
				Economy::gPlayed3SecBeep = true;
				if (audioManager->HasSound("sfx_beep")) {
					audioManager->PlaySound("sfx_beep", audioManager->GetVfxVolume(), false);
				}
			}
			if (!Economy::gPlayed2SecBeep && prevTime > 2.0f && currentTime <= 2.0f) {
				Economy::gPlayed2SecBeep = true;
				if (audioManager->HasSound("sfx_beep")) {
					audioManager->PlaySound("sfx_beep", audioManager->GetVfxVolume(), false);
				}
			}
			if (!Economy::gPlayed1SecBeep && prevTime > 1.0f && currentTime <= 1.0f) {
				Economy::gPlayed1SecBeep = true;
				if (audioManager->HasSound("sfx_beep")) {
					audioManager->PlaySound("sfx_beep", audioManager->GetVfxVolume(), false);
				}
			}

			if (!Economy::gPlayedTimeUp && currentTime <= 0.0f) {
				Economy::gPlayedTimeUp = true;
				if (audioManager->HasSound("sfx_time_up")) {
					audioManager->PlaySound("sfx_time_up", audioManager->GetVfxVolume(), false);
				}
			}
		}
#else
		(void)scene;
		(void)prevTime;
#endif
	}

	void ApplyDefaultSceneSetup(Scene& scene) {
		scene.SetSceneBackground(MyoonchiPaths::Textures::BACKGROUND);
	}

	void AttachTagLogic(Scene& scene, int id, const std::string& tag) {
		LogicManager& logicManager = scene.GetLogicManager();

		if (tag == "player") {
			logicManager.AddLogic<PlayerLogic>(id);
			scene.SetPlayerID(id);
			scene.AttachPlayerAnimations(id);
			return;
		}

		if (tag == "table") {
			logicManager.AddLogic<TableLogic>(id);
		}
		else if (tag == "work_table") {
			logicManager.AddLogic<WorkTableLogic>(id);
		}
		else if (tag == "customer_table") {
			logicManager.AddLogic<CustomerTableLogic>(id);
		}
		else if (tag == "ingredient_box" || tag == "plate_box") {
			logicManager.AddLogic<IngredientBoxLogic>(id);
		}
		else if (tag == "exit_gate") {
			logicManager.AddLogic<ExitGateLogic>(id);
			scene.RegisterExitGate(id);
		}
		else if (tag == "trash_box") {
			logicManager.AddLogic<TrashCanLogic>(id);
			scene.RegisterExitGate(id);
		}
		else if (tag == "order_ui_logic") {
			logicManager.AddLogic<OrderUILogic>(id);
		}
		else if (tag == "btn_play") {
			auto* logic = logicManager.AddLogic<MenuButtonLogic>(id, MyoonchiPaths::Levels::KITCHEN_01, true);
			if (logic && scene.GetAudioManager()) {
				logic->SetAudioManager(scene.GetAudioManager());
			}
		}
		else if (tag == "btn_howtoplay") {
			logicManager.AddLogic<HowToPlayButtonLogic>(id);
		}
		else if (tag == "btn_quit") {
			logicManager.AddLogic<PauseButtonLogic>(id, PauseAction::Quit);
		}
	}

	void AttachPauseOverlayButton(Scene& scene, int id, const std::string& action) {
		LogicManager& logicManager = scene.GetLogicManager();

		if (action == "resume") {
			logicManager.AddLogic<PauseButtonLogic>(id, PauseAction::Resume);
		}
		else if (action == "howtoplay") {
			logicManager.AddLogic<HowToPlayButtonLogic>(id);
		}
		else if (action == "quit") {
			logicManager.AddLogic<PauseButtonLogic>(id, PauseAction::Quit);
		}
	}
}

void RegisterMyoonchiDinerBindings(Scene& scene) {
	auto customerManager = std::make_shared<CustomerManagerSystem>();
	scene.SetCustomerUpdateHook([customerManager](float dt, Scene& s) {
		customerManager->Update(dt, s);
	});
	scene.SetCustomerResetHook([customerManager](Scene& s) {
		customerManager->Reset();
		(void)s;
	});
	scene.SetRuntimeObjectSetupHook(ApplyRuntimeObjectSetup);
	scene.SetSimulationUpdateHook(UpdateSimulationPolicy);
	scene.SetDefaultSceneSetupHook(ApplyDefaultSceneSetup);

	scene.SetTagLogicBinder(AttachTagLogic);
	scene.SetPauseOverlayButtonBinder(AttachPauseOverlayButton);
}
