#include "MyoonchiDinerBindings.hpp"

#include "Core/FilePaths.hpp"
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
#include "Graphics/SceneManager.hpp"

#include <memory>
#include <string>

namespace {
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
			auto* logic = logicManager.AddLogic<MenuButtonLogic>(id, FilePaths::Levels::KITCHEN_01, true);
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

	scene.SetTagLogicBinder(AttachTagLogic);
	scene.SetPauseOverlayButtonBinder(AttachPauseOverlayButton);
}
