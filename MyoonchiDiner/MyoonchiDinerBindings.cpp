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
#include "Core/SimpleNpcLogic.hpp"
#include "Core/Quota.hpp"
#include "Graphics/SceneManager.hpp"

#include "GamePaths.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace {
	void ApplyTagRules(Scene& scene, int id, const std::string& tag, float speedX, float speedY) {
		if (tag == "player") {
			scene.SetPlayerID(id);
		}
		else if (tag == "npc1") {
			scene.SetNPC1ID(id);
			scene.SetNPCVelocity(id, speedX, speedY);
		}
		else if (tag == "npc2") {
			scene.SetNPC2ID(id);
			scene.SetNPCVelocity(id, speedX, speedY);
		}
		else if (tag == "dino") {
			scene.SetDinoID(id);
			scene.SetNPCVelocity(id, speedX, speedY);
		}
	}

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
		else if (tag == "customer_template") {
			scene.AttachCustomersAnimations(id);
			scene.SetAnimation(id, animName.empty() ? "IDLE_FRONT" : animName);
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

	void OnPostLevelLoaded(Scene& scene, bool simulationActive) {
#ifndef _DEBUG
		if (!simulationActive) {
			if (Layer* menuLayer = scene.GetLayer("10")) {
				menuLayer->SetVisible(true);
				menuLayer->SetEnabled(true);
			}
		}

		if (AudioManager* audioManager = scene.GetAudioManager()) {
			if (!simulationActive) {
				audioManager->PlaySound(MyoonchiPaths::Audio::BGM_MAIN_MENU, audioManager->GetBgmVolume(), false);
			}
			else {
				const float fadeIn = 1.0f;
				audioManager->PlaySound(MyoonchiPaths::Audio::BGM_LEVEL_THEME, 0.0f, false);
				audioManager->FadeChannel(MyoonchiPaths::Audio::BGM_LEVEL_THEME, audioManager->GetBgmVolume(), fadeIn);

				audioManager->PlaySound(MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE, 0.0f, false);
				audioManager->FadeChannel(MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE, audioManager->GetBgmVolume() * 0.5f, fadeIn);
			}
		}
#else
		(void)scene;
		(void)simulationActive;
#endif
	}

	void OnCutsceneFadeOut(Scene& scene, float outSeconds) {
#ifndef _DEBUG
		if (AudioManager* audioManager = scene.GetAudioManager()) {
			audioManager->FadeChannel(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE, 0.0f, outSeconds);
		}
#else
		(void)scene;
		(void)outSeconds;
#endif
	}

	void OnCutsceneFirstFrame(Scene& scene, const std::string& firstImage) {
#ifndef _DEBUG
		if (AudioManager* audioManager = scene.GetAudioManager()) {
			if (firstImage.find("Win") != std::string::npos || firstImage.find("daychange") != std::string::npos) {
				if (audioManager->HasSound(MyoonchiPaths::Audio::BGM_WIN_CUTSCENE)) {
					audioManager->PlaySound(MyoonchiPaths::Audio::BGM_WIN_CUTSCENE, audioManager->GetBgmVolume(), false);
				}
			}
		}
#else
		(void)scene;
		(void)firstImage;
#endif
	}

	void OnCutsceneBeforeFinalLoad(Scene& scene, float outSeconds) {
#ifndef _DEBUG
		if (AudioManager* audioManager = scene.GetAudioManager()) {
			audioManager->StopSound(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE);
			if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_GAMEOVER)) {
				audioManager->FadeChannel(MyoonchiPaths::Audio::SFX_GAMEOVER, 0.0f, outSeconds);
			}
			if (audioManager->HasSound(MyoonchiPaths::Audio::BGM_WIN_CUTSCENE)) {
				audioManager->FadeChannel(MyoonchiPaths::Audio::BGM_WIN_CUTSCENE, 0.0f, outSeconds);
			}
		}
#else
		(void)scene;
		(void)outSeconds;
