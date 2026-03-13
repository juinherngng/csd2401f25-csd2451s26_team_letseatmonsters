/*
----------------------------------------------------------------------------------------------------
 FILE NAME:          MyoonchiDinerBindings.cpp
 PROJECT NAME:       Project GAM200
 AUTHOR:             Ng Juin Herng, juinherng.ng@digipen.edu (60%)
 CO-AUTHOR:          Seah Wang Hua, wanghua.seah@digipen.edu (40%)

 DESCRIPTION:
	Central game-integration layer for Myoonchi Diner.

	This file registers and implements Scene hooks that connect engine systems
	to game-specific behavior. In practice, this file is the "policy + glue"
	layer between data-driven level content and runtime game logic.

	Responsibilities:
	  1) Object bootstrap from level JSON
		 - Tag -> logic binding (table dispatch)
		 - Role IDs and authored velocity application
		 - Runtime animation attachment rules

	  2) Global simulation policy
		 - Economy update and time-based SFX warnings
		 - Level-specific customer/economy tuning

	  3) Scene lifecycle orchestration
		 - Default scene setup
		 - Post-load menu/audio setup
		 - Cutscene audio transitions and cleanup

	  4) Tutorial runtime controller
		 - Step-gated tutorial progression
		 - Context-sensitive world highlights
		 - Completion popup handling and return-to-menu flow

	  5) Pause/navigation integration
		 - Pause overlay button action binding
		 - Navigation blocker extraction for pathfinding

	Design notes:
	  - Anonymous-namespace helpers are internal and file-local.
	  - Hook callbacks are intentionally lightweight wrappers that delegate
		detailed behavior to focused helper functions.
	  - Tutorial flow is updated from the customer update hook so popup input
		remains responsive even when simulation is paused/disabled.

	All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "MyoonchiDinerBindings.hpp"

#include "Core/AudioManager.hpp"
#include "Core/HowtoPlayButtonLogic.hpp"
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
#include "Core/StartGamePromptLogic.hpp"
#include "Core/IngredientLogic.hpp"
#include "Core/PlateLogic.hpp"
#include "Graphics/SceneManager.hpp"
#include "FilePaths.hpp"
#include "Graphics/ResourceManager.hpp"
#include "Core/LevelEditorPanelFonts.hpp"
#include "Core/FilePaths.hpp"
#include "Core/InputManager.hpp"
#include "Graphics/GraphicsEngine.hpp"
#include "GamePaths.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <optional>

namespace {
	/************************************************************************/
	/*!
	\brief
		File-local query/helper utilities used by tutorial flow and hook
		callbacks. These helpers perform object scans and lightweight state
		inference from Scene + LogicManager.

	\details
		Most helpers return IDs or booleans and intentionally avoid side effects.
		This keeps hook implementations deterministic and easy to reason about.
	*/
	/************************************************************************/

	static int FindFirstByTagAndTexture(Scene& scene, const std::string& tag, const char* texContains) {
		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;
			const int id = obj->GetID();
			if (scene.GetObjectTag(id) != tag) continue;

			const std::string& tex = scene.GetObjectTexturePath(id);
			if (texContains && tex.find(texContains) == std::string::npos) continue;
			return id;
		}
		return -1;
	}

	static std::optional<DishType> TryGetCurrentOrderDish(Scene& scene) {
		LogicManager& logic = scene.GetLogicManager();

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;

			auto* table = logic.GetLogicForObject<CustomerTableLogic>(obj->GetID());
			if (!table || !table->HasSeatedCustomer()) continue;

			for (int customerId : table->GetSeatedCustomerIDs()) {
				if (customerId < 0) continue;

				auto* npc = logic.GetLogicForObject<SimpleNpcLogic>(customerId);
				if (!npc) continue;

				if (npc->IsWaitingForFood() && npc->HasOrderBeenTaken() && !npc->HasDishServed()) {
					return npc->GetDesiredDishType();
				}
			}
		}

		return std::nullopt;
	}

	static bool IsHeldItemPlate(Scene& scene, LogicManager& logic, int heldItemID) {
			if (heldItemID < 0) return false;

			// Primary check
			if (logic.GetLogicForObject<PlateLogic>(heldItemID) != nullptr) {
				return true;
			}

			// Fallback by texture path
			const std::string& tex = scene.GetObjectTexturePath(heldItemID);
			return (tex.find("plate") != std::string::npos || tex.find("Plate") != std::string::npos);
		}

	static int FindFirstTableHoldingAnyPlate(Scene& scene) {
			LogicManager& logic = scene.GetLogicManager();

			for (GameObject* obj : scene.GetAllObjectsRaw()) {
				if (!obj) continue;

				TableLogic* table = logic.GetLogicForObject<TableLogic>(obj->GetID());
				if (!table || !table->HasItem()) continue;

				if (IsHeldItemPlate(scene, logic, table->GetHeldItemID())) {
					return obj->GetID();
				}
			}

			return -1;
	}

	static std::vector<int> FindAllEmptyTables(Scene& scene) {
		LogicManager& logic = scene.GetLogicManager();
		std::vector<int> ids;

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;

			const int id = obj->GetID();
			if (scene.GetObjectTag(id) != "table") continue;

			TableLogic* table = logic.GetLogicForObject<TableLogic>(id);
			if (!table) continue;
			if (table->HasItem()) continue;

			ids.push_back(id);
		}

		return ids;
	}

	static bool HasAnySeatedCustomer(Scene& scene) {
		LogicManager& logic = scene.GetLogicManager();

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;

			auto* table = logic.GetLogicForObject<CustomerTableLogic>(obj->GetID());
			if (table && table->HasSeatedCustomer()) {
				return true;
			}
		}
		return false;
	}

	static bool HasActiveOrderUi(Scene& scene) {
		// Reuse your existing criteria for "customer order is active / visible"
		return TryGetCurrentOrderDish(scene).has_value();
	}

	static bool IsTutorialLevelLoaded(Scene& scene) {
	const std::string levelPath = scene.GetCurrentLevelPath();
	return levelPath == MyoonchiPaths::Levels::TUTORIAL ||
		levelPath.find("tutorial") != std::string::npos;
}


	static const char* IngredientBoxTokenForDish(DishType d) {
		switch (d) {
		case DishType::VegDish:  return "VegIngredientBox";
		case DishType::MeatDish: return "MeatIngredientBox";
		case DishType::SoupDish: return "MeatIngredientBox"; // first ingredient in your recipe UI
		default:                 return "VegIngredientBox";
		}
	}

	static const char* StationTokenForDish(DishType d) {
		switch (d) {
		case DishType::VegDish:  return "Cutting_Board";
		case DishType::MeatDish: return "Grills";
		case DishType::SoupDish: return "Grills"; // first station in your recipe UI
		default:                 return "Cutting_Board";
		}
	}

	static const char* StationTokenForRawIngredient(IngredientType t) {
		switch (t) {
		case IngredientType::Vegetable: return "Cutting_Board";
		case IngredientType::Meat:      return "Grills";
		case IngredientType::Shroom:    return "Stove";
		default:                        return nullptr;
		}
	}

	static const char* IngredientBoxTokenForDishSecond(DishType d) {
		switch (d) {
		case DishType::VegDish:  return "VegIngredientBox";   // veg + veg
		case DishType::MeatDish: return "VegIngredientBox";   // meat + veg
		case DishType::SoupDish: return "ShroomIngredientBox"; // meat + shroom
		default:                 return "VegIngredientBox";
		}
	}

	static const char* StationTokenForDishSecond(DishType d) {
		switch (d) {
		case DishType::VegDish:  return "Cutting_Board";
		case DishType::MeatDish: return "Cutting_Board";
		case DishType::SoupDish: return "Stove";
		default:                 return "Cutting_Board";
		}
	}

	static int FindTableHoldingPlate(Scene& scene, int minIngredients, bool requireUnpreparedDish) {
		LogicManager& logic = scene.GetLogicManager();

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;

			TableLogic* table = logic.GetLogicForObject<TableLogic>(obj->GetID());
			if (!table || !table->HasItem()) continue;

			const int held = table->GetHeldItemID();
			PlateLogic* plate = logic.GetLogicForObject<PlateLogic>(held);
			if (!plate) continue;

			if (plate->GetIngredientCount() < minIngredients) continue;
			if (requireUnpreparedDish && plate->HasPreparedDish()) continue;

			return obj->GetID();
		}

		return -1;
	}

	static int FindFirstEmptyTable(Scene& scene) {
		LogicManager& logic = scene.GetLogicManager();

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;

			const int id = obj->GetID();
			if (scene.GetObjectTag(id) != "table") continue;

			TableLogic* table = logic.GetLogicForObject<TableLogic>(id);
			if (!table) continue;
			if (table->HasItem()) continue;

			return id;
		}

		return -1;
	}

	static int FindCustomerTableReadyForPayment(Scene& scene) {
		LogicManager& logic = scene.GetLogicManager();

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;

			auto* table = logic.GetLogicForObject<CustomerTableLogic>(obj->GetID());
			if (!table || !table->HasSeatedCustomer()) continue;

			for (int customerId : table->GetSeatedCustomerIDs()) {
				if (customerId < 0) continue;

				auto* npc = logic.GetLogicForObject<SimpleNpcLogic>(customerId);
				if (!npc) continue;

				if (!npc->HasPaid() && (npc->IsPaying() || npc->HasFinishedEating())) {
					return obj->GetID();
				}
			}
		}

		return -1;
	}
}

