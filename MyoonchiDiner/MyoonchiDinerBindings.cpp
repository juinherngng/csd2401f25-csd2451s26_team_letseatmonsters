/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			MyoonchiDinerBindings.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		Implements all Myoonchi Diner game-specific hooks that are
					injected into the engine's Scene at startup. This includes:
					  - Tag-to-logic dispatch (via a static hash-map table)
					  - Tag-based ID/velocity rules for special objects
					  - Runtime animation attachment for animated objects
					  - Per-frame economy/timer simulation policy
					  - Post-level-load audio and UI setup
					  - Cutscene audio hooks (fade-out, first-frame, pre-load)
					  - Pause overlay button binding
					  - Navigation blocker collection for pathfinding

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

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
	/************************************************************************/
	/*!
	\brief
		Assigns engine-level role IDs and authored velocities to objects
		based on their tag. Called by the engine during level loading for
		every object that has a non-empty tag.
	\param scene   The active Scene.
	\param id      Object ID being configured.
	\param tag     The object's tag string from JSON.
	\param speedX  Authored horizontal speed from JSON.
	\param speedY  Authored vertical speed from JSON.
	*/
	/************************************************************************/
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

	/************************************************************************/
	/*!
	\brief
		Attaches the correct sprite-sheet animation set to an object based
		on its tag or texture path. Called by the engine's RuntimeLevel
		builder for every object marked as animated in JSON.
	\param scene       The active Scene.
	\param id          Object ID.
	\param tag         Object tag from JSON.
	\param texturePath Texture file path (used as fallback for dino detection).
	\param animated    Whether the JSON entry has "animated": true.
	\param animName    Optional initial animation name override from JSON.
	\param speedX      Authored speed X (unused here, forwarded by engine).
	\param speedY      Authored speed Y (unused here, forwarded by engine).
	*/
	/************************************************************************/
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

	/************************************************************************/
	/*!
	\brief
		Per-frame simulation hook. Ticks the economy timer, checks for
		win/lose conditions, and triggers countdown sound effects at
		key time thresholds.
	\param dt     Frame delta time in seconds.
	\param scene  The active Scene (used to reach AudioManager).
	*/
	/************************************************************************/
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

	/************************************************************************/
	/*!
	\brief
		Sets the default scene background texture. Called by the engine
		when a scene is first created before any level JSON is loaded.
	\param scene  The Scene to configure.
	*/
	/************************************************************************/
	void ApplyDefaultSceneSetup(Scene& scene) {
		scene.SetSceneBackground(MyoonchiPaths::Textures::BACKGROUND);
	}

	/************************************************************************/
	/*!
	\brief
		Post-level-load hook. Enables the main-menu UI layer when the
		simulation is inactive (menu state) and starts the appropriate
		BGM for the loaded level. Release-only.
	\param scene            The Scene that just finished loading.
	\param simulationActive True if gameplay is active, false for menus.
	*/
	/************************************************************************/
	void OnPostLevelLoaded(Scene& scene, bool simulationActive) {
#ifndef _DEBUG
		if (!simulationActive) {
			// Ensure the main-menu UI layer is visible and interactive
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

	/************************************************************************/
	/*!
	\brief
		Cutscene fade-out audio hook. Fades the intro cutscene BGM to
		silence over the given duration when a cutscene begins to exit.
	\param scene       The active Scene.
	\param outSeconds  Duration of the fade-out in seconds.
	*/
	/************************************************************************/
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

	/************************************************************************/
	/*!
	\brief
		Cutscene first-frame hook. Starts the win cutscene BGM when the
		first displayed image belongs to the win sequence.
	\param scene       The active Scene.
	\param firstImage  File path of the first cutscene frame being shown.
	*/
	/************************************************************************/
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

	/************************************************************************/
	/*!
	\brief
		Cutscene pre-final-load hook. Cleans up all cutscene audio by stopping 
		or fading them out before the target level is loaded.
	\param scene       The active Scene.
	\param outSeconds  Fade-out duration in seconds for lingering channels.
	*/
	/************************************************************************/
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

	/************************************************************************/
	/*!
	\brief
		Returns the static tag-to-handler dispatch table. Each entry maps
		a JSON tag string to a lambda that attaches the corresponding
		game logic and/or animation to the object. Uses a static local
		so the table is built once and reused on every call (O(1) lookup).
	\return
		Const reference to the dispatch table.
	*/
	/************************************************************************/
	const std::unordered_map<std::string, TagHandler>& GetTagDispatchTable() {
		// Very scalable and easy to maintain as more tags are added.
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

	/************************************************************************/
	/*!
	\brief
		Looks up the given tag in the dispatch table and, if found,
		executes the associated handler to attach game logic and/or
		animations to the object. Unrecognized tags are silently ignored.
	\param scene  The active Scene.
	\param id     Object ID to bind logic to.
	\param tag    The object's tag string from JSON.
	*/
	/************************************************************************/
	void AttachTagLogic(Scene& scene, int id, const std::string& tag) {
		const auto& table = GetTagDispatchTable();
		auto it = table.find(tag);
		if (it != table.end()) {
			it->second(scene, id);
		}
	}

	/************************************************************************/
	/*!
	\brief
		Attaches the correct button logic to a pause-overlay button
		based on its action string.
	\param scene   The active Scene.
	\param id      Object ID of the button.
	\param action  Action identifier authored in the overlay layout.
	*/
	/************************************************************************/
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

	/************************************************************************/
	/*!
	\brief
		Applies level-specific gameplay tuning for customer flow and
		economy progression based on the currently loaded kitchen level.

	\param scene
		The active Scene used to identify the current level.

	\param customerManager
		The CustomerManagerSystem to configure with level-specific
		spawn cooldown and customer capacity values.
	*/
	/************************************************************************/
	void ConfigureLevelGameplayTuning(Scene& scene, CustomerManagerSystem& customerManager) {
		const std::string levelPath = scene.GetCurrentLevelPath();
		const bool isLevel1 = levelPath.find("kitchen01") != std::string::npos;
		const bool isLevel2 = levelPath.find("kitchen02") != std::string::npos;

		if (isLevel2) {
			customerManager.SetSpawnCooldown(6.0f);
			customerManager.SetMaxCustomers(12);
			Economy::SetTimeLimitSeconds(240.0f);
			Economy::SetQuota(310);
		}
		else if (isLevel1) {
			customerManager.SetSpawnCooldown(10.0f);
			customerManager.SetMaxCustomers(4);
			Economy::SetTimeLimitSeconds(180.0f);
			Economy::SetQuota(200);
		}
	}

	/************************************************************************/
	/*!
	\brief
		Builds a list of AABB blockers for the pathfinding system by
		collecting every table-tagged object that has a valid collider
		on an enabled, collidable layer. The mover's own object is
		excluded to prevent self-blocking.
	\param scene          The active Scene.
	\param moverObjectID  Object ID of the entity requesting navigation.
	\param outBoxes       Output vector filled with blocker AABBs.
	*/
	/************************************************************************/
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

/************************************************************************/
/*!
\brief
	Wires every Myoonchi Diner game hook into the engine's Scene.
	Sets up customer management, object/animation setup, simulation
	updates, scene lifecycle hooks, cutscene audio, pause overlay,
	navigation blockers, and tag-velocity classification.
\param scene  The engine Scene to register all hooks on.
*/
/************************************************************************/
void RegisterMyoonchiDinerBindings(Scene& scene) {
	// Customer management system (shared across hooks)
	auto customerManager = std::make_shared<CustomerManagerSystem>();
	scene.SetCustomerUpdateHook([customerManager](float dt, Scene& s) {
		customerManager->Update(dt, s);
	});
	scene.SetCustomerResetHook([customerManager](Scene& s) {
		customerManager->Reset();
		ConfigureLevelGameplayTuning(s, *customerManager);
		Economy::Reset();
	});
	scene.SetRuntimeObjectSetupHook(ApplyRuntimeObjectSetup);
	scene.SetTagRuleHook(ApplyTagRules);

	// Per-frame simulation hook (economy timer + countdown SFX)
	scene.SetSimulationUpdateHook(UpdateSimulationPolicy);

	// Scene lifecycle hooks
	scene.SetDefaultSceneSetupHook(ApplyDefaultSceneSetup);
	scene.SetPostLevelLoadHook(OnPostLevelLoaded);

	// Cutscene audio hooks
	scene.SetCutsceneFadeOutHook(OnCutsceneFadeOut);
	scene.SetCutsceneFirstFrameHook(OnCutsceneFirstFrame);
	scene.SetCutsceneBeforeFinalLoadHook(OnCutsceneBeforeFinalLoad);

	// Pause overlay: tell engine which audio channels to fade on pause
	scene.SetPauseOverlayAudioChannels(MyoonchiPaths::Audio::BGM_LEVEL_THEME, MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE);

	// Logic / UI binders
	scene.SetTagLogicBinder(AttachTagLogic);
	scene.SetPauseOverlayButtonBinder(AttachPauseOverlayButton);
	scene.SetNavigationBlockerCollector(CollectNavigationBlockersForGame);

	// Skip-cutscene audio: fade out intro BGM when player skips
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

	// Tag velocity hook: tells engine which tags use authored velocity
	scene.SetTagUsesVelocityHook([](const std::string& tag) {
		return (tag == "npc1" || tag == "npc2" || tag == "dino");
	});
}