#endif
	}

	using TagHandler = std::function<void(Scene&, int)>;

	// I used a dispatch table here using unordered_map for O(1) average lookup instead of if else statements.
	// Very scalable and easy to maintain as more tags are added.
	const std::unordered_map<std::string, TagHandler>& GetTagDispatchTable() {
		static const std::unordered_map<std::string, TagHandler> table = {
			{ "player", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<PlayerLogic>(id);
				scene.SetPlayerID(id);
				scene.AttachPlayerAnimations(id);
			}},
			{ "customer_template", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<SimpleNpcLogic>(id);
			}},
			{ "work_vfx_cut", [](Scene& scene, int id) {
				scene.AttachWorkVfxCutAnimations(id);
				scene.SetAnimation(id, "LOOP");
			}},
			{ "work_vfx_grill", [](Scene& scene, int id) {
				scene.AttachWorkVfxGrillAnimations(id);
				scene.SetAnimation(id, "LOOP");
			}},
			{ "work_vfx_stove", [](Scene& scene, int id) {
				scene.AttachWorkVfxStoveAnimations(id);
				scene.SetAnimation(id, "LOOP");
			}},
			{ "table", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<TableLogic>(id);
			}},
			{ "work_table", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<WorkTableLogic>(id);
			}},
			{ "customer_table", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<CustomerTableLogic>(id);
			}},
			{ "ingredient_box", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<IngredientBoxLogic>(id);
			}},
			{ "plate_box", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<IngredientBoxLogic>(id);
			}},
			{ "exit_gate", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<ExitGateLogic>(id);
				scene.RegisterExitGate(id);
			}},
			{ "trash_box", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<TrashCanLogic>(id);
				scene.RegisterExitGate(id);
			}},
			{ "order_ui_logic", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<OrderUILogic>(id);
			}},
			{ "btn_play", [](Scene& scene, int id) {
				auto* logic = scene.GetLogicManager().AddLogic<MenuButtonLogic>(id, MyoonchiPaths::Levels::KITCHEN_01, true);
				if (logic && scene.GetAudioManager()) {
					logic->SetAudioManager(scene.GetAudioManager());
				}
			}},
			{ "btn_howtoplay", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<HowToPlayButtonLogic>(id);
			}},
			{ "btn_quit", [](Scene& scene, int id) {
				scene.GetLogicManager().AddLogic<PauseButtonLogic>(id, PauseAction::Quit);
			}},
		};
		return table;
	}

	void AttachTagLogic(Scene& scene, int id, const std::string& tag) {
		const auto& table = GetTagDispatchTable();
		auto it = table.find(tag);
		if (it != table.end()) {
			it->second(scene, id);
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

	void CollectNavigationBlockersForGame(Scene& scene, int moverObjectID, std::vector<collision::AABB>& outBoxes) {
		outBoxes.clear();

		LogicManager& logicMgr = scene.GetLogicManager();

		for (GameObject* obj : scene.GetAllObjectsRaw())
		{
			if (!obj) continue;
			if (obj->GetID() == moverObjectID) continue;

			TableLogic* table = logicMgr.GetLogicForObject<TableLogic>(obj->GetID());
			if (!table) continue;

			const Math::Vector2D colSize = obj->GetColliderSize();
			if (colSize.x <= 0.0f || colSize.y <= 0.0f) continue;

			const std::string layerName = scene.GetObjectLayer(obj->GetID());
			Layer* layer = scene.GetLayer(layerName);
			if (layer && (!layer->IsEnabled() || !layer->IsCollidable())) {
				continue;
			}

			const glm::vec3 p = obj->GetPositionGLM();
			outBoxes.push_back(
				physics::MakeColliderBox(obj, Math::Vector3D(p.x, p.y, p.z))
			);
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
	scene.SetTagRuleHook(ApplyTagRules);
	scene.SetSimulationUpdateHook(UpdateSimulationPolicy);
	scene.SetDefaultSceneSetupHook(ApplyDefaultSceneSetup);
	scene.SetPostLevelLoadHook(OnPostLevelLoaded);
	scene.SetCutsceneFadeOutHook(OnCutsceneFadeOut);
	scene.SetCutsceneFirstFrameHook(OnCutsceneFirstFrame);
	scene.SetCutsceneBeforeFinalLoadHook(OnCutsceneBeforeFinalLoad);
	scene.SetPauseOverlayAudioChannels(MyoonchiPaths::Audio::BGM_LEVEL_THEME, MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE);

	scene.SetTagLogicBinder(AttachTagLogic);
	scene.SetPauseOverlayButtonBinder(AttachPauseOverlayButton);
	scene.SetNavigationBlockerCollector(CollectNavigationBlockersForGame);
	scene.SetSkipCutsceneAudioHook([](Scene& s, float outSeconds) {
#ifndef _DEBUG
		if (AudioManager* audioManager = s.GetAudioManager()) {
			audioManager->FadeChannel(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE, 0.0f, outSeconds);
		}
#else
		(void)s;
		(void)outSeconds;
#endif
	});
	scene.SetTagUsesVelocityHook([](const std::string& tag) {
		return (tag == "npc1" || tag == "npc2" || tag == "dino");
	});
}