namespace {
	/************************************************************************/
	/*!
	\brief
		Tutorial finite-state controller.

	\details
		Owns tutorial progression state, highlight overlays, and completion popup
		interaction. The flow is level-gated (tutorial-only) and is safe to tick
		every frame, including while gameplay simulation is inactive.
	*/
	/************************************************************************/
	enum class TutorialStep {
		Move = 0,
		WaitForFirstCustomerOrder,
		PickFirstIngredient,
		ProcessFirstIngredient,
		PlateFirstIngredient,
		PickSecondIngredient,
		ProcessSecondIngredient,
		GetPlateAndPlaceOnTable,
		CombineDishOnPlate,
		ServeDish,
		WaitForCustomerToFinishFood,
		CollectMoneyFromCustomer,
		FinalCustomerFreePlay,
		Done
	};

	struct TutorialFlow {
		bool active = false;
		TutorialStep step = TutorialStep::Move;
		glm::vec2 moveStart{ 0.0f, 0.0f };
		bool moveStartCaptured = false;
		int lastMoney = 0;
		int paymentsCollected_ = 0;
		bool completionPopupShown_ = false;
		std::vector<int> completionPopupIDs_{};

		float blinkAccum_ = 0.0f;
		bool blinkOn_ = true;
		static constexpr float kBlinkHalfPeriod_ = 0.18f;

		static constexpr float kOutlineOffset = 2.5f;
		static constexpr int kOutlineSortOrder = 200;
		static constexpr int kOutlineMaskSortOrder = 201;
		const glm::vec4 kOutlineTint{ 0.0f, 1.0f, 1.0f, 0.92f };

		struct OutlineSet {
			int sourceID = -1;
			std::array<int, 5> ids{ -1, -1, -1, -1, -1 }; // 4 cyan edges + 1 center mask
		};

		OutlineSet ingredientOutline_{};
		OutlineSet stationOutline_{};
		std::vector<OutlineSet> extraStationOutlines_{};

		int completionMenuButtonID_ = -1;
		bool popupMouseHeld_ = false;

		bool GetMouseWorld(InputManager& input, glm::vec2& outWorld) {
			if (GraphicsEngine::Instance().GetMouseWorldInScene(outWorld)) {
				return true;
			}
			glm::vec3 w = input.ScreenToWorld(
				static_cast<float>(input.GetMousePosition().x),
				static_cast<float>(input.GetMousePosition().y));
			outWorld = glm::vec2(w.x, w.y);
			return true;
		}

		bool IsPointInObject(Scene& scene, int objectID, const glm::vec2& p) {
			GameObject* obj = scene.GetGameObjectByID(objectID);
			if (!obj) return false;

			const glm::vec3 pos = obj->GetPositionGLM();
			const glm::vec3 sz = obj->GetScaleGLM();
			const glm::vec2 min(pos.x - sz.x * 0.5f, pos.y - sz.y * 0.5f);
			const glm::vec2 max(pos.x + sz.x * 0.5f, pos.y + sz.y * 0.5f);

			return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
		}

		void HandleCompletionPopupInput(Scene& scene) {
			GLFWwindow* window = glfwGetCurrentContext();
			if (!window) {
				return;
			}

			const bool mouseDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
			const bool clickEdge = mouseDown && !popupMouseHeld_;
			popupMouseHeld_ = mouseDown;

			if (!clickEdge) {
				return;
			}

			double mx = 0.0;
			double my = 0.0;
			glfwGetCursorPos(window, &mx, &my);

			glm::vec2 mouseWorld{};
			if (!GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld)) {
				InputManager& input = InputManager::Get();
				glm::vec3 w = input.ScreenToWorld(static_cast<float>(mx), static_cast<float>(my));
				mouseWorld = glm::vec2(w.x, w.y);
			}

			if (IsPointInObject(scene, completionMenuButtonID_, mouseWorld)) {
				ClearCompletionPopup(scene);
				if (AudioManager* audioManager = scene.GetAudioManager()) {
					audioManager->StopSound(MyoonchiPaths::Audio::BGM_LEVEL_THEME);
					audioManager->StopSound(MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE);
					audioManager->StopSound(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE);
					audioManager->StopSound(MyoonchiPaths::Audio::BGM_WIN_CUTSCENE);
				}
				scene.StartLevelTransition(MyoonchiPaths::Levels::MAIN_MENU, false);
			}
		}

		void ClearExtraStationOutlines(Scene& scene) {
			for (auto& set : extraStationOutlines_) {
				DespawnOutlineSet(scene, set);
			}
			extraStationOutlines_.clear();
		}

		void EnsureExtraStationOutlines(Scene& scene, const std::vector<int>& targetIDs) {
			if (extraStationOutlines_.size() < targetIDs.size()) {
				extraStationOutlines_.resize(targetIDs.size());
			}

			for (size_t i = 0; i < targetIDs.size(); ++i) {
				EnsureOutlineTarget(scene, extraStationOutlines_[i], targetIDs[i]);
			}

			for (size_t i = targetIDs.size(); i < extraStationOutlines_.size(); ++i) {
				DespawnOutlineSet(scene, extraStationOutlines_[i]);
			}
		}


		void DespawnOutlineSet(Scene& scene, OutlineSet& set) {
			for (int id : set.ids) {
				if (id >= 0 && scene.GetGameObjectByID(id)) {
					scene.DespawnByID(id);
				}
			}
			set.ids = { -1, -1, -1, -1, -1 };
			set.sourceID = -1;
		}

		void SetOutlineVisible(Scene& scene, OutlineSet& set, bool visible) {
			const float a = visible ? kOutlineTint.a : 0.0f;
			for (int i = 0; i < 4; ++i) {
				const int id = set.ids[i];
				if (id < 0) continue;
				if (GameObject* o = scene.GetGameObjectByID(id)) {
					o->SetColorTint(glm::vec4(kOutlineTint.r, kOutlineTint.g, kOutlineTint.b, a));
				}
			}
			// mask stays opaque to preserve interior
			if (set.ids[4] >= 0) {
				if (GameObject* m = scene.GetGameObjectByID(set.ids[4])) {
					m->SetColorTint(glm::vec4(1.f, 1.f, 1.f, 1.f));
				}
			}
		}

		void SyncOutlineSet(Scene& scene, OutlineSet& set) {
			if (set.sourceID < 0) return;
			GameObject* src = scene.GetGameObjectByID(set.sourceID);
			if (!src) {
				DespawnOutlineSet(scene, set);
				return;
			}

			const glm::vec3 p = src->GetPositionGLM();
			const glm::vec3 s = src->GetScaleGLM();
			const float r = src->GetRotation();
			const std::string layer = scene.GetObjectLayer(set.sourceID);

			const std::array<glm::vec2, 4> offsets{
				glm::vec2(-kOutlineOffset, 0.0f),
				glm::vec2( kOutlineOffset, 0.0f),
				glm::vec2(0.0f, -kOutlineOffset),
				glm::vec2(0.0f,  kOutlineOffset)
			};

			for (int i = 0; i < 4; ++i) {
				const int id = set.ids[i];
				if (id < 0) continue;
				if (GameObject* o = scene.GetGameObjectByID(id)) {
					o->SetPosition(glm::vec3(p.x + offsets[i].x, p.y + offsets[i].y, p.z));
					o->SetScale(glm::vec3(std::abs(s.x), std::abs(s.y), 1.0f));
					o->SetRotation(r, glm::vec3(0, 0, 1));
					o->SetRenderSortOrder(kOutlineSortOrder);
					scene.AssignObjectToLayer(id, layer);
				}
			}

			const int maskID = set.ids[4];
			if (maskID >= 0) {
				if (GameObject* m = scene.GetGameObjectByID(maskID)) {
					m->SetPosition(p);
					m->SetScale(glm::vec3(std::abs(s.x), std::abs(s.y), 1.0f));
					m->SetRotation(r, glm::vec3(0, 0, 1));
					m->SetRenderSortOrder(kOutlineMaskSortOrder);
					scene.AssignObjectToLayer(maskID, layer);
				}
			}
		}

		void SpawnOutlineSet(Scene& scene, OutlineSet& set, int sourceID) {
			DespawnOutlineSet(scene, set);

			GameObject* src = scene.GetGameObjectByID(sourceID);
			if (!src) return;

			const std::string tex = scene.GetObjectTexturePath(sourceID);
			if (tex.empty()) return;

			const std::string layer = scene.GetObjectLayer(sourceID);
			const glm::vec3 p = src->GetPositionGLM();
			const glm::vec3 s = src->GetScaleGLM();
			const float r = src->GetRotation();

			const std::array<glm::vec2, 4> offsets{
				glm::vec2(-kOutlineOffset, 0.0f),
				glm::vec2( kOutlineOffset, 0.0f),
				glm::vec2(0.0f, -kOutlineOffset),
				glm::vec2(0.0f,  kOutlineOffset)
			};

			for (int i = 0; i < 4; ++i) {
				GameObject* o = scene.SpawnStaticSprite(
					tex,
					glm::vec3(p.x + offsets[i].x, p.y + offsets[i].y, p.z),
					glm::vec2(std::abs(s.x), std::abs(s.y)),
					layer
				);
				if (!o) continue;

				o->SetColorTint(kOutlineTint);
				o->SetColliderSize(Math::Vector2D(0.f, 0.f));
				o->SetMovableByPhysics(false);
				o->EnableShadow(false);
				o->SetRotation(r, glm::vec3(0, 0, 1));
				o->SetRenderSortOrder(kOutlineSortOrder);

				if (Shader* outlineShader = ResourceManager::Instance().GetShader("hover_outline")) {
					o->SetShader(outlineShader);
				}

				set.ids[i] = o->GetID();
			}

			GameObject* m = scene.SpawnStaticSprite(
				tex,
				p,
				glm::vec2(std::abs(s.x), std::abs(s.y)),
				layer
			);
			if (m) {
				m->SetColorTint(glm::vec4(1.f, 1.f, 1.f, 1.f));
				m->SetColliderSize(Math::Vector2D(0.f, 0.f));
				m->SetMovableByPhysics(false);
				m->EnableShadow(false);
				m->SetRotation(r, glm::vec3(0, 0, 1));
				m->SetRenderSortOrder(kOutlineMaskSortOrder);
				set.ids[4] = m->GetID();
			}

			set.sourceID = sourceID;
		}

		void EnsureOutlineTarget(Scene& scene, OutlineSet& set, int targetID) {
			if (targetID < 0) {
				DespawnOutlineSet(scene, set);
				return;
			}
			if (set.sourceID != targetID) {
				SpawnOutlineSet(scene, set, targetID);
			} else {
				SyncOutlineSet(scene, set);
			}
		}

		void ClearHighlights(Scene& scene) {
			DespawnOutlineSet(scene, ingredientOutline_);
			DespawnOutlineSet(scene, stationOutline_);
			ClearExtraStationOutlines(scene);
			blinkAccum_ = 0.0f;
			blinkOn_ = true;
		}

		void UpdateHighlights(Scene& scene, PlayerLogic* playerLogic, LogicManager& logic, float dt) {
			int targetIngredientID = -1;
			int targetStationID = -1;
			std::vector<int> targetStationIDs;

			const DishType dish = TryGetCurrentOrderDish(scene).value_or(DishType::VegDish);

			if (step == TutorialStep::PickFirstIngredient) {
				targetIngredientID = FindFirstByTagAndTexture(scene, "ingredient_box", IngredientBoxTokenForDish(dish));
			}
			else if (step == TutorialStep::ProcessFirstIngredient) {
				const char* stationToken = StationTokenForDish(dish);
				if (playerLogic && playerLogic->IsHolding()) {
					const int heldID = playerLogic->GetCarriedItemID();
					if (auto* ing = logic.GetLogicForObject<IngredientLogic>(heldID); ing && ing->IsRaw()) {
						if (const char* mapped = StationTokenForRawIngredient(ing->GetType())) {
							stationToken = mapped;
						}
					}
				}
				targetStationID = FindFirstByTagAndTexture(scene, "work_table", stationToken);
			}
			else if (step == TutorialStep::PlateFirstIngredient) {
	const bool holdingPlate =
		playerLogic && playerLogic->IsHolding() &&
		IsHeldItemPlate(scene, logic, playerLogic->GetCarriedItemID());

	if (holdingPlate) {
		// Only after picking up plate: highlight all empty tables
		targetIngredientID = -1;
		targetStationID = -1;
		targetStationIDs = FindAllEmptyTables(scene);
	}
	else {
		// Before picking up plate: highlight only plate box
		targetIngredientID = FindFirstByTagAndTexture(scene, "plate_box", nullptr);
		targetStationID = -1;
		targetStationIDs.clear();
	}
}
else if (step == TutorialStep::PickSecondIngredient) {
				targetIngredientID = FindFirstByTagAndTexture(scene, "ingredient_box", IngredientBoxTokenForDishSecond(dish));
			}
			else if (step == TutorialStep::ProcessSecondIngredient) {
				const char* stationToken = StationTokenForDishSecond(dish);
				if (playerLogic && playerLogic->IsHolding()) {
					const int heldID = playerLogic->GetCarriedItemID();
					if (auto* ing = logic.GetLogicForObject<IngredientLogic>(heldID); ing && ing->IsRaw()) {
						if (const char* mapped = StationTokenForRawIngredient( ing->GetType())) {
							stationToken = mapped;
						}
					}
				}
				targetStationID = FindFirstByTagAndTexture(scene, "work_table", stationToken);
			}
			else if (step == TutorialStep::GetPlateAndPlaceOnTable) {
	const bool holdingPlate =
		playerLogic && playerLogic->IsHolding() &&
		IsHeldItemPlate(scene, logic, playerLogic->GetCarriedItemID());

	if (holdingPlate) {
		// Only after picking up plate: highlight all empty tables
		targetIngredientID = -1;
		targetStationID = -1;
		targetStationIDs = FindAllEmptyTables(scene);
	}
	else {
		// Before picking up plate: highlight only plate box
		targetIngredientID = FindFirstByTagAndTexture(scene, "plate_box", nullptr);
		targetStationID = -1;
		targetStationIDs.clear();
	}
}
else if (step == TutorialStep::CombineDishOnPlate) {
				targetStationID = FindTableHoldingPlate(scene, 1, true); // table holding plate with first ingredient
			}
			else if (step == TutorialStep::CollectMoneyFromCustomer) {
				targetStationID = FindCustomerTableReadyForPayment(scene);
			}

			EnsureOutlineTarget(scene, ingredientOutline_, targetIngredientID);

			if (!targetStationIDs.empty()) {
				EnsureOutlineTarget(scene, stationOutline_, targetStationIDs.front());

				if (targetStationIDs.size() > 1) {
					std::vector<int> extras(targetStationIDs.begin() + 1, targetStationIDs.end());
					EnsureExtraStationOutlines(scene, extras);
				}
				else {
					ClearExtraStationOutlines(scene);
				}
			}
			else {
				EnsureOutlineTarget(scene, stationOutline_, targetStationID);
				ClearExtraStationOutlines(scene);
			}

			blinkAccum_ += dt;
			while (blinkAccum_ >= kBlinkHalfPeriod_) {
				blinkAccum_ -= kBlinkHalfPeriod_;
				blinkOn_ = !blinkOn_;
			}

			SetOutlineVisible(scene, ingredientOutline_, blinkOn_);
			SetOutlineVisible(scene, stationOutline_, blinkOn_);
			for (auto& set : extraStationOutlines_) {
				SetOutlineVisible(scene, set, blinkOn_);
			}
		}

		void Reset(Scene& scene, bool enable) {
			ClearHighlights(scene);
			ClearCompletionPopup(scene);
			active = enable;
			step = TutorialStep::Move;
			moveStart = { 0.0f, 0.0f };
			moveStartCaptured = false;
			lastMoney = Economy::gPlayerMoney;
			paymentsCollected_ = 0;
			LEPANELFONTS::SetTextByName("TutorialText", active ? "Click anywhere to move." : "");
		}

		void Advance(const std::string& nextText) {
			if (step != TutorialStep::Done) {
				step = static_cast<TutorialStep>(static_cast<int>(step) + 1);
			}
			LEPANELFONTS::SetTextByName("TutorialText", nextText);
		}

		void Update(Scene& scene, float dt) {
	const bool isTutorial = IsTutorialLevelLoaded(scene);
	if (!isTutorial) {
		ClearHighlights(scene);
		ClearCompletionPopup(scene);
		active = false;
		return;
	}

	if (completionPopupShown_) {
		ClearHighlights(scene);
		HandleCompletionPopupInput(scene);
		return;
	}

	if (!active || !scene.IsSimulationActive()) {
		ClearHighlights(scene);
		return;
	}

			const int playerId = scene.GetPlayerID();
			GameObject* player = scene.GetGameObjectByID(playerId);
			if (!player) return;

			LogicManager& logic = scene.GetLogicManager();
			PlayerLogic* playerLogic = logic.GetLogicForObject<PlayerLogic>(playerId);
			if (!playerLogic) return;

			switch (step) {
			case TutorialStep::Move:
			{
				glm::vec3 p = player->GetPositionGLM();
				if (!moveStartCaptured) {
					moveStart = { p.x, p.y };
					moveStartCaptured = true;
				}
				glm::vec2 d = glm::vec2(p.x, p.y) - moveStart;
				if (glm::dot(d, d) > (40.0f * 40.0f)) {
					Advance("Wait for a customer to arrive.");
				}
				break;
			}

			case TutorialStep::WaitForFirstCustomerOrder:
			{
				if (!HasAnySeatedCustomer(scene)) {
					LEPANELFONTS::SetTextByName("TutorialText", "Wait for a customer to arrive.");
				}
				else if (!HasActiveOrderUi(scene)) {
					LEPANELFONTS::SetTextByName("TutorialText", "Wait for a customer to arrive.");
				}
				else {
					Advance("Pick the first ingredient from the correct ingredient box as seen in the Order.");
				}
				break;
			}

			case TutorialStep::PickFirstIngredient:
			{
				if (playerLogic->IsHolding()) {
					const int heldId = playerLogic->GetCarriedItemID();
					if (auto* ing = logic.GetLogicForObject<IngredientLogic>(heldId); ing && ing->IsRaw()) {
						Advance("Bring it to the correct workstation as seen in the Order.");
					}
				}
				break;
			}

			case TutorialStep::ProcessFirstIngredient:
			{
				bool startedProcessing = false;
				for (GameObject* obj : scene.GetAllObjectsRaw()) {
					if (!obj) continue;
					if (auto* wt = logic.GetLogicForObject<WorkTableLogic>(obj->GetID()); wt && wt->IsProcessing()) {
						startedProcessing = true;
						break;
					}
				}

				bool holdingProcessed = false;
				if (playerLogic->IsHolding()) {
					const int heldId = playerLogic->GetCarriedItemID();
					if (auto* ing = logic.GetLogicForObject<IngredientLogic>(heldId); ing && ing->IsProcessed()) {
						holdingProcessed = true;
					}
				}

				if (startedProcessing || holdingProcessed) {
					Advance("Take a plate from the plate box, place it on any empty table.");
				}
				break;
			}

			case TutorialStep::PlateFirstIngredient:
			{
				bool firstOnPlate = false;
				for (GameObject* obj : scene.GetAllObjectsRaw()) {
					if (!obj) continue;
					if (auto* plate = logic.GetLogicForObject<PlateLogic>(obj->GetID());
						plate && plate->GetIngredientCount() >= 1 && !plate->HasPreparedDish()) {
						firstOnPlate = true;
						break;
					}
				}
				if (FindFirstTableHoldingAnyPlate(scene) >= 0) {
					Advance("Pick the second ingredient from the correct ingredient box as seen in the Order.");
				}
				break;
			}

			case TutorialStep::PickSecondIngredient:
			{
				if (playerLogic->IsHolding()) {
					const int heldId = playerLogic->GetCarriedItemID();
					if (auto* ing = logic.GetLogicForObject<IngredientLogic>(heldId); ing && ing->IsRaw()) {
						Advance("Bring the second ingredient to its workstation.");
					}
				}
				break;
			}

			case TutorialStep::ProcessSecondIngredient:
			{
				bool startedProcessing = false;
				for (GameObject* obj : scene.GetAllObjectsRaw()) {
					if (!obj) continue;
					if (auto* wt = logic.GetLogicForObject<WorkTableLogic>(obj->GetID()); wt && wt->IsProcessing()) {
						startedProcessing = true;
						break;
					}
				}

				bool holdingProcessed = false;
				if (playerLogic->IsHolding()) {
					const int heldId = playerLogic->GetCarriedItemID();
					if (auto* ing = logic.GetLogicForObject<IngredientLogic>(heldId); ing && ing->IsProcessed()) {
						holdingProcessed = true;
					}
				}

				if (startedProcessing || holdingProcessed) {
					Advance("Collect a plate and place it on any empty table.");
				}
				break;
			}

			case TutorialStep::GetPlateAndPlaceOnTable:
			{
				if (FindFirstTableHoldingAnyPlate(scene) >= 0) {
					Advance("Place both processed ingredients on the same plate to combine and make the dish.");
				}
				break;
			}

			case TutorialStep::CombineDishOnPlate:
			{
				bool dishReady = false;
				for (GameObject* obj : scene.GetAllObjectsRaw()) {
					if (!obj) continue;
					if (auto* plate = logic.GetLogicForObject<PlateLogic>(obj->GetID()); plate && plate->HasPreparedDish()) {
						dishReady = true;
						break;
					}
				}
				if (dishReady) {
					Advance("Serve the completed dish to a customer table.");
				}
				break;
			}

			case TutorialStep::ServeDish:
			{
				bool dishServed = false;
				for (GameObject* obj : scene.GetAllObjectsRaw()) {
					if (!obj) continue;

					auto* table = logic.GetLogicForObject<CustomerTableLogic>(obj->GetID());
					if (!table || !table->HasSeatedCustomer()) continue;

					for (int customerId : table->GetSeatedCustomerIDs()) {
						if (customerId < 0) continue;

						auto* npc = logic.GetLogicForObject<SimpleNpcLogic>(customerId);
						if (!npc) continue;

						if (npc->HasDishServed()) {
							dishServed = true;
							break;
						}
					}

					if (dishServed) {
						Advance("Wait for customer to finish food.");
					}
					break;
				}
				break;
			}

			case TutorialStep::WaitForCustomerToFinishFood:
			{
				bool readyForPayment = false;
				for (GameObject* obj : scene.GetAllObjectsRaw()) {
					if (!obj) continue;

					auto* table = logic.GetLogicForObject<CustomerTableLogic>(obj->GetID());
					if (!table || !table->HasSeatedCustomer()) continue;

					for (int customerId : table->GetSeatedCustomerIDs()) {
						if (customerId < 0) continue;

						auto* npc = logic.GetLogicForObject<SimpleNpcLogic>(customerId);
						if (!npc) continue;

						if (!npc->HasPaid() && (npc->IsPaying() || npc->HasFinishedEating())) {
							readyForPayment = true;
							break;
						}
					}

					if (readyForPayment) {
						Advance("Collect money from customer.");
					}
					break;
				}

				break;
			}

			case TutorialStep::CollectMoneyFromCustomer:
			{
				if (Economy::gPlayerMoney > lastMoney) {
					lastMoney = Economy::gPlayerMoney;
					++paymentsCollected_;

					if (paymentsCollected_ >= 2) {
						step = TutorialStep::Done;
						LEPANELFONTS::SetTextByName("TutorialText", "");
						ShowCompletionPopup(scene);
					}
					else {
						// No walkthrough for customer 2; just show one final objective line.
						step = TutorialStep::FinalCustomerFreePlay;
						LEPANELFONTS::SetTextByName("TutorialText", "Serve the last customer to complete the Tutorial!");
					}
				}
				break;
			}

			case TutorialStep::FinalCustomerFreePlay:
			{
				// Free-play phase: no step-by-step gating/highlights, only wait for final payment.
				if (Economy::gPlayerMoney > lastMoney) {
					lastMoney = Economy::gPlayerMoney;
					++paymentsCollected_;

					if (paymentsCollected_ >= 2) {
						step = TutorialStep::Done;
						LEPANELFONTS::SetTextByName("TutorialText", "");
						ShowCompletionPopup(scene);
					}
				}
				break;
			}

			case TutorialStep::Done:
				break;
			}

			if (step == TutorialStep::Done) {
				ClearHighlights(scene);
			}
			else {
				UpdateHighlights(scene, playerLogic, logic, dt);
			}
		}

		void ClearCompletionPopup(Scene& scene) {
	for (int id : completionPopupIDs_) {
		if (id >= 0 && scene.GetGameObjectByID(id)) {
			scene.DespawnByID(id);
		}
	}
	completionPopupIDs_.clear();
	completionPopupShown_ = false;
	completionMenuButtonID_ = -1;
	popupMouseHeld_ = false;
}

		void ShowCompletionPopup(Scene& scene) {
	if (completionPopupShown_) return;
	completionPopupShown_ = true;

	// Freeze gameplay while popup is shown.
	scene.SetSimulationActive(false);

	const glm::vec3 center{
		static_cast<float>(GraphicsEngine::kRefW) * 0.5f,
		static_cast<float>(GraphicsEngine::kRefH) * 0.5f,
		0.0f
	};
	const std::string uiLayer = "999999";

	if (GameObject* popup = scene.SpawnStaticSprite(
		"../assets/tutorial_complete.png",
		center,
		glm::vec2(1152.0f, 648.0f),
		uiLayer)) {
		completionPopupIDs_.push_back(popup->GetID());
	}

	if (GameObject* menu = scene.SpawnStaticSprite(
		FilePaths::Textures::BTN_QUIT,
		glm::vec3(center.x, center.y + 160.0f, 0.0f),
		glm::vec2(350.0f, 100.0f),
		uiLayer)) {
		completionMenuButtonID_ = menu->GetID();
		completionPopupIDs_.push_back(completionMenuButtonID_);
		scene.SetObjectTexturePath(completionMenuButtonID_, FilePaths::Textures::BTN_QUIT);
		// Do NOT attach MenuButtonLogic here.
	}
}
	};

	static TutorialFlow gTutorialFlow;
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

		if (AudioManager* audioManager = scene.GetAudioManager()) {
			float currentTime = Economy::gTimeRemaining;

			if (!Economy::gPlayed10SecWarning && prevTime > 10.0f && currentTime <= 10.0f) {
				Economy::gPlayed10SecWarning = true;
				if (audioManager->HasSound("sfx_clock_ticking_10secs")) {
					audioManager->PlaySound("sfx_clock_ticking_10secs", audioManager->GetVfxVolume() * 1.2f, false);
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
	void OnPostLevelLoaded(Scene& scene, bool simulationActive, CustomerManagerSystem& customerManager) {
		const bool isTutorial = simulationActive && IsTutorialLevelLoaded(scene);
	gTutorialFlow.Reset(scene, isTutorial);

	if (isTutorial) {
		customerManager.SetMaxCustomers(2);
		customerManager.SetTotalSpawnLimit(2);
		customerManager.SetSpawnedCustomersInfinitePatience(true);

		// Prevent normal quota win cutscene during tutorial.
		Economy::SetQuota(999);
		Economy::gQuotaReached = false;
	}
	else {
		customerManager.SetMaxCustomers(4);
		customerManager.ClearTotalSpawnLimit();
		customerManager.SetSpawnedCustomersInfinitePatience(false);
	}

	if (AudioManager* audioManager = scene.GetAudioManager()) {
		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) continue;
			if (scene.GetObjectTag(obj->GetID()) != "btn_play") continue;
			if (auto* logic = scene.GetLogicManager().GetLogicForObject<StartGamePromptLogic>(obj->GetID())) {
				logic->SetAudioManager(audioManager);
			}
		}
	}

#ifndef _DEBUG
	// existing audio/UI logic unchanged...
	if (!simulationActive) {
		if (Layer* menuLayer = scene.GetLayer("10")) {
			menuLayer->SetVisible(true);
			menuLayer->SetEnabled(true);
		}
	}

	if (AudioManager* audioManager = scene.GetAudioManager()) {
		if (!simulationActive) {
			audioManager->StopSound(MyoonchiPaths::Audio::BGM_LEVEL_THEME);
			audioManager->StopSound(MyoonchiPaths::Audio::BGM_KITCHEN_AMBIENCE);
			audioManager->StopSound(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE);
			audioManager->StopSound(MyoonchiPaths::Audio::BGM_WIN_CUTSCENE);
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
				auto* logic = scene.GetLogicManager().AddLogic<StartGamePromptLogic>(id, MyoonchiPaths::Levels::TUTORIAL, MyoonchiPaths::Levels::KITCHEN_01, true);
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
			{ "btn_next_level", [](Scene& scene, int id) {
				auto* logic = scene.GetLogicManager().AddLogic<MenuButtonLogic>(id, FilePaths::Levels::KITCHEN_02, true);
				if (logic && scene.GetAudioManager()) {
					logic->SetAudioManager(scene.GetAudioManager());
				}
			}},
			{ "btn_retry_level", [](Scene& scene, int id) {
				auto* logic = scene.GetLogicManager().AddLogic<MenuButtonLogic>(id, scene.GetCurrentLevelPath(), true);
				if (logic && scene.GetAudioManager()) {
					logic->SetAudioManager(scene.GetAudioManager());
				}
			}},
			{ "btn_main_menu", [](Scene& scene, int id) {
				auto* logic = scene.GetLogicManager().AddLogic<MenuButtonLogic>(id, FilePaths::Levels::MAIN_MENU, false);
				if (logic && scene.GetAudioManager()) {
					logic->SetAudioManager(scene.GetAudioManager());
				}
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

		for (GameObject* obj : scene.GetAllObjectsRaw()) {
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
	Hook registration entry point.

\details
	Order of registration:
	  1) Customer hooks (update/reset)
	  2) Runtime/tag setup hooks
	  3) Simulation and scene lifecycle hooks
	  4) Cutscene and pause/audio hooks
	  5) Logic binders and navigation collector

	This function should remain side-effect free beyond hook wiring.
*/
/************************************************************************/
void RegisterMyoonchiDinerBindings(Scene& scene) {
	// Customer management system (shared across hooks)
	auto customerManager = std::make_shared<CustomerManagerSystem>();
	auto applyLevelGameplayTuning = [customerManager](Scene& s) {
		ConfigureLevelGameplayTuning(s, *customerManager);
	};

	scene.SetCustomerUpdateHook([customerManager](float dt, Scene& s) {
		customerManager->Update(dt, s);

		// Always tick tutorial flow so completion popup input still works
		// even when simulation is disabled.
		gTutorialFlow.Update(s, dt);
	});
	scene.SetCustomerResetHook([customerManager, applyLevelGameplayTuning](Scene& s) {
		customerManager->Reset();
		applyLevelGameplayTuning(s);
		Economy::Reset();
		});
	scene.SetRuntimeObjectSetupHook(ApplyRuntimeObjectSetup);
	scene.SetTagRuleHook(ApplyTagRules);

	// Per-frame simulation hook (economy timer + countdown SFX)
	scene.SetSimulationUpdateHook(UpdateSimulationPolicy);

	// Scene lifecycle hooks
	scene.SetDefaultSceneSetupHook(ApplyDefaultSceneSetup);
	scene.SetPostLevelLoadHook([customerManager, applyLevelGameplayTuning](Scene& s, bool simulationActive) {
		OnPostLevelLoaded(s, simulationActive, *customerManager);
		if (simulationActive) {
			applyLevelGameplayTuning(s);
		}
		});

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

	// Skip-cutscene audio: stop intro BGM and play skip SFX when player skips
	scene.SetSkipCutsceneAudioHook([](Scene& s, float outSeconds) {
#ifndef _DEBUG
		(void)outSeconds;
		if (AudioManager* audioManager = s.GetAudioManager()) {
			// Stop intro BGM immediately to avoid an audio pop caused by
			// OnCutsceneBeforeFinalLoad hard-stopping the channel mid-fade.
			audioManager->StopSound(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE);

			// Play skip cutscene SFX as 2D UI sound to preserve its initial transient.
			if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_SKIP_INTRO_CUTSCENE)) {
				audioManager->PlaySound(MyoonchiPaths::Audio::SFX_SKIP_INTRO_CUTSCENE,
					audioManager->GetVfxVolume(),
					false);
			}
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
